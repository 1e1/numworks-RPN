#pragma once

#include "eadkpp.h"
#include "rpn.h"

/* Maps the calculator's key events (modifiers already resolved by the OS, e.g.
 * shift+minus arrives as Space, shift+backspace as Clear) to RPN commands.
 *
 * Design notes:
 *   - Operator/function keys keep their printed symbol but *apply* to the stack
 *     instead of inserting text.
 *   - Keys that are meaningless in RPN are repurposed for stack ops:
 *       (  -> SWAP        )  -> OVER
 *       shift+minus (Space) -> NEG      shift+division (Greater) -> INV
 *       Ans -> ->Decimal
 *   - shift+up / shift+down roll the stack.
 */
namespace Keymap {

using EADK::Keyboard::Event;

// Returns the ASCII fragment to append to the input line, or nullptr if the
// event is not a text/number key.
inline const char* textForEvent(Event event) {
  switch (event) {
    case Event::Zero: return "0";
    case Event::One: return "1";
    case Event::Two: return "2";
    case Event::Three: return "3";
    case Event::Four: return "4";
    case Event::Five: return "5";
    case Event::Six: return "6";
    case Event::Seven: return "7";
    case Event::Eight: return "8";
    case Event::Nine: return "9";
    case Event::Dot: return ".";
    case Event::Ee: return "e";  // ASCII so Value::parse understands it
    default: return nullptr;
  }
}

// Returns the RPN command bound to the event, or Command::None.
inline Command commandForEvent(Event event) {
  switch (event) {
    // Entry / stack
    case Event::Exe:
    case Event::Ok: return Command::EnterOrDup;
    case Event::Backspace: return Command::DropOrBackspace;
    case Event::Clear: return Command::Clear;
    case Event::Left_parenthesis: return Command::Swap;
    case Event::Right_parenthesis: return Command::Over;
    case Event::Space: return Command::Neg;      // shift + minus
    case Event::Greater: return Command::Inv;    // shift + division
    case Event::Ans: return Command::ToDecimal;
    case Event::Shift_up: return Command::RollUp;
    case Event::Shift_down: return Command::RollDown;

    // Binary operators
    case Event::Plus: return Command::Add;
    case Event::Minus: return Command::Sub;
    case Event::Multiplication: return Command::Mul;
    case Event::Division: return Command::Div;
    case Event::Power: return Command::Pow;

    // Unary functions
    case Event::Sqrt: return Command::Sqrt;
    case Event::Square: return Command::Square;
    case Event::Exp: return Command::Exp;
    case Event::Ln: return Command::Ln;
    case Event::Log: return Command::Log10;
    case Event::Sine: return Command::Sin;
    case Event::Cosine: return Command::Cos;
    case Event::Tangent: return Command::Tan;
    case Event::Arcsine: return Command::Asin;
    case Event::Arccosine: return Command::Acos;
    case Event::Arctangent: return Command::Atan;
    case Event::Exclamation: return Command::Factorial;  // alpha + dot

    // Constants
    case Event::Pi: return Command::PushPi;

    default: return Command::None;
  }
}

}  // namespace Keymap
