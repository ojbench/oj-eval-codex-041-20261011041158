# Allocator implementation

`mm.c` implements the allocator using the course's `mem_sbrk` interface.
Its segregated free lists use heap-relative 32-bit links, 4-byte headers,
and footers only in free blocks. Coalescing and removal are constant time;
allocation searches size classes for a suitable block. Reallocation first
tries shrinking, merging with a free successor, or extending the heap end.
The heap checker validates physical blocks and free-list membership.
Small size classes are exact; larger classes have two bins per power of two.
Allocation takes the first fitting block and grows the heap only as needed,
without inserting newly allocated storage into a free list.

The harness, headers, configuration, and traces come from the official
[OS2023 practice 2-2 handout](https://github.com/ACMClassCourses/OS2023-Practices/tree/main/practice-2/practice_2-2).
The allocator is an original implementation. The driver is unchanged;
compiler warning suppressions apply only to its legacy source. The harness
uses its supported gettimeofday timer and anonymous heap mapping so that
neither `/proc/cpuinfo` nor `/dev/zero` is required in the judge sandbox.

Build `code` with `make`. This enables the handout's `OJ` mode, which reads
one trace from standard input. Build the command-line harness with
`make mdriver`, then run `./mdriver` for the default suite or
`./mdriver -D -c traces/corners.rep` for exhaustive correctness checking.
Run `make test` for a deterministic 100,000-operation randomized stress test
with AddressSanitizer, UndefinedBehaviorSanitizer, and heap checks after
every operation. Submission results are recorded in `SUBMISSIONS.md`.
