from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path

import pandas as pd

from .etas import evaluate_fixture


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--catalog-dir", type=Path, required=True)
    parser.add_argument("--case-index", type=Path, required=True)
    parser.add_argument("--contract", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()

    contract = json.loads(args.contract.read_text())
    index = pd.read_csv(args.case_index)

    rows = []
    for row in index.to_dict(orient="records"):
        path = args.catalog_dir / f"{row['case_id']}.csv"
        result = evaluate_fixture(path, float(row["decision_time_days"]))
        rows.append({
            "case_id": row["case_id"],
            "mode": row["mode"],
            "days": int(row["days"]),
            "decision_time_days": float(row["decision_time_days"]),
            "catalog_sha256": sha256(path),
            "contract_id": contract["contract_id"],
            **result,
        })

    frame = pd.DataFrame(rows).sort_values(["case_id"], kind="stable")
    frame.to_csv(args.output, index=False, lineterminator="\n")
    print(json.dumps({
        "status": "PASS",
        "rows": len(frame),
        "output_sha256": sha256(args.output),
    }, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
