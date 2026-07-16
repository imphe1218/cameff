from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
import json
import math

import numpy as np
import pandas as pd
from scipy.optimize import minimize

ALPHA_GRID = (0.5, 1.0, 1.5, 2.0)
C_GRID = (0.05, 0.10, 0.50)
P_GRID = (1.0, 1.2, 1.5)
TRIGGER_MEMORY_DAYS = 365
TIE_TOLERANCE_LOGLIK = 1e-12
MIN_EVENTS_FOR_MC = 200
MIN_EVENTS_ABOVE_MC = 100


@dataclass(frozen=True)
class Fit:
    status: str
    mc: float | None = None
    b_value: float | None = None
    alpha: float | None = None
    c: float | None = None
    p: float | None = None
    mu: float | None = None
    k: float | None = None
    loglik: float | None = None
    branching_proxy: float | None = None
    events_total: int = 0
    events_above_mc: int = 0


def estimate_mc(magnitudes: np.ndarray) -> float | None:
    mags = np.asarray(magnitudes, dtype=np.float64)
    mags = mags[np.isfinite(mags)]
    if mags.size < MIN_EVENTS_FOR_MC:
        return None

    minimum = math.floor(float(np.min(mags)) * 10.0) / 10.0
    maximum = math.ceil(float(np.max(mags)) * 10.0) / 10.0
    bins = np.arange(minimum, maximum + 0.1000000001, 0.1, dtype=np.float64)
    hist, edges = np.histogram(mags, bins=bins)
    index = int(np.argmax(hist))  # first maximum: required tie-break
    mc = round(float(edges[index]) + 0.2, 2)
    return min(max(mc, 1.0), 5.0)


def estimate_b(magnitudes: np.ndarray, mc: float) -> tuple[float | None, int]:
    mags = np.asarray(magnitudes, dtype=np.float64)
    selected = mags[np.isfinite(mags) & (mags >= mc)]
    n = int(selected.size)
    if n < MIN_EVENTS_ABOVE_MC:
        return None, n
    denominator = float(np.mean(selected)) - (mc - 0.05)
    if denominator <= 0.0:
        return None, n
    return math.log10(math.e) / denominator, n


def _daily_arrays(events: pd.DataFrame, decision_time_days: float, mc: float, alpha: float):
    before = events[
        (events["time_days"].to_numpy(dtype=np.float64) < decision_time_days)
        & np.isfinite(events["magnitude"].to_numpy(dtype=np.float64))
    ]
    if before.empty:
        return None

    start_day = int(np.floor(before["time_days"].min()))
    end_day = int(np.floor(np.nextafter(decision_time_days, -np.inf)))
    if end_day < start_day:
        return None

    days = end_day - start_day + 1
    y = np.zeros(days, dtype=np.float64)
    marks = np.zeros(days, dtype=np.float64)

    selected = before[before["magnitude"] >= mc]
    indices = np.floor(selected["time_days"].to_numpy(dtype=np.float64)).astype(np.int64) - start_day
    magnitudes = selected["magnitude"].to_numpy(dtype=np.float64)

    np.add.at(y, indices, 1.0)
    np.add.at(marks, indices, np.exp(alpha * (magnitudes - mc)))
    return start_day, y, marks, selected


def _history(marks: np.ndarray, c: float, p: float) -> np.ndarray:
    g = np.zeros_like(marks, dtype=np.float64)
    for t in range(marks.size):
        max_lag = min(t, TRIGGER_MEMORY_DAYS)
        if max_lag <= 0:
            continue
        lags = np.arange(1, max_lag + 1, dtype=np.float64)
        g[t] = np.sum(marks[t - lags.astype(np.int64)] * np.power(lags + c, -p))
    return g


def _nll_and_grad(z: np.ndarray, y: np.ndarray, g: np.ndarray):
    mu = float(np.exp(z[0]))
    k = float(np.exp(z[1]))
    lam = np.maximum(mu + k * g, 1e-12)
    nll = float(np.sum(lam - y * np.log(lam)))
    residual = 1.0 - y / lam
    grad = np.array(
        [
            np.sum(residual * mu),
            np.sum(residual * k * g),
        ],
        dtype=np.float64,
    )
    return nll, grad


def _fit_mu_k(y: np.ndarray, g: np.ndarray):
    mean_y = float(np.mean(y))
    mean_g = float(np.mean(g))
    x0 = np.log(np.array(
        [
            max(0.5 * mean_y, 1e-5),
            max(0.5 * mean_y / (mean_g + 1e-9), 1e-8),
        ],
        dtype=np.float64,
    ))

    result = minimize(
        fun=lambda z: _nll_and_grad(z, y, g),
        x0=x0,
        method="L-BFGS-B",
        jac=True,
        bounds=[(-15.0, 5.0), (-20.0, 5.0)],
        options={
            "maxiter": 2000,
            "ftol": 1e-12,
            "gtol": 1e-8,
            "maxls": 20,
            "maxcor": 10,
        },
    )
    return result


