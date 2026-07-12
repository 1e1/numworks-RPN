// Host renderer for the full calculator screen: drives an Engine with a sample
// stack, renders via Screen::render into recorded draw-ops, then paint.py paints
// a 320x240 PNG to inspect. No EADK dependency.

#include <cstdio>

#include "../src/layout.h"
#include "../src/rpn.h"
#include "../src/screen.h"

namespace {
struct RecCanvas : Canvas {
  void text(int x, int y, const char* s, bool large, unsigned fg, unsigned bg) override {
    std::printf("T %d %d %d %06x %06x %s\n", x, y, large ? 1 : 0, fg, bg, s);
  }
  void rect(int x, int y, int w, int h, unsigned color) override {
    std::printf("R %d %d %d %d %06x\n", x, y, w, h, color);
  }
};
void feed(Engine& e, const char* digits) {
  for (const char* p = digits; *p; p++) {
    char s[2] = {*p, 0};
    e.appendText(s);
  }
}
}  // namespace

int main() {
  Engine e;
  // Build a stack: 1/2, √2+√3, 3+2√2, π/2, then type "4"
  feed(e, "1"); e.execute(Command::EnterOrDup);
  feed(e, "2"); e.execute(Command::Div);            // 1/2
  feed(e, "2"); e.execute(Command::Sqrt);
  feed(e, "3"); e.execute(Command::Sqrt);
  e.execute(Command::Add);                          // √2+√3
  feed(e, "1"); e.execute(Command::EnterOrDup);
  feed(e, "2"); e.execute(Command::Sqrt); e.execute(Command::Add);
  feed(e, "2"); e.execute(Command::Pow);            // (1+√2)^2 = 3+2√2
  e.execute(Command::PushPi);
  feed(e, "2"); e.execute(Command::Div);            // π/2
  feed(e, "4");                                     // pending input

  std::printf("CANVAS %d %d\n", Screen::kWidth, Screen::kHeight);
  RecCanvas c;
  Screen::render(c, e, false, 0);
  return 0;
}
