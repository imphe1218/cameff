# Milestone 1 acceptance criteria

1. Builds as strict ISO C17 with GCC and warnings-as-errors.
2. Requires a cutoff timestamp, preventing future-event leakage.
3. Selects only events inside the configured time-space window.
4. Produces bounded signal values and per-signal confidence in [0,1].
5. Represents missing/weak evidence explicitly rather than inventing certainty.
6. Treats fault-map confidence as an input signal, allowing regions with incomplete mapped faults to remain uncertain.
7. Outputs an evidence level, not a calibrated earthquake probability.
8. Passes deterministic unit tests.

The next milestone should introduce the earthquake-situation classifier and the expert interface without changing these public data contracts unnecessarily.
