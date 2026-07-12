// Host-side unit tests for the pure RPN engine (no EADK dependency).
// Build & run: see tests/Makefile  (make -C tests run)

#include <cstdio>
#include <cstring>

#include "../src/rpn.h"
#include "../src/value.h"

static int g_failures = 0;
static int g_checks = 0;

static void check(bool condition, const char* what) {
  g_checks++;
  if (!condition) {
    g_failures++;
    std::printf("  FAIL: %s\n", what);
  }
}

static bool formatEquals(const Value& v, const char* expected) {
  char buffer[64];
  v.format(buffer, sizeof(buffer));
  return std::strcmp(buffer, expected) == 0;
}

static void testValueArithmetic() {
  std::printf("Value arithmetic\n");
  // Exact rational addition stays exact: 1/3 + 1/6 = 1/2
  check(formatEquals(Value::add(Value::rational(1, 3), Value::rational(1, 6)),
                     "1/2"),
        "1/3 + 1/6 == 1/2");
  // Integer result reduces to an integer
  check(formatEquals(Value::add(Value::integer(2), Value::integer(3)), "5"),
        "2 + 3 == 5");
  // Division stays exact
  check(formatEquals(Value::div(Value::integer(3), Value::integer(6)), "1/2"),
        "3 / 6 == 1/2");
  // Division by zero is undefined
  check(Value::div(Value::integer(1), Value::integer(0)).isUndefined(),
        "1 / 0 undefined");
  // Sign normalization: 1/-2 -> -1/2
  check(formatEquals(Value::rational(1, -2), "-1/2"), "1/-2 == -1/2");
  // Overflow demotes to approximate rather than wrapping
  Value big = Value::mul(Value::integer(5000000000LL), Value::integer(5000000000LL));
  check(!big.isExact(), "overflowing product demotes to approx");
  // Exact integer power
  check(formatEquals(Value::pow(Value::integer(2), Value::integer(10)), "1024"),
        "2 ^ 10 == 1024");
  // Negative exponent stays exact
  check(formatEquals(Value::pow(Value::integer(2), Value::integer(-2)), "1/4"),
        "2 ^ -2 == 1/4");
  // Perfect square root stays exact
  check(formatEquals(Value::sqrtValue(Value::integer(9)), "3"), "sqrt(9) == 3");
  check(formatEquals(Value::sqrtValue(Value::rational(9, 4)), "3/2"),
        "sqrt(9/4) == 3/2");
  // Non-perfect square goes approximate
  check(!Value::sqrtValue(Value::integer(2)).isExact(), "sqrt(2) approx");
}

static void testParse() {
  std::printf("Parsing\n");
  check(formatEquals(Value::parse("42"), "42"), "parse integer");
  check(formatEquals(Value::parse("-7"), "-7"), "parse negative integer");
  check(!Value::parse("3.5").isExact(), "parse decimal -> approx");
  check(Value::parse("").isUndefined(), "parse empty -> undefined");
  check(!Value::parse("1e3").isExact(), "parse exponent -> approx");
}

static void testStack() {
  std::printf("Stack operations\n");
  Stack s;
  s.push(Value::integer(1));
  s.push(Value::integer(2));
  s.push(Value::integer(3));
  check(s.depth() == 3, "depth after 3 pushes");
  check(formatEquals(s.peek(1), "3"), "peek level 1 is top");
  check(formatEquals(s.peek(3), "1"), "peek level 3 is bottom");
  s.swap();  // 1 3 2
  check(formatEquals(s.peek(1), "2") && formatEquals(s.peek(2), "3"),
        "swap top two");
  // Stack is now (level3:1, level2:3, level1:2); ROT brings level 3 to top.
  s.rot();
  check(formatEquals(s.peek(1), "1"), "rot brings level 3 to top");
  s.clear();
  check(s.isEmpty(), "clear empties stack");
}

// Drive the full engine the way the keyboard would.
static void testEngineFlow() {
  std::printf("Engine flow\n");
  {
    Engine e;
    e.appendText("3");
    e.execute(Command::EnterOrDup);
    e.appendText("6");
    e.execute(Command::Div);  // commits 6, divides 3/6
    check(formatEquals(e.stack().peek(1), "1/2"), "3 ENTER 6 / == 1/2");
  }
  {
    Engine e;
    // Subtraction is level2 - level1: 3 ENTER 10 - gives 3 - 10 = -7
    e.appendText("3");
    e.execute(Command::EnterOrDup);
    e.appendText("10");
    e.execute(Command::Sub);
    check(formatEquals(e.stack().peek(1), "-7"), "3 ENTER 10 - == -7");
  }
  {
    Engine e;
    // SWAP fixes operand order: 3 ENTER 10 SWAP - gives 10 - 3 = 7
    e.appendText("3");
    e.execute(Command::EnterOrDup);
    e.appendText("10");
    e.execute(Command::EnterOrDup);
    e.execute(Command::Swap);
    e.execute(Command::Sub);
    check(formatEquals(e.stack().peek(1), "7"), "3 ENTER 10 SWAP - == 7");
  }
  {
    Engine e;
    // Negate while typing acts like CHS
    e.appendText("5");
    e.execute(Command::Neg);
    check(formatEquals(e.stack().peek(1), "-5"), "5 NEG == -5");
  }
  {
    Engine e;
    // DUP on empty input duplicates the top
    e.appendText("7");
    e.execute(Command::EnterOrDup);
    e.execute(Command::EnterOrDup);  // dup
    check(e.stack().depth() == 2 && formatEquals(e.stack().peek(1), "7"),
          "ENTER on empty input duplicates");
  }
  {
    Engine e;
    // Backspace drops when the input is empty
    e.appendText("4");
    e.execute(Command::EnterOrDup);
    e.execute(Command::DropOrBackspace);
    check(e.stack().isEmpty(), "backspace on empty input drops top");
  }
  {
    Engine e;
    // Factorial exact for small integers
    e.appendText("5");
    e.execute(Command::Factorial);
    check(formatEquals(e.stack().peek(1), "120"), "5! == 120");
  }
  {
    Engine e;
    // Too few arguments is reported, not crashed
    e.execute(Command::Add);
    check(std::strcmp(e.status(), "Too few arguments") == 0,
          "add with empty stack reports error");
  }
}

int main() {
  std::printf("== RPN engine tests ==\n");
  testValueArithmetic();
  testParse();
  testStack();
  testEngineFlow();
  std::printf("\n%d checks, %d failures\n", g_checks, g_failures);
  if (g_failures == 0) std::printf("ALL TESTS PASSED\n");
  return g_failures == 0 ? 0 : 1;
}
