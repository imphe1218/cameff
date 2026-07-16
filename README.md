# CAMEFF P36B — Mc Histogram Equivalence Fix

P36B corrects the real-catalog magnitude-completeness parity defect identified
by P36A.

## Root cause

The former C17 code derived a bin using division by `0.1` and an epsilon.
NumPy's `arange` derives an effective binary64 step from:

```text
(start + step) - start
```

and then creates explicit edges. These behaviors differ at decimal boundaries.

## Validation

- C17 build and native unit test: PASS
- Frozen 36-case equivalence suite: 36/36 PASS
- Real-catalog Mc parity: 4/4

The complete ETAS real-catalog parity target is included. Run it locally before
creating the corrective release.

No FM-v1.4 mathematics was changed.
