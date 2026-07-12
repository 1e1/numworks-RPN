#pragma once

#include "input_field.h"
#include "stack.h"

/* High-level RPN commands. Digit/text entry is handled separately through
 * appendText(); these are the actions bound to operator and stack keys. */
enum class Command {
  None,
  // Entry / stack
  EnterOrDup,  // push pending input, or duplicate level 1 if input is empty
  DropOrBackspace,
  Swap,
  Over,
  Rot,
  Dup,
  RollUp,
  RollDown,
  Pick,  // uses the pending input as the level index
  Clear,
  ToDecimal,  // replace level 1 with its approximate (double) form
  // Binary operators
  Add,
  Sub,
  Mul,
  Div,
  Pow,
  // Unary functions
  Neg,
  Inv,
  Sqrt,
  Square,
  Exp,
  Ln,
  Log10,
  Sin,
  Cos,
  Tan,
  Asin,
  Acos,
  Atan,
  Factorial,
  // Constants
  PushPi,
  PushE,
  // Settings
  ToggleAngle,
};

/* Angle mode for the trigonometric functions. */
enum class AngleMode { Radian, Degree };

class Engine {
 public:
  Engine() : m_status(""), m_angleMode(AngleMode::Radian) {}

  Stack& stack() { return m_stack; }
  const Stack& stack() const { return m_stack; }
  InputField& input() { return m_input; }
  const InputField& input() const { return m_input; }
  const char* status() const { return m_status; }

  AngleMode angleMode() const { return m_angleMode; }
  void toggleAngleMode();

  bool appendText(const char* text);
  void execute(Command command);

 private:
  void commitInput();          // parse the input line and push it, then clear
  bool ensureOperands(int n);  // commit pending input, check depth, set status
  void applyUnary(Value (*f)(const Value&));
  void applyBinary(Value (*f)(const Value&, const Value&));
  void applyDouble(double (*f)(double), bool angleInput, bool angleOutput);
  void setStatus(const char* status) { m_status = status; }

  Stack m_stack;
  InputField m_input;
  const char* m_status;
  AngleMode m_angleMode;
};
