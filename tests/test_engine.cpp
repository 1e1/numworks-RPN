// Host-side unit tests for the symbolic RPN engine (no EADK dependency).
// Build & run: make -C tests run

#include <cmath>
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

static bool fmtEq(const Value& v, const char* expected) {
  char buffer[64];
  v.format(buffer, sizeof(buffer));
  bool ok = std::strcmp(buffer, expected) == 0;
  if (!ok) std::printf("    got \"%s\" expected \"%s\"\n", buffer, expected);
  return ok;
}

static void testRational() {
  std::printf("Rational arithmetic\n");
  check(fmtEq(Value::add(Value::rational(1, 3), Value::rational(1, 6)), "1/2"),
        "1/3 + 1/6 == 1/2");
  check(fmtEq(Value::add(Value::integer(2), Value::integer(3)), "5"), "2+3==5");
  check(fmtEq(Value::div(Value::integer(3), Value::integer(6)), "1/2"),
        "3/6 == 1/2");
  check(Value::div(Value::integer(1), Value::integer(0)).isUndefined(),
        "1/0 undefined");
  check(fmtEq(Value::rational(1, -2), "-1/2"), "1/-2 == -1/2");
  Value big = Value::mul(Value::integer(5000000000LL), Value::integer(5000000000LL));
  check(!big.isExact(), "overflow demotes to approx");
  check(fmtEq(Value::pow(Value::integer(2), Value::integer(10)), "1024"),
        "2^10 == 1024");
  check(fmtEq(Value::pow(Value::integer(2), Value::integer(-2)), "1/4"),
        "2^-2 == 1/4");
}

static void testSymbolic() {
  std::printf("Symbolic radicals & pi\n");
  check(fmtEq(Value::sqrtValue(Value::integer(9)), "3"), "sqrt(9) == 3");
  check(fmtEq(Value::sqrtValue(Value::rational(9, 4)), "3/2"), "sqrt(9/4)==3/2");
  check(fmtEq(Value::sqrtValue(Value::integer(2)), "\xe2\x88\x9a" "2"),
        "sqrt(2) == √2");
  check(fmtEq(Value::sqrtValue(Value::integer(8)), "2\xe2\x88\x9a" "2"),
        "sqrt(8) == 2√2");
  // 2 * sqrt(8) = 4*sqrt(2)
  check(fmtEq(Value::mul(Value::integer(2), Value::sqrtValue(Value::integer(8))),
              "4\xe2\x88\x9a" "2"),
        "2*sqrt(8) == 4√2");
  // sqrt(2)^2 == 2 and sqrt(2)*sqrt(2) == 2
  Value r2 = Value::sqrtValue(Value::integer(2));
  check(fmtEq(Value::mul(r2, r2), "2"), "sqrt(2)*sqrt(2) == 2");
  check(fmtEq(Value::pow(r2, Value::integer(2)), "2"), "sqrt(2)^2 == 2");
  // sqrt(2)+sqrt(2) == 2*sqrt(2); sqrt(2)+sqrt(3) is inexact
  check(fmtEq(Value::add(r2, r2), "2\xe2\x88\x9a" "2"), "√2+√2 == 2√2");
  check(Value::add(r2, Value::sqrtValue(Value::integer(3))).isExact(),
        "√2+√3 stays exact (sum of terms)");
  // 1/sqrt(2) rationalizes to sqrt(2)/2
  check(fmtEq(r2.inverse(), "\xe2\x88\x9a" "2/2"), "1/√2 == √2/2");
  // pi forms
  check(fmtEq(Value::pi(), "\xcf\x80"), "pi == π");
  check(fmtEq(Value::mul(Value::integer(2), Value::pi()), "2\xcf\x80"),
        "2*pi == 2π");
  check(fmtEq(Value::div(Value::pi(), Value::integer(2)), "\xcf\x80/2"),
        "pi/2 == π/2");
  check(Value::add(Value::pi(), Value::integer(1)).isExact(),
        "pi + 1 stays exact (sum of terms)");
  check(std::fabs(Value::pi().toDouble() - 3.14159265) < 1e-6, "pi ~ 3.14159");
}

