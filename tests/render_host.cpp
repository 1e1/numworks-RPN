// Host renderer for the 2D layout engine: builds layouts for sample values and
// prints their draw-ops so tests/paint.py can paint a PNG to inspect.
// No EADK dependency.

#include <cstdio>

#include "../src/layout.h"
#include "../src/value.h"

namespace {
struct RecCanvas : Canvas {
  void text(int x, int y, const char* s, bool large, unsigned fg, unsigned bg) override {
    std::printf("T %d %d %d %06x %06x %s\n", x, y, large ? 1 : 0, fg, bg, s);
  }
  void rect(int x, int y, int w, int h, unsigned color) override {
    std::printf("R %d %d %d %d %06x\n", x, y, w, h, color);
  }
};

void item(const char* label, const Value& v) {
  Layout l;
  int root = v.buildLayout(l);
  std::printf("ITEM %s %d %d %d\n", label, l.width(root), l.above(root), l.below(root));
  RecCanvas c;
  l.draw(root, c, 0, l.above(root), 0x1a1b1e, 0xffffff);
  std::printf("END\n");
}

Value r(int n) { return Value::integer(n); }
Value sqrtI(int n) { return Value::sqrtValue(Value::integer(n)); }
}  // namespace

int main() {
  item("1/2", Value::rational(1, 2));
  item("-3/4", Value::rational(-3, 4));
  item("3", r(3));
  item("sqrt2", sqrtI(2));
  item("4sqrt2", Value::mul(r(2), sqrtI(8)));
  item("sqrt2/2", sqrtI(2).inverse());
  item("pi", Value::pi());
  item("2pi", Value::mul(r(2), Value::pi()));
  item("pi/2", Value::div(Value::pi(), r(2)));
  item("pi^2", Value::mul(Value::pi(), Value::pi()));
  item("sqrt2+sqrt3", Value::add(sqrtI(2), sqrtI(3)));
  item("3+2sqrt2", Value::pow(Value::add(r(1), sqrtI(2)), r(2)));
  item("1+pi", Value::add(Value::pi(), r(1)));
  item("approx", Value::real(1.4142136));
  return 0;
}
