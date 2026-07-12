# Roadmap — v2: symbolic stack with Poincaré-style presentation

v1.0.0 is a numeric RPN calculator: the stack holds *numbers* (exact rationals
or doubles) and every operator computes eagerly. v2 changes the nature of the
stack.

## Vision

The stack holds **expression terms**, not just numbers — a bit like lambda
calculus: operators **build a term**, and a **reduction** step normalizes it on
demand rather than computing eagerly. Results are then **presented like
Poincaré**: real 2D math layout (stacked fractions, radicals, raised exponents)
with the **exact form and its decimal approximation** shown together, exactly
the feel of the standard NumWorks app.

```
  2 ENTER 8 √ ×        level 1:  2·√8   ── exact, unreduced term
  reduce               level 1:  4·√2   ── normalized
  →Num                 level 1:  5.656854
```

## The core constraint (unchanged from v1)

An external app cannot call Poincaré (EADK sandbox). Two routes to the vision:

| | Route A — native fork | Route B — self-contained engine |
|---|---|---|
| Where | RPL mode inside Epsilon (the `epsilon` fork) | still a portable `.nwa` |
| Math | reuse Poincaré `Expression` + simplification + `Layout` | build a small term engine + 2D renderer ourselves |
| Fidelity | full (real Poincaré look & CAS) | good, bounded |
| Cost | fork maintenance, upstream sync, bigger build | more app code, but stays portable |

**Recommendation:** prototype **Route B, Phase 1** first — it extends the
existing host-tested pure core and keeps the app a single `.nwa`. Keep Route A
as the escape hatch if you later want true Poincaré fidelity.

## Architecture

### 1. Term model (`expr`)
Replace the flat `Value` on the stack with an expression tree:

- Leaves: `Integer`, `Rational`, `Constant` (π, e, i), `Symbol` (future).
- Nodes: n-ary `Add`, n-ary `Mul`, `Pow`, `Neg`, `Func` (sin, ln, …).
- N-ary `Add`/`Mul` with **sorted, canonicalized** operands so equal terms
  collapse (`√2 + √2 → 2·√2`).

The stack becomes a stack of `Expr` (kept small: arena/pool allocation, no heap
churn — important on the device).

### 2. Reduction (the "lambda" step)
Operators are **constructors**: `×` just builds `Mul(a, b)`. A `reduce()` pass
normalizes to a canonical form:

- rational arithmetic (reuse v1's overflow-checked `Value`),
- constant folding, like-term collection, `√` extraction of square factors,
  power rules (`xᵃ·xᵇ → xᵃ⁺ᵇ`), sign normalization.

Two outputs from any term: **exact** (the normalized tree) and **approximate**
(numeric eval to double). Reduction runs on `ENTER` / on demand — building a
term then reducing it mirrors *build → β-reduce → normal form*.

### 3. 2D layout renderer (`layout`) — the Poincaré look
The biggest new piece in Route B. A layout tree of boxes with a **measure**
(width, height, baseline) then **draw** pass over the EADK framebuffer:

- `HBox` / `VBox`, `FractionLayout` (numerator over bar over denominator),
- `RadicalLayout` (√ with vinculum), `SuperscriptLayout` (raised exponent).

Reuses the two EADK font sizes; measurement drives right-alignment on the stack.

### 4. Presentation
Each stack level renders its **exact 2D layout**; a secondary line (or a
per-level toggle) shows the **decimal approximation**, like the standard app's
exact/approx duality. `→Num` forces the approximate view.

## Phasing

1. **Term engine + reducer** for rationals, `√`, π, basic simplification — pure
   C++, unit-tested on host (extends the current `make test` harness). 1D text
   output first (`4·√2`).
2. **2D layout renderer**: fractions, powers, radicals over the framebuffer.
3. **Exact / approximate dual presentation** + `→Num`.
4. *(optional)* evaluate a **Route A** native-fork port if fidelity or scope
   demands the real Poincaré engine.

## Risks & bounds

- **Binary size** — v1 is already ~500 KB; a layout engine adds more. Budget it;
  consider dropping `_printf_float` via a hand-rolled number formatter.
- **Mini-CAS scope creep** — explicitly *not* a general algebra system. Bound it
  to: exact rationals, `√`, π/e, like-term collection, power/product rules.
- **Device memory** — pool-allocate terms; cap expression depth/size and report
  when a level exceeds it rather than failing silently.

## What carries over from v1

The pure, host-tested core (`value`, `stack`, `input_field`, `rpn`) is the
foundation: `Value` becomes the numeric leaf of the term model, `Stack` becomes
a stack of `Expr`, and the `make test` harness extends to cover reduction rules.
