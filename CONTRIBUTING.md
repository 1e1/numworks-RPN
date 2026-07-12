# Developer & contributor guide

A map of the codebase and the workflows, so anyone (including future-you) can
pick the project back up quickly.

## What it is

An RPN calculator shipped as a NumWorks **external app** (`.nwa`). External apps
run in a sandbox and **cannot call the built-in Poincaré engine**, so the app
carries its own symbolic engine and its own 2D math renderer.

## Repository layout

```
src/
  value.{h,cpp}        the symbolic engine (see “Engine” below)
  layout.{h,cpp}       backend-agnostic 2D math layout (Canvas + Layout tree)
  screen.{h,cpp}       full-screen renderer (title/stack/status/input/menu)
  view.{h,cpp}         EADK adapter: implements Canvas on the device display
  rpn.{h,cpp}          Engine + Command enum (Enter, operators, functions…)
  stack.{h,cpp}        fixed-capacity RPN stack of Value
  input_field.{h,cpp}  the typed input line
  keymap.h             key event → Command mapping
  menu.h               Toolbox stack-menu entries
  utf8.h               UTF-8 glyph counter (shared by layout + screen)
  eadkpp.h             C++ wrapper over the External App Dev Kit
  main.cpp             event loop + app metadata
tests/                 host unit tests + PNG render harnesses
docker/                web simulator (run.sh) + real-font screenshot (screenshot.sh)
docs/                  GitHub Pages: landing page, interactive key map, ROADMAP
.github/               CI / Release (rc + retention prune) / Pages workflows
```

## Build, test, run

Requires Node.js (for [`nwlink`](https://www.npmjs.com/package/nwlink)); the
device build also needs `arm-none-eabi-gcc`.

```shell
make              # device app  -> output/rpn.nwa
make install      # build + upload to a plugged-in calculator
make test         # host unit tests for the engine (no calculator needed)
make PLATFORM=web # web app .nwb (needs emcc)
make screenshot   # real-font render via the Epsilon Linux simulator (Docker)
docker/run.sh     # run the app in the Epsilon web simulator (Docker)
```

The engine and layout are pure C++ with no calculator dependency, so `make test`
compiles and runs them on the host.

## Data flow

```
keyboard event ─▶ keymap.h ─▶ Engine::execute (rpn.cpp) ─▶ Stack<Value>
                                                              │
                              Screen::render (screen.cpp) ◀───┘
                              via a Canvas: View (EADK) on device,
                              a recording canvas → PNG on host
```

## Engine (`src/value`)

A `Value` is **exact-symbolic**, an **inexact double**, or **undefined**.

The exact form is a canonical *polynomial in atoms*: `Σ coeff · Π atom^e`, where
an atom is `π`, a variable, `√(sub-expression)`, or `1/(sub-expression)`. This
represents fractions, `k√m`, rational multiples of π and their
sums/products/integer powers, **nested radicals** (`√(1+√2)`) and **conjugate
division** (`1/(1+√2)` → `√2−1`). Anything outside it (transcendental functions,
overflow, or exceeding the arena) falls back to a double — **never a wrongly
exact result**.

Implementation notes:
- **No STL / no exceptions.** Nodes (`Poly`/`Term`/`Factor`/`Base`) live in
  fixed, index-based pools — a shared arena.
- **Garbage collection.** `Value::collect(roots, n)` is a copying collector that
  keeps only what the roots reach and compacts the arena. `Engine::execute`
  calls it (with the live stack as roots) when `Value::arenaNearFull()`.
- **Overflow safety.** Rational arithmetic is overflow-checked and every arena
  allocation is guarded; on overflow the op degrades to a double (`gFull`).
- **Device sizing.** Pool sizes shrink under `#if PLATFORM_DEVICE`; the host and
  simulator use large pools. The simulator has ample RAM, so it does **not**
  prove the device RAM budget — verify complex-expression behaviour on real
  hardware.

## Layout / rendering

`layout.{h,cpp}` is a small TeX-like box model (`text`, `hbox`, `frac`,
`radical`, `superscript`) drawn against an abstract `Canvas`. `Value::buildLayout`
walks the expression tree into it. `screen.cpp` composes the whole screen (also
Canvas-based), so the **same** rendering code drives the device (`view.cpp` →
EADK) and the host PNG harnesses (`tests/render_*.cpp` → `tests/paint.py`).

## How to add things

- **A new function/operator key:** add a `Command` (rpn.h), handle it in
  `Engine::execute` (rpn.cpp) — usually via `applyDouble`/`applyUnary`/
  `applyBinary` — map the key in `keymap.h`, and add a Toolbox entry in `menu.h`
  if it has no dedicated key.
- **A new exact simplification:** extend `simplifyTmp` / `ValueImpl` in
  `value.cpp`, then add cases to `tests/test_engine.cpp`.
- **Always** re-run `make test`, a device compile, and `make screenshot`.

## Verifying a change

1. `make test` — host unit tests.
2. Device compile — `make` with `arm-none-eabi-gcc` (CI does this on every push).
3. `make screenshot` — eyeball the real-font render in `capture/render.png`.
4. For anything touching device memory, flash `output/rpn.nwa` to a real
   calculator; the simulator cannot validate the RAM budget.

## Release process

Tag `vX.Y.Z` (or `vX.Y.Z-rc[.N]` for a pre-release) and push it:
- **CI** builds + tests + the `.nwa`;
- **Release** builds the `.nwa`, publishes the GitHub release, and applies the
  retention policy (`.github/scripts/prune-releases.sh` — see its header; tested
  by `test_prune_releases.py`);
- **Pages** deploys `docs/`.

## Roadmap

Next steps and the native-Epsilon escape hatch: [docs/ROADMAP.md](docs/ROADMAP.md).
