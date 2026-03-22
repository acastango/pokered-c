/* no_avx512.h — Injected before every TU via -include to work around
 * GCC 10.1.0 ICE (internal compiler error) in avx512fintrin.h.
 *
 * The crash triggers through this chain (all via SDL_cpuinfo.h -> intrin.h):
 *   intrin.h:73  -> x86intrin.h
 *   x86intrin.h  -> immintrin.h  (avx/avx2/avx512)
 *   x86intrin.h  -> fma4intrin.h, xopintrin.h  (depend on avx types)
 *
 * Defining _X86INTRIN_H_INCLUDED skips the problematic x86intrin.h entirely.
 * intrin.h then continues and includes the individual SSE/SSE2/SSE3 headers
 * directly (xmmintrin.h, emmintrin.h, pmmintrin.h …) which are safe, and
 * __rdtsc() is still declared later in intrin.h itself.
 */
#define _X86INTRIN_H_INCLUDED
