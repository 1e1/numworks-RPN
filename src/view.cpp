#include "view.h"

#include "eadkpp.h"
#include "screen.h"

namespace {
// Adapts the backend-agnostic Screen renderer onto the EADK display.
struct EadkCanvas : Canvas {
  void text(int x, int y, const char* s, bool large, unsigned fg,
            unsigned bg) override {
    EADK::Display::drawString(s, EADK::Point(x, y), large, EADK::Color(fg),
                              EADK::Color(bg));
  }
  void rect(int x, int y, int w, int h, unsigned color) override {
    EADK::Display::pushRectUniform(EADK::Rect(x, y, w, h), EADK::Color(color));
  }
};
}  // namespace

void View::draw(const Engine& engine, bool menuOpen, int menuSelection) {
  EadkCanvas canvas;
  Screen::render(canvas, engine, menuOpen, menuSelection);
}
