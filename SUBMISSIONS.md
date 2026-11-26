# Submission history

| Attempt | Submission | Commit | Result |
| --- | --- | --- | --- |
| 1 | 879440 | 859cb76 | Runtime error on all tests; remove harness filesystem dependencies before retrying. |
| 2 | 879444 | 8ef8bc4 | All traces correct, 1446.0647 aggregate points; optimize utilization and search cost. |
| 3 | 879446 | 5db0f1a | All traces correct, 1506.9604 aggregate points. |
| 4 | 879452 | 940e655 | All traces correct, 1518.9261 aggregate points (best). |

Before attempt 1, all 20 default traces, exhaustive realloc/realloc2 traces,
and a 100,000-operation ASan/UBSan randomized stress test passed locally.

The final allocator also passes the randomized sanitizer test. Its local
default-suite aggregate performance index reaches 100; OJ scores individual
traces, so the OJ aggregate differs. The judge reports `wrong_answer` for
partial performance credit even when the driver reports `valid: yes`.
The four zero-weight correctness traces (`corners`, `malloc-free`, `malloc`,
`short2`) receive a zero performance index from the official driver.
Its scoring formula and trace weights have not been changed.

Four of five allowed submissions were used. No repository modifications or
pushes occurred while a submission was awaiting its result.
