# RPN for NumWorks

A **Reverse Polish Notation** (RPN) calculator as an external app for the
[NumWorks](https://www.numworks.com) graphing calculator.

No parentheses, no `=`: you push numbers onto a stack and operators act on it.
Arithmetic keeps **exact fractions** (`1/3 + 1/6` stays `1/2`), and functions
fall back to decimal approximations — the familiar NumWorks feel, RPN-style.

➡️ **Project page & key map:** <https://1e1.github.io/numworks-RPN/>

> **Scope.** NumWorks external apps run in a sandbox and cannot call the
> built-in Poincaré engine, so this app ships its own numeric core: exact
> rational arithmetic for `+ − × ÷` and integer powers, IEEE double for
> transcendental functions. Full symbolic simplification would require a native
> build of Epsilon rather than an external app.

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
2 EXE √          → 1.414214     approximation
5 EXE 4 EXE −    → 1            (5 − 4)
π EXE 2 ×        → 6.283185
5 !              → 120          exact factorial
```

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

## Project layout

```
src/
  value.{h,cpp}        exact-rational-or-double number type
  stack.{h,cpp}        fixed-capacity RPN stack
  input_field.{h,cpp}  the typed input line
  rpn.{h,cpp}          the command engine (Enter, operators, functions…)
  keymap.h             key event → command mapping
  menu.h               Toolbox stack-menu entries
  view.{h,cpp}         screen rendering (EADK display)
  main.cpp             event loop + metadata
  eadkpp.h             C++ wrapper over the External App Dev Kit
tests/                 host unit tests for the numeric core
docs/                  GitHub Pages landing page
```

## Roadmap

The next version aims for a **symbolic stack with Poincaré-style result
presentation** (2D layouts, exact + approximate). See
[docs/ROADMAP.md](docs/ROADMAP.md).

## License

See [LICENSE](LICENSE).