static void testSums() {
  std::printf("Sums of terms\n");
  Value r2 = Value::sqrtValue(Value::integer(2));
  Value r3 = Value::sqrtValue(Value::integer(3));
  // sqrt(2) + sqrt(3) stays exact as a two-term sum
  Value s = Value::add(r2, r3);
  check(s.isExact() && fmtEq(s, "\xe2\x88\x9a" "2 + \xe2\x88\x9a" "3"),
        "√2 + √3 stays exact");
  // pi + 1
  check(fmtEq(Value::add(Value::pi(), Value::integer(1)), "1 + \xcf\x80"),
        "pi + 1 == 1 + π");
  check(!Value::add(Value::pi(), Value::integer(1)).isInteger(), "pi+1 not int");
  // (1 + sqrt(2))^2 = 3 + 2 sqrt(2)
  Value onePlusR2 = Value::add(Value::integer(1), r2);
  check(fmtEq(Value::pow(onePlusR2, Value::integer(2)), "3 + 2\xe2\x88\x9a" "2"),
        "(1+√2)^2 == 3+2√2");
  // conjugate product collapses: (sqrt2+sqrt3)(sqrt2-sqrt3) = -1
  check(fmtEq(Value::mul(Value::add(r2, r3), Value::sub(r2, r3)), "-1"),
        "(√2+√3)(√2-√3) == -1");
  // like terms collect: sqrt(2) + sqrt(2) = 2 sqrt(2)
  check(fmtEq(Value::add(r2, r2), "2\xe2\x88\x9a" "2"), "√2+√2 == 2√2");
  // dividing by a two-term sum uses the conjugate and stays exact: √2 - 1
  check(fmtEq(Value::div(Value::integer(1), onePlusR2), "-1 + \xe2\x88\x9a" "2"),
        "1/(1+√2) == √2-1 (conjugate, exact)");
  // conjugate division with a non-unit denominator: 1/(√3-1) = 1/2 + √3/2
  Value invR3m1 = Value::div(Value::integer(1), Value::sub(r3, Value::integer(1)));
  check(invR3m1.isExact() && fmtEq(invR3m1, "1/2 + \xe2\x88\x9a" "3/2"),
        "1/(√3-1) == 1/2 + √3/2");
  // nested radical stays exact and renders with an inner parenthesised radicand
  Value nested = Value::sqrtValue(onePlusR2);
  check(nested.isExact() && fmtEq(nested, "\xe2\x88\x9a(1 + \xe2\x88\x9a" "2)"),
        "√(1+√2) stays exact and nests");
}

static void testVariables() {
  std::printf("Symbolic variables\n");
  Value x = Value::var('x');
  check(fmtEq(x, "x"), "x renders as x");
  check(fmtEq(Value::add(x, x), "2x"), "x + x == 2x");
  check(fmtEq(Value::mul(x, x), "x^2"), "x * x == x^2");
  check(fmtEq(Value::add(Value::mul(Value::integer(3), x), x), "4x"),
        "3x + x == 4x");
  check(std::isnan(x.toDouble()), "a free variable has no numeric value");
}

static void testMemory() {
  std::printf("Garbage collection\n");
  // A collection must preserve every value the roots can still reach.
  Stack s;
  s.push(Value::sqrtValue(Value::integer(2)));                 // √2
  s.push(Value::div(Value::pi(), Value::integer(2)));          // π/2
  s.push(Value::sqrtValue(Value::add(Value::integer(1),
                                     Value::sqrtValue(Value::integer(2)))));  // √(1+√2)
  Value::collect(s.mutableData(), s.depth());
  check(fmtEq(s.peek(3), "\xe2\x88\x9a" "2"), "√2 survives a collection");
  check(fmtEq(s.peek(2), "\xcf\x80/2"), "π/2 survives a collection");
  check(fmtEq(s.peek(1), "\xe2\x88\x9a(1 + \xe2\x88\x9a" "2)"),
        "nested radical survives a collection");
  // The value stays usable afterwards: √2 · √2 == 2.
  check(fmtEq(Value::mul(s.peek(3), s.peek(3)), "2"),
        "a collected value is still computable");
}

