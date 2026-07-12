# Changelog

All notable changes to this project are documented here. This project follows
[Semantic Versioning](https://semver.org/).

## v2.0.0 — Symbolic stack

- The stack value is a general **expression tree** in canonical
  polynomial-in-atoms form. Exact results:
  - fractions, `k√m`, rational multiples of π, and their sums/products/integer
    powers: `8 √ 2 ×` → `4√2`, `π 2 ÷` → `π/2`, `√2·√2` → `2`, `√2+√3`,
    `(1+√2)²` → `3+2√2`, `(√2+√3)(√2−√3)` → `-1`;
  - **nested radicals** (`√(1+√2)`) and **conjugate division**
    (`1/(1+√2)` → `√2−1`, `1/(√3−1)` → `(1+√3)/2`); symbolic variables.
- Transcendental functions (`sin`, `ln`), overflow, or exceeding the on-device
  arena fall back to a decimal — a result is never *wrongly* exact.
- STL-free engine over a fixed arena with a compacting garbage collector;
  overflow-checked arithmetic; device-sized pools.
- **2D rendering** on the stack: stacked fractions, radicals with a vinculum and
  raised exponents; every level shows its exact form and level 1 also shows its
  decimal approximation (`≈`).
- π is an exact constant; `→Dec` (`Ans`) forces the decimal form.
- Factorial now uses `gamma(n+1)` for large or non-integer arguments instead of
  an unbounded loop.
- All arithmetic remains overflow-checked `int64` with a decimal fallback.

## v1.0.0 — First release

- Reverse Polish Notation calculator as a NumWorks external app (`.nwa`).
- Exact rational arithmetic for `+ − × ÷` and integer powers; IEEE double for
  transcendental functions.
- Stack-oriented key map: operator keys act on the stack, RPN-unused keys become
  stack operations, `Toolbox` opens a stack menu; RAD/DEG angle mode.
- Pure, host-tested numeric core (`make test`); device build via `nwlink` and
  `arm-none-eabi`.
- GitHub Actions for CI, tagged releases and a GitHub Pages site with an
  interactive N0120 key map.