def fit_catalog(events: pd.DataFrame, decision_time_days: float) -> Fit:
    times = events["time_days"].to_numpy(dtype=np.float64)
    mags = events["magnitude"].to_numpy(dtype=np.float64)
    mask = (times < decision_time_days) & np.isfinite(mags)
    prior = mags[mask]

    mc = estimate_mc(prior)
    if mc is None:
        return Fit(status="INSUFFICIENT_EVIDENCE", events_total=int(prior.size))

    b_value, above = estimate_b(prior, mc)
    if b_value is None:
        return Fit(
            status="INSUFFICIENT_EVIDENCE",
            mc=mc,
            events_total=int(prior.size),
            events_above_mc=above,
        )

    best = None
    best_loglik = -math.inf

    for alpha in ALPHA_GRID:
        daily = _daily_arrays(events, decision_time_days, mc, alpha)
        if daily is None:
            continue
        _, y, marks, selected = daily

        for c in C_GRID:
            for p in P_GRID:
                g = _history(marks, c, p)
                result = _fit_mu_k(y, g)
                if not bool(result.success):
                    continue

                mu = float(np.exp(result.x[0]))
                k = float(np.exp(result.x[1]))
                loglik = float(-result.fun)

                kernel_sum = float(np.sum(
                    np.power(np.arange(1, TRIGGER_MEMORY_DAYS + 1, dtype=np.float64) + c, -p)
                ))
                mean_mark = float(np.mean(np.exp(alpha * (selected["magnitude"].to_numpy(dtype=np.float64) - mc))))
                branching = k * kernel_sum * mean_mark
                if branching >= 1.0:
                    continue

                # First candidate wins numerical tie.
                if best is None or loglik > best_loglik + TIE_TOLERANCE_LOGLIK:
                    best_loglik = loglik
                    best = Fit(
                        status="AVAILABLE",
                        mc=mc,
                        b_value=b_value,
                        alpha=alpha,
                        c=c,
                        p=p,
                        mu=mu,
                        k=k,
                        loglik=loglik,
                        branching_proxy=branching,
                        events_total=int(prior.size),
                        events_above_mc=above,
                    )

    if best is None:
        return Fit(
            status="UNKNOWN_PATTERN",
            mc=mc,
            b_value=b_value,
            events_total=int(prior.size),
            events_above_mc=above,
        )
    return best


def seven_day_probability(events: pd.DataFrame, decision_time_days: float, fit: Fit) -> float:
    if fit.status != "AVAILABLE":
        raise ValueError("Fit unavailable")

    before = events[
        (events["time_days"].to_numpy(dtype=np.float64) < decision_time_days)
        & np.isfinite(events["magnitude"].to_numpy(dtype=np.float64))
    ]
    start_day = int(np.floor(before["time_days"].min()))
    decision_day = int(np.floor(decision_time_days))
    days = decision_day - start_day + 1
    marks = np.zeros(days, dtype=np.float64)

    selected = before[before["magnitude"] >= fit.mc]
    indices = np.floor(selected["time_days"].to_numpy(dtype=np.float64)).astype(np.int64) - start_day
    np.add.at(
        marks,
        indices,
        np.exp(fit.alpha * (selected["magnitude"].to_numpy(dtype=np.float64) - fit.mc)),
    )

    g = _history(marks, fit.c, fit.p)
    daily_rate = max(float(fit.mu + fit.k * g[-1]), 1e-12)
    m6_tail = min(1.0, 10.0 ** (-fit.b_value * (6.0 - fit.mc)))
    probability = 1.0 - math.exp(-7.0 * daily_rate * m6_tail)
    return min(max(probability, 1e-9), 1.0 - 1e-9)


def evaluate_fixture(path: Path, decision_time_days: float) -> dict:
    events = pd.read_csv(path, dtype={"time_days": np.float64, "magnitude": np.float64})
    fit = fit_catalog(events, decision_time_days)
    payload = {
        "status": fit.status,
        "mc": fit.mc,
        "b_value": fit.b_value,
        "alpha": fit.alpha,
        "c": fit.c,
        "p": fit.p,
        "mu": fit.mu,
        "k": fit.k,
        "loglik": fit.loglik,
        "branching_proxy": fit.branching_proxy,
        "events_total": fit.events_total,
        "events_above_mc": fit.events_above_mc,
    }
    if fit.status == "AVAILABLE":
        payload["raw_probability"] = seven_day_probability(events, decision_time_days, fit)
    else:
        payload["raw_probability"] = None
    return payload
