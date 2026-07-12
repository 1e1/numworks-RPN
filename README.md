<div align="center">

<img src="src/icon.png" alt="RPN app icon" width="64" height="64">

# RPN for NumWorks

[![GitHub release](https://img.shields.io/github/v/release/1e1/numworks-RPN?style=flat-square)](https://github.com/1e1/numworks-RPN/releases)
[![GitHub commit activity](https://img.shields.io/github/commit-activity/m/1e1/numworks-RPN?style=flat-square&color=brightgreen)](https://github.com/1e1/numworks-RPN/commits/main)
[![License](https://img.shields.io/github/license/1e1/numworks-RPN?style=flat-square)](https://github.com/1e1/numworks-RPN/blob/main/LICENSE)

### *No parentheses, no `=`.*

</div>

A **Reverse Polish Notation** (RPN) calculator as an external app for the
[NumWorks](https://www.numworks.com) graphing calculator.

No parentheses, no `=`: you push numbers onto a stack and operators act on it.
Results stay **symbolic and exact** where they can — fractions (`1/3 + 1/6` →
`1/2`), radicals (`8 √ 2 ×` → `4√2`), and rational multiples of π (`π 2 ÷` →
`π/2`) — and fall back to a decimal approximation otherwise. The familiar
NumWorks feel, RPN-style.

➡️ **Project page & key map:** <https://1e1.github.io/numworks-RPN/>

> **Scope.** NumWorks external apps run in a sandbox and cannot call the
> built-in Poincaré engine, so this app ships its own symbolic core: a general
> **expression tree** kept in a canonical polynomial-in-atoms form. It keeps
> exact fractions, `k√m`, rational multiples of π, their sums/products/integer
> powers, **nested radicals** (`√(1+√2)`) and **conjugate division**
> (`1/(1+√2)` → `√2−1`), and it carries symbolic variables. Transcendental
> functions (`sin`, `ln`), integer overflow, or exceeding the on-device arena
> fall back to a decimal — so a result is never *wrongly* exact. The engine is
> STL-free with a compacting garbage collector over a fixed arena.

## Install

1. Download the latest `rpn-vX.Y.Z.nwa` from the
   [Releases](https://github.com/1e1/numworks-RPN/releases) page.
2. Open the [NumWorks app uploader](https://my.numworks.com/apps), plug in your
   calculator on the *connected* screen, and upload the file.

The app then appears at the end of the home menu. (Resets remove external apps.)

## How RPN works here

You type a number and press **EXE** to push it. Operators consume the top of the
stack. Subtraction and division are `level2 (op) level1`:

```
3 EXE 6 ÷        → 1/2          exact fraction
2 EXE √          → √2           exact radical
8 EXE √ 2 ×      → 4√2          simplified
2 √ 3 √ +        → √2 + √3      sums stay exact
π EXE 2 ÷        → π/2          rational multiple of π
5 EXE 4 EXE −    → 1            (5 − 4)
5 !              → 120          exact factorial
```

On the calculator these render in **2D** — stacked fractions, `√` with a
vinculum, raised exponents — and level 1 also shows its decimal (`≈`). Press
`Ans` (→Dec) to force the decimal approximation of the top level.

If you enter operands in the wrong order, press **( = SWAP** to exchange the top
two levels.

## Key map

🎹 **Interactive keyboard map:** <https://1e1.github.io/numworks-RPN/keymap.html> —
click a key, a table row, or a category; unused keys are highlighted.

Operator and function keys keep their printed symbol but **apply to the stack**
instead of inserting text. Keys with no RPN meaning are repurposed for stack ops.

| Key | Action |
|---|---|
| `0–9` `.` `EE` | type a number (`EE` → exponent `e`) |
| `EXE` / `OK` | **ENTER**: push input, or duplicate level 1 if empty |
| `backspace` | delete a character, or **DROP** level 1 if input is empty |
| `shift+backspace` (clear) | **CLEAR** the whole stack |
| `+` `−` `×` `÷` | add / subtract / multiply / divide |
| `^` | power `yˣ` |
| `x²` `√` | square / square root |
| `eˣ` `ln` `log` | exp / natural log / log₁₀ |
| `sin` `cos` `tan` | trigonometry (see angle mode) |
| `shift+sin/cos/tan` | arcsin / arccos / arctan |
| `π` | push π |
| `(` | **SWAP** level 1 and level 2 |
| `)` | **OVER** (copy level 2 to the top) |
| `shift+−` (space) | **NEG** (change sign) |
| `shift+÷` (`>`) | **INV** (`1/x`) |
| `Ans` | **→Dec** (force the decimal approximation of level 1) |
| `shift+↑` / `shift+↓` | **ROLL** the stack up / down |
| `shift+.` (`!`) | **factorial** |
| `Toolbox` | open the **stack menu** (below) |

### Toolbox stack menu

Lower-frequency operations, navigated with `↑`/`↓` and `OK`, closed with
`Toolbox`/`back`: SWAP, OVER, ROT, ROLL up/down, DUP, PICK (input = level),
`1/x`, →Dec, CLEAR, and the **RAD/DEG angle toggle**.

## Build from source

Requires `arm-none-eabi-gcc` and Node.js (for [`nwlink`](https://www.npmjs.com/package/nwlink)).

```shell
make            # build output/rpn.nwa
make install    # build and upload to a plugged-in calculator
make test       # build and run the host-side engine unit tests
```

The numeric core (`src/value`, `src/stack`, `src/rpn`, `src/input_field`) is pure
C++ with no calculator dependency, so `make test` compiles and runs it on your
host machine.

## Try it in a web simulator (Docker)

Run the app in a browser via the Epsilon web simulator — only Docker is needed
(the emscripten/node toolchain lives inside the container). You need an
[Epsilon](https://github.com/numworks/epsilon) checkout next to this repo (its
folder must contain Epsilon's `Makefile`).

```shell
docker/run.sh                      # or: EPSILON_DIR=/path/to/epsilon docker/run.sh
```

Then open <http://localhost:8000/epsilon.html?nwb=/rpn.nwb>. The first run builds
the simulator (long; cached in a Docker volume afterwards). You can also build
the web app alone with `make PLATFORM=web` (needs `emcc`).

> **Note.** Loading the app in the *web* simulator is not wired up yet (our side
> module needs libc/soft-float symbols Epsilon's size-deduped main module does
> not export). To see a real-font render, use the native capture below.

### Real-font screenshot (Docker)

```shell
make screenshot        # or: docker/screenshot.sh
```

Builds the Epsilon **Linux** simulator, renders the app headless with a sample
stack, and writes `capture/render.png` — the actual NumWorks font, no browser
needed.

## Project layout

```
src/
  value.{h,cpp}        the symbolic engine (arena tree + GC, exact/decimal)
  layout.{h,cpp}       backend-agnostic 2D math layout (Canvas + Layout tree)
  screen.{h,cpp}       full-screen renderer (title/stack/status/input/menu)
  view.{h,cpp}         EADK adapter: implements Canvas on the device display
  rpn.{h,cpp}          the command engine (Enter, operators, functions…)
  stack.{h,cpp}        fixed-capacity RPN stack of Value
  input_field.{h,cpp}  the typed input line
  keymap.h             key event → command mapping
  menu.h               Toolbox stack-menu entries
  utf8.h               UTF-8 glyph counter (shared by layout + screen)
  eadkpp.h             C++ wrapper over the External App Dev Kit
  main.cpp             event loop + metadata
tests/                 host unit tests + PNG render harnesses
docs/                  GitHub Pages landing page
```

For a fuller tour of the architecture and the contribution workflow, see
[CONTRIBUTING.md](CONTRIBUTING.md).

## Roadmap

The next version aims for a **symbolic stack with Poincaré-style result
presentation** (2D layouts, exact + approximate). See
[docs/ROADMAP.md](docs/ROADMAP.md).

## License

See [LICENSE](LICENSE).
