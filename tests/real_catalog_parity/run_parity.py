from __future__ import annotations

import argparse
import json
import subprocess
import sys
from pathlib import Path

import pandas as pd

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
from python_oracle.etas import evaluate_fixture

ABS_K = 1e-12
REL_K = 1e-6
ABS_BRANCHING = 1e-12
REL_BRANCHING = 1e-6

def mixed_ok(a: float, b: float, abs_tol: float, rel_tol: float) -> bool:
    return abs(a - b) <= max(abs_tol, rel_tol * max(abs(a), abs(b)))

def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--repo-root", type=Path, required=True)
    args = parser.parse_args()

    cli = args.repo_root / "build" / "cameff_cli"
    manifest = pd.read_csv(HERE / "prepared_catalog_manifest.csv")
    registry = json.loads((HERE / "case_registry.json").read_text())
    results = []

    for case in registry:
        if case["status"] != "READY":
            results.append({
                "case_id": case["case_id"],
                "result": "BLOCKED",
                "reason": case["status"],
            })
            continue

        info = manifest.loc[manifest["case_id"] == case["case_id"]].iloc[0]
        path = HERE / "catalogs" / f"{case['case_id']}_prepared.csv"
        decision_days = float(info["decision_time_days"])

        py = evaluate_fixture(path, decision_days)
        proc = subprocess.run(
            [str(cli), "etas-csv", str(path), str(decision_days)],
            capture_output=True,
            text=True,
            timeout=300,
        )
        c = json.loads(proc.stdout.strip()) if proc.stdout.strip() else {"status": "NO_OUTPUT"}

        checks = {}
        checks["status"] = py["status"] == c.get("status")
        if py["status"] == "AVAILABLE" and checks["status"]:
            checks["mc"] = abs(py["mc"] - c["mc"]) <= 1e-12
            checks["b_value"] = abs(py["b_value"] - c["b_value"]) <= 1e-10
            checks["alpha"] = py["alpha"] == c["alpha"]
            checks["c"] = py["c"] == c["c"]
            checks["p"] = py["p"] == c["p"]
            checks["mu"] = mixed_ok(py["mu"], c["mu"], 0.0, 1e-6)
            checks["k"] = mixed_ok(py["k"], c["k"], ABS_K, REL_K)
            checks["loglik"] = abs(py["loglik"] - c["loglik"]) <= 1e-6
            checks["branching"] = mixed_ok(
                py["branching_proxy"], c["branching_proxy"],
                ABS_BRANCHING, REL_BRANCHING
            )
            checks["raw_probability"] = abs(
                py["raw_probability"] - c["raw_probability"]
            ) <= 1e-8

        result = "PASS" if all(checks.values()) else "FAIL"
        results.append({
            "case_id": case["case_id"],
            "result": result,
            "checks": checks,
            "python": py,
            "c17": c,
        })

    ready = [x for x in results if x["result"] in {"PASS", "FAIL"}]
    blocked = [x for x in results if x["result"] == "BLOCKED"]
    summary = {
        "ready_cases": len(ready),
        "passed": sum(x["result"] == "PASS" for x in ready),
        "failed": sum(x["result"] == "FAIL" for x in ready),
        "blocked": len(blocked),
        "overall": (
            "PASS_WITH_BLOCKER"
            if ready and all(x["result"] == "PASS" for x in ready) and blocked
            else "PASS"
            if ready and all(x["result"] == "PASS" for x in ready)
            else "FAIL"
        ),
        "results": results,
    }
    out = HERE / "results" / "parity_results.json"
    out.write_text(json.dumps(summary, indent=2) + "\n")
    print(json.dumps(summary, indent=2))
    return 0 if summary["failed"] == 0 else 1

if __name__ == "__main__":
    raise SystemExit(main())
