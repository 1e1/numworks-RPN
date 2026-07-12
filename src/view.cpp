#include "view.h"

extern "C" {
#include <stdio.h>
#include <string.h>
}

#include "eadkpp.h"
#include "menu.h"

namespace {

using EADK::Color;
using EADK::Point;
using EADK::Rect;
namespace Display = EADK::Display;

// Screen and font metrics (Epsilon: small glyph 7x14, large glyph 10x18).
constexpr int kWidth = EADK::Screen::Width;    // 320
constexpr int kHeight = EADK::Screen::Height;  // 240
constexpr int kLargeW = 10;
constexpr int kLargeH = 18;

constexpr int kTitleH = 20;
constexpr int kInputH = 22;
constexpr int kInputY = kHeight - kInputH;          // 218
constexpr int kStatusH = 18;
constexpr int kStatusY = kInputY - kStatusH;        // 200
constexpr int kStackTop = kTitleH + 2;              // 22
constexpr int kStackBottom = kStatusY - 2;          // 198
constexpr int kRowH = kLargeH;
constexpr int kVisibleRows = (kStackBottom - kStackTop) / kRowH;

// Palette
const Color kBackground = 0xFFFFFF;
const Color kText = 0x000000;
const Color kTitleBg = 0xFFB734;  // NumWorks amber
const Color kLabel = 0x808080;
const Color kSeparator = 0xD0D0D0;
const Color kError = 0xB00000;
const Color kMenuBg = 0x2B2B2B;
const Color kMenuText = 0xF0F0F0;
const Color kMenuSelBg = 0xFFB734;
const Color kMenuSelText = 0x000000;

void fill(int x, int y, int w, int h, Color c) {
  Display::pushRectUniform(Rect(x, y, w, h), c);
}

void text(const char* s, int x, int y, bool large, Color fg, Color bg) {
  Display::drawString(s, Point(x, y), large, fg, bg);
}

// Right-aligned large text ending at the right margin.
void textRight(const char* s, int rightX, int y, Color fg, Color bg) {
  int width = (int)strlen(s) * kLargeW;
  int x = rightX - width;
  if (x < 0) x = 0;
  text(s, x, y, true, fg, bg);
}

void drawTitle(const Engine& engine) {
  fill(0, 0, kWidth, kTitleH, kTitleBg);
  text("RPN", 4, 1, true, kText, kTitleBg);
  const char* mode = engine.angleMode() == AngleMode::Radian ? "RAD" : "DEG";
  textRight(mode, kWidth - 4, 1, kText, kTitleBg);
}

void drawStack(const Engine& engine) {
  fill(0, kStackTop, kWidth, kStackBottom - kStackTop, kBackground);
  const Stack& stack = engine.stack();
  int shown = stack.depth();
  if (shown > kVisibleRows) shown = kVisibleRows;

  char buffer[64];
  char label[8];
  for (int level = 1; level <= shown; level++) {
    int y = kStackBottom - level * kRowH;
    snprintf(label, sizeof(label), "%d:", level);
    text(label, 2, y + 2, false, kLabel, kBackground);
    stack.peek(level).format(buffer, sizeof(buffer));
    textRight(buffer, kWidth - 4, y, kText, kBackground);
  }
  // Indicate hidden deeper levels.
  if (stack.depth() > kVisibleRows) {
    text("...", 2, kStackTop, false, kLabel, kBackground);
  }
}

void drawStatusAndInput(const Engine& engine) {
  // Status line
  fill(0, kStatusY, kWidth, kStatusH, kBackground);
  if (engine.status()[0] != '\0') {
    text(engine.status(), 4, kStatusY + 1, false, kError, kBackground);
  }
  // Separator + input line
  fill(0, kInputY - 1, kWidth, 1, kSeparator);
  fill(0, kInputY, kWidth, kInputH, kBackground);

  char line[EADK::Screen::Width];
  snprintf(line, sizeof(line), ">%s", engine.input().text());
  text(line, 4, kInputY + 2, true, kText, kBackground);
  // Block cursor after the text.
  int cursorX = 4 + (int)strlen(line) * kLargeW;
  fill(cursorX + 1, kInputY + 2, 2, kLargeH, kText);
}

void drawMenu(int selection) {
  const int boxX = 24, boxW = kWidth - 48;
  const int rowH = 16;
  const int rows = kMenuEntryCount + 1;  // title + entries
  const int boxH = rows * rowH + 6;
  const int boxY = (kHeight - boxH) / 2;

  fill(boxX - 2, boxY - 2, boxW + 4, boxH + 4, kText);
  fill(boxX, boxY, boxW, boxH, kMenuBg);
  text("STACK MENU", boxX + 6, boxY + 3, false, kMenuSelBg, kMenuBg);

  for (int i = 0; i < kMenuEntryCount; i++) {
    int y = boxY + 3 + (i + 1) * rowH;
    bool selected = (i == selection);
    Color bg = selected ? kMenuSelBg : kMenuBg;
    Color fg = selected ? kMenuSelText : kMenuText;
    if (selected) fill(boxX + 2, y - 1, boxW - 4, rowH, bg);
    text(kMenuEntries[i].label, boxX + 6, y, false, fg, bg);
  }
}

}  // namespace

void View::draw(const Engine& engine, bool menuOpen, int menuSelection) {
  fill(0, 0, kWidth, kHeight, kBackground);
  drawTitle(engine);
  drawStack(engine);
  drawStatusAndInput(engine);
  if (menuOpen) drawMenu(menuSelection);
}
