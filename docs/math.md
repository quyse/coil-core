# Coil Core Math

## Basic math `coil_core_math`

Provides vectors, matrices, quaternions, and usual operations on them. Supports both aligned (for performance) and non-aligned (for serialization) values. Currently there's no SIMD support, so the only hope is automatic vectorization by compiler.

## Deterministic math `coil_core_math_determ`

Additionally provides deterministic floating-point scalars and vectors, suitable for e.g. deterministic network multiplayer. To avoid horrors of FPU determinism, the implementation uses platform-specific SIMD intrinsics (and currently is only implemented for x86/x64 with SSE2). Transcendental functions are approximated with polynomials.

## Logic `coil_core_logic`

Math useful in gameplay programming.
