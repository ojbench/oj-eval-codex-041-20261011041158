# Submission history

| Attempt | Submission | Commit | Result |
| --- | --- | --- | --- |
| 1 | 879440 | 859cb76 | Runtime error on all tests; remove harness filesystem dependencies before retrying. |
| 2 | 879444 | 8ef8bc4 | All traces correct, 1446.0647 aggregate points; optimize utilization and search cost. |
| 3 | 879446 | 5db0f1a | All traces correct, 1506.9604 aggregate points. |

Before attempt 1, all 20 default traces, exhaustive realloc/realloc2 traces,
and a 100,000-operation ASan/UBSan randomized stress test passed locally.
