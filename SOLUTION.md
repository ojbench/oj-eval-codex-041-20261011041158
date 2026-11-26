# Allocator implementation

`mm.c` implements the allocator using the course's `mem_sbrk` interface.
Its segregated free lists use heap-relative 32-bit links, 4-byte headers,
and footers only in free blocks. Coalescing and removal are constant time;
allocation searches size classes for a suitable block. Reallocation first
tries shrinking, merging with a free successor, or extending the heap end.
The heap checker validates physical blocks and free-list membership.

The harness, headers, configuration, and traces come from the official
[OS2023 practice 2-2 handout](https://github.com/ACMClassCourses/OS2023-Practices/tree/main/practice-2/practice_2-2).
The allocator is an original implementation. The harness is unchanged;
compiler warning suppressions apply only to its legacy driver source.

Build `code` with `make`. This enables the handout's `OJ` mode, which reads
one trace from standard input. Build the command-line harness with
`make mdriver`, then run `./mdriver` for the default suite or
`./mdriver -D -c traces/corners.rep` for exhaustive correctness checking.
