# Isolated Vita mspace diagnostic

These C sources are mirrored from TASM's tested diagnostic. They create their
own1MiB USER_RW arena, exercise Sony mspace serially and concurrently, check
allocation ranges, live overlap and payload canaries, then join every worker.
They do not use the game's heap for the tested allocations.

Integration:

1. Copy `heap_selftest.c` and `heap_selftest.h` into the port's `source/utils`.
2. Add the C source to the build and adapt the `utils/logger.h` dependency to
   the port's logger if needed. Compile with `DEBUG_SOLOADER` for the actual test.
3. Invoke `tasm_heap_selftest()` explicitly from an idle menu after logging and
   save backups work. TASM's dev harness rejects the command outside MainMenu.
   Never add this stress test to normal startup or the gameplay loop.
4. Exclude the diagnostic from FPS comparisons. Check the `done` result and
   `joined` fields; command acknowledgement alone is insufficient.
5. Close the app after a corruption/timeout result. The diagnostic retains its
   private arena/static context in those cases; do not force normal destruction
   of possibly corrupt allocator metadata or memory still used by a worker.

On TASM166, priority0xA0 and fixed affinities to user cores0/1/2 worked:
4096serial allocations passed in93824us;16384concurrent allocations passed in
602625us. All workers joined, with no overlap or changed canaries.165's initial
priority0x100 failed to create a worker, so no allocator test ran in that build.
Use actual API errors when adapting to another process; these measurements
and priorities are target evidence, not universal performance guarantees.

A pass covers the exercised sizes, pattern and concurrency. It does not prove
all allocator sequences safe or validate native object ownership/lifetime.
The failure retention branches were reviewed; corruption was not deliberately
introduced to claim fault-injection coverage.
