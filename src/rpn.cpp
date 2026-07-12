#include "rpn.h"

extern "C" {
#include <math.h>
}

namespace {
constexpr double kPi = 3.14159265358979323846;
constexpr double kE = 2.71828182845904523536;

double toRadians(double x) { return x * kPi / 180.0; }
double toDegrees(double x) { return x * 180.0 / kPi; }

double factorialDouble(double x) {
  // Only defined here for non-negative integers; caller validates.
  double result = 1.0;
  for (int i = 2; i <= (int)(x + 0.5); i++) result *= i;
  return result;
}
}  // namespace

void Engine::toggleAngleMode() {
  m_angleMode =
      m_angleMode == AngleMode::Radian ? AngleMode::Degree : AngleMode::Radian;
  setStatus(m_angleMode == AngleMode::Radian ? "Radians" : "Degrees");
}

bool Engine::appendText(const char* text) {
  setStatus("");
  return m_input.append(text);
}

void Engine::commitInput() {
  if (m_input.isEmpty()) return;
  Value v = Value::parse(m_input.text());
  m_input.clear();
  if (v.isUndefined()) {
    setStatus("Invalid input");
    return;
  }
  if (!m_stack.push(v)) setStatus("Stack full");
}

bool Engine::ensureOperands(int n) {
  commitInput();
  if (m_stack.depth() < n) {
    setStatus("Too few arguments");
    return false;
  }
  return true;
}

void Engine::applyUnary(Value (*f)(const Value&)) {
  if (!ensureOperands(1)) return;
  Value a = m_stack.pop();
  m_stack.push(f(a));
}

void Engine::applyBinary(Value (*f)(const Value&, const Value&)) {
  if (!ensureOperands(2)) return;
  Value b = m_stack.pop();
  Value a = m_stack.pop();
  m_stack.push(f(a, b));
}

void Engine::applyDouble(double (*f)(double), bool angleInput,
                         bool angleOutput) {
  if (!ensureOperands(1)) return;
  Value a = m_stack.pop();
  double x = a.toDouble();
  if (angleInput && m_angleMode == AngleMode::Degree) x = toRadians(x);
  double y = f(x);
  if (angleOutput && m_angleMode == AngleMode::Degree) y = toDegrees(y);
  m_stack.push(Value::real(y));
}

void Engine::execute(Command command) {
  setStatus("");
  switch (command) {
    case Command::None:
      break;

    case Command::EnterOrDup:
      if (!m_input.isEmpty()) {
        commitInput();
      } else if (!m_stack.dup()) {
        setStatus("Empty stack");
      }
      break;

    case Command::DropOrBackspace:
      if (!m_input.isEmpty()) {
        m_input.backspace();
      } else if (!m_stack.drop()) {
        setStatus("Empty stack");
      }
      break;

    case Command::Swap:
      if (!ensureOperands(2)) break;
      m_stack.swap();
      break;
    case Command::Over:
      if (!ensureOperands(2)) break;
      m_stack.over();
      break;
    case Command::Rot:
      if (!ensureOperands(3)) break;
      m_stack.rot();
      break;
    case Command::Dup:
      if (!ensureOperands(1)) break;
      m_stack.dup();
      break;
    case Command::RollUp:
      if (!ensureOperands(2)) break;
      m_stack.rollUp();
      break;
    case Command::RollDown:
      if (!ensureOperands(2)) break;
      m_stack.rollDown();
      break;
    case Command::Pick: {
      // The pending input, if any, is the level index to copy.
      int level = 2;
      if (!m_input.isEmpty()) {
        Value v = Value::parse(m_input.text());
        m_input.clear();
        if (v.isExact()) level = (int)v.numerator();
      }
      if (!m_stack.pick(level)) setStatus("Bad level");
      break;
    }
    case Command::Clear:
      m_input.clear();
      m_stack.clear();
      break;
    case Command::ToDecimal:
      if (!ensureOperands(1)) break;
      m_stack.push(m_stack.pop().approx());
      break;

    case Command::Add:
      applyBinary(Value::add);
      break;
    case Command::Sub:
      applyBinary(Value::sub);
      break;
    case Command::Mul:
      applyBinary(Value::mul);
      break;
    case Command::Div:
      applyBinary(Value::div);
      break;
    case Command::Pow:
      applyBinary(Value::pow);
      break;

    case Command::Neg:
      if (!m_input.isEmpty()) {
        // Negate the number being typed rather than the stack.
        Value v = Value::parse(m_input.text());
        m_input.clear();
        m_stack.push(v.negated());
      } else {
        applyUnary([](const Value& a) { return a.negated(); });
      }
      break;
    case Command::Inv:
      applyUnary([](const Value& a) { return a.inverse(); });
      break;
    case Command::Sqrt:
      applyUnary(Value::sqrtValue);
      break;
    case Command::Square:
      applyUnary([](const Value& a) { return Value::mul(a, a); });
      break;
    case Command::Exp:
      applyDouble(::exp, false, false);
      break;
    case Command::Ln:
      applyDouble(::log, false, false);
      break;
    case Command::Log10:
      applyDouble(::log10, false, false);
      break;
    case Command::Sin:
      applyDouble(::sin, true, false);
      break;
    case Command::Cos:
      applyDouble(::cos, true, false);
      break;
    case Command::Tan:
      applyDouble(::tan, true, false);
      break;
    case Command::Asin:
      applyDouble(::asin, false, true);
      break;
    case Command::Acos:
      applyDouble(::acos, false, true);
      break;
    case Command::Atan:
      applyDouble(::atan, false, true);
      break;
    case Command::Factorial: {
      if (!ensureOperands(1)) break;
      Value a = m_stack.pop();
      double x = a.toDouble();
      if (a.isExact() && a.denominator() == 1 && a.numerator() >= 0 &&
          a.numerator() <= 20) {
        int64_t r = 1;
        for (int64_t i = 2; i <= a.numerator(); i++) r *= i;
        m_stack.push(Value::integer(r));
      } else if (x >= 0.0) {
        m_stack.push(Value::real(factorialDouble(x)));
      } else {
        m_stack.push(Value::undefined());
        setStatus("Factorial needs n>=0");
      }
      break;
    }

    case Command::PushPi:
      commitInput();
      m_stack.push(Value::real(kPi));
      break;
    case Command::PushE:
      commitInput();
      m_stack.push(Value::real(kE));
      break;

    case Command::ToggleAngle:
      toggleAngleMode();
      break;
  }
}
