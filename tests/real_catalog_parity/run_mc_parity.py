from __future__ import annotations

import json
import subprocess
import sys
from pathlib import Path

import pandas as pd

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
from python_oracle.etas import estimate_mc

ROOT = HERE.parents[1]
CLI = ROOT / "build" / "cameff_cli"

def main() -> int:
    manifest = pd.read_csv(HERE / "prepared_catalog_manifest.csv")
    results = []

    for row in manifest.to_dict(orient="records"):
        path = HERE / "catalogs" / f"{row['case_id']}_prepared.csv"
        frame = pd.read_csv(path)
        expected = float(estimate_mc(frame["magnitude"].to_numpy()))
        proc = subprocess.run(
            [str(CLI), "mc-csv", str(path), str(row["decision_time_days"])],
            capture_output=True,
            text=True,
            timeout=60,
        )
        payload = json.loads(proc.stdout)
        actual = float(payload["mc"])
        passed = abs(expected - actual) <= 1e-12
        results.append({
            "case_id": row["case_id"],
            "python_mc": expected,
            "c17_mc": actual,
            "passed": passed,
        })

    summary = {
        "cases": len(results),
        "passed": sum(x["passed"] for x in results),
        "failed": sum(not x["passed"] for x in results),
        "status": "PASS" if all(x["passed"] for x in results) else "FAIL",
        "results": results,
    }
    print(json.dumps(summary, indent=2))
    return 0 if summary["failed"] == 0 else 1

if __name__ == "__main__":
    raise SystemExit(main())
