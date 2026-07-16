from __future__ import annotations

import csv
import json
import math
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
CLI = ROOT / "build" / "cameff_cli"
CATALOGS = ROOT / "tests" / "fixtures" / "catalogs"
ORACLE = ROOT / "tests" / "fixtures" / "oracle" / "P34J_P34F_NORMATIVE_ETAS_ORACLE.csv"

ABS_K = 1e-12
REL_K = 1e-6
ABS_BRANCHING = 1e-12
REL_BRANCHING = 1e-6

def mixed_ok(a: float, b: float, abs_tol: float, rel_tol: float) -> bool:
    return abs(a - b) <= max(abs_tol, rel_tol * max(abs(a), abs(b)))

def main() -> int:
    rows = list(csv.DictReader(ORACLE.open(newline="")))
    passed = 0
    failures = []

    for row in rows:
        case_id = row["case_id"]
        proc = subprocess.run(
            [
                str(CLI),
                "etas-csv",
                str(CATALOGS / f"{case_id}.csv"),
                row["decision_time_days"],
            ],
            check=False,
            capture_output=True,
            text=True,
            timeout=180,
        )
        payload = json.loads(proc.stdout.strip()) if proc.stdout.strip() else {"status": "NO_OUTPUT"}

        checks = {}
        if row["status"] != "AVAILABLE":
            checks["status"] = payload.get("status") != "AVAILABLE"
        else:
            checks["status"] = payload.get("status") == "AVAILABLE"
            if checks["status"]:
                checks["mc"] = abs(float(row["mc"]) - float(payload["mc"])) <= 1e-12
                checks["b"] = abs(float(row["b_value"]) - float(payload["b_value"])) <= 1e-10
                checks["alpha"] = float(row["alpha"]) == float(payload["alpha"])
                checks["c"] = float(row["c"]) == float(payload["c"])
                checks["p"] = float(row["p"]) == float(payload["p"])
                checks["mu"] = mixed_ok(float(row["mu"]), float(payload["mu"]), 0.0, 1e-6)
                checks["k"] = mixed_ok(float(row["k"]), float(payload["k"]), ABS_K, REL_K)
                checks["loglik"] = abs(float(row["loglik"]) - float(payload["loglik"])) <= 1e-6
                checks["branching"] = mixed_ok(
                    float(row["branching_proxy"]),
                    float(payload["branching_proxy"]),
                    ABS_BRANCHING,
                    REL_BRANCHING,
                )
                checks["raw_probability"] = abs(
                    float(row["raw_probability"]) - float(payload["raw_probability"])
                ) <= 1e-8

        if all(checks.values()):
            passed += 1
        else:
            failures.append({"case_id": case_id, "checks": checks, "payload": payload})

    summary = {
        "cases": len(rows),
        "passed": passed,
        "failed": len(failures),
        "status": "PASS" if not failures else "FAIL",
        "failures": failures,
    }
    print(json.dumps(summary, indent=2))
    return 0 if not failures else 1

if __name__ == "__main__":
    raise SystemExit(main())