static void testStack() {
  std::printf("Stack operations\n");
  Stack s;
  s.push(Value::integer(1));
  s.push(Value::integer(2));
  s.push(Value::integer(3));
  check(s.depth() == 3, "depth 3");
  check(fmtEq(s.peek(1), "3"), "peek level 1 is top");
  s.swap();
  check(fmtEq(s.peek(1), "2") && fmtEq(s.peek(2), "3"), "swap top two");
  s.rot();  // (level3:1, level2:3, level1:2) -> top becomes 1
  check(fmtEq(s.peek(1), "1"), "rot brings level 3 to top");
  s.clear();
  check(s.isEmpty(), "clear empties stack");

  // over / roll / pick on a fresh 1,2,3,4 stack (level 1 == 4)
  for (int i = 1; i <= 4; i++) s.push(Value::integer(i));
  s.over();  // copy level 2 (==3) to the top
  check(fmtEq(s.peek(1), "3") && s.depth() == 5, "over copies level 2");
  s.drop();
  s.rollUp();  // top goes to the bottom -> 4,1,2,3
  check(fmtEq(s.peek(1), "3"), "rollUp moves the top to the bottom");
  s.rollDown();  // bottom comes back to the top -> 1,2,3,4
  check(fmtEq(s.peek(1), "4"), "rollDown moves the bottom to the top");
  s.pick(2);  // copy level 2 (==3) to the top
  check(fmtEq(s.peek(1), "3") && s.depth() == 5, "pick copies the given level");
}

static void testEngineFlow() {
  std::printf("Engine flow\n");
  {
    Engine e;
    e.appendText("3");
    e.execute(Command::EnterOrDup);
    e.appendText("6");
    e.execute(Command::Div);
    check(fmtEq(e.stack().peek(1), "1/2"), "3 ENTER 6 / == 1/2");
  }
  {
    Engine e;
    e.appendText("3");
    e.execute(Command::EnterOrDup);
    e.appendText("10");
    e.execute(Command::Sub);
    check(fmtEq(e.stack().peek(1), "-7"), "3 ENTER 10 - == -7");
  }
  {
    Engine e;  // SWAP fixes operand order: 10 - 3 == 7
    e.appendText("3");
    e.execute(Command::EnterOrDup);
    e.appendText("10");
    e.execute(Command::EnterOrDup);
    e.execute(Command::Swap);
    e.execute(Command::Sub);
    check(fmtEq(e.stack().peek(1), "7"), "3 ENTER 10 SWAP - == 7");
  }
  {
    Engine e;  // 2 ENTER 8 sqrt * == 4√2
    e.appendText("2");
    e.execute(Command::EnterOrDup);
    e.appendText("8");
    e.execute(Command::Sqrt);
    e.execute(Command::Mul);
    check(fmtEq(e.stack().peek(1), "4\xe2\x88\x9a" "2"), "2 ENTER 8 √ * == 4√2");
  }
  {
    Engine e;  // π then 2 then / == π/2
    e.execute(Command::PushPi);
    e.appendText("2");
    e.execute(Command::Div);
    check(fmtEq(e.stack().peek(1), "\xcf\x80/2"), "π 2 / == π/2");
  }
  {
    Engine e;
    e.appendText("5");
    e.execute(Command::Neg);
    check(fmtEq(e.stack().peek(1), "-5"), "5 NEG == -5");
  }
  {
    Engine e;
    e.appendText("7");
    e.execute(Command::EnterOrDup);
    e.execute(Command::EnterOrDup);  // dup
    check(e.stack().depth() == 2 && fmtEq(e.stack().peek(1), "7"),
          "ENTER on empty input duplicates");
  }
  {
    Engine e;
    e.appendText("4");
    e.execute(Command::EnterOrDup);
    e.execute(Command::DropOrBackspace);
    check(e.stack().isEmpty(), "backspace on empty input drops");
  }
  {
    Engine e;
    e.appendText("5");
    e.execute(Command::Factorial);
    check(fmtEq(e.stack().peek(1), "120"), "5! == 120");
  }
  {
    Engine e;  // large n uses gamma, stays finite, must not hang
    e.appendText("25");
    e.execute(Command::Factorial);
    check(!e.stack().peek(1).isExact() && std::isfinite(e.stack().peek(1).toDouble()),
          "25! is a finite approximation");
  }
  {
    Engine e;  // negative factorial rejected
    e.appendText("3");
    e.execute(Command::Neg);
    e.execute(Command::Factorial);
    check(std::strcmp(e.status(), "Factorial needs n>=0") == 0,
          "(-3)! reports an error");
  }
  {
    Engine e;
    e.execute(Command::Add);
    check(std::strcmp(e.status(), "Too few arguments") == 0,
          "add on empty stack reports error");
  }
}

int main() {
  std::printf("== RPN symbolic engine tests ==\n");
  testRational();
  testSymbolic();
  testSums();
  testVariables();
  testMemory();
  testStack();
  testEngineFlow();
  std::printf("\n%d checks, %d failures\n", g_checks, g_failures);
  if (g_failures == 0) std::printf("ALL TESTS PASSED\n");
  return g_failures == 0 ? 0 : 1;
}
