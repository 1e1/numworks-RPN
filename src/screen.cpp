#include "screen.h"

extern "C" {
#include <stdio.h>
#include <string.h>
}

#include "menu.h"
#include "utf8.h"

namespace {
using Screen::kHeight;
using Screen::kWidth;

// Palette (0xRRGGBB)
constexpr unsigned kBg = 0xFFFFFF;
constexpr unsigned kInk = 0x1A1B1E;
constexpr unsigned kTitleBg = 0xF5A623;
constexpr unsigned kLabel = 0x8C8C8C;
constexpr unsigned kSep = 0xD0D0D0;
constexpr unsigned kError = 0xB00000;
constexpr unsigned kMenuBg = 0x2B2B2B;
constexpr unsigned kMenuSelBg = 0xF5A623;
constexpr unsigned kMenuTxt = 0xF0F0F0;

constexpr int kTitleH = 20;
constexpr int kInputH = 22;
constexpr int kInputY = kHeight - kInputH;  // 218
constexpr int kStatusH = 18;
constexpr int kStatusY = kInputY - kStatusH;  // 200
constexpr int kStackTop = kTitleH + 2;        // 22
constexpr int kStackBottom = kStatusY - 2;    // 198
constexpr int kLargeW = Layout::kLargeW;

void textLarge(Canvas& c, int x, int y, const char* s, unsigned fg, unsigned bg) {
  c.text(x, y, s, true, fg, bg);
}
void textSmall(Canvas& c, int x, int y, const char* s, unsigned fg, unsigned bg) {
  c.text(x, y, s, false, fg, bg);
}
int rightX(int width) {
  int x = kWidth - 4 - width;
  return x < 2 ? 2 : x;
}

void drawMenu(Canvas& c, int selection) {
  const int rowH = 16;
  const int rows = kMenuEntryCount + 1;
  const int boxW = kWidth - 48;
  const int boxH = rows * rowH + 6;
  const int boxX = 24;
  const int boxY = (kHeight - boxH) / 2;
  c.rect(boxX - 2, boxY - 2, boxW + 4, boxH + 4, kInk);
  c.rect(boxX, boxY, boxW, boxH, kMenuBg);
  textSmall(c, boxX + 6, boxY + 3, "STACK MENU", kMenuSelBg, kMenuBg);
  for (int i = 0; i < kMenuEntryCount; i++) {
    int y = boxY + 3 + (i + 1) * rowH;
    bool sel = i == selection;
    if (sel) c.rect(boxX + 2, y - 1, boxW - 4, rowH, kMenuSelBg);
    textSmall(c, boxX + 6, y, kMenuEntries[i].label, sel ? kInk : kMenuTxt,
              sel ? kMenuSelBg : kMenuBg);
  }
}
}  // namespace

void Screen::render(Canvas& c, const Engine& engine, bool menuOpen,
                    int menuSelection) {
  c.rect(0, 0, kWidth, kHeight, kBg);

  // Title bar
  c.rect(0, 0, kWidth, kTitleH, kTitleBg);
  textLarge(c, 4, 1, "RPN", kInk, kTitleBg);
  const char* mode = engine.angleMode() == AngleMode::Radian ? "RAD" : "DEG";
  textLarge(c, kWidth - 4 - utf8GlyphCount(mode) * kLargeW, 1, mode, kInk, kTitleBg);

  // Stack: pack levels from the bottom up, each as its 2D layout
  const Stack& stack = engine.stack();
  int yCursor = kStackBottom;
  char label[8];
  for (int level = 1; level <= stack.depth(); level++) {
    Layout layout;
    int root = stack.peek(level).buildLayout(layout);
    int w = layout.width(root);
    int h = layout.height(root);
    if (h < Layout::kLargeH) h = Layout::kLargeH;
    int top = yCursor - h;
    if (top < kStackTop) {
      textSmall(c, 2, kStackTop, "\xe2\x8b\xae", kLabel, kBg);  // ⋮ more above
      break;
    }
    int axisY = top + layout.above(root);
    snprintf(label, sizeof(label), "%d:", level);
    textSmall(c, 2, axisY - Layout::kSmallH / 2, label, kLabel, kBg);
    layout.draw(root, c, rightX(w), axisY, kInk, kBg);
    yCursor = top - 6;
  }

  // Status line: error, else the decimal approximation of level 1 (if exact)
  c.rect(0, kStatusY, kWidth, kStatusH, kBg);
  if (engine.status()[0] != '\0') {
    textSmall(c, 4, kStatusY + 1, engine.status(), kError, kBg);
  } else if (stack.depth() >= 1) {
    const Value& top = stack.peek(1);
    if (top.isExact()) {
      char approx[32];
      snprintf(approx, sizeof(approx), "\xe2\x89\x88 %.7g", top.toDouble());  // ≈
      textSmall(c, kWidth - 4 - utf8GlyphCount(approx) * Layout::kSmallW,
                kStatusY + 1, approx, kLabel, kBg);
    }
  }

  // Input line
  c.rect(0, kInputY - 1, kWidth, 1, kSep);
  char line[64];
  snprintf(line, sizeof(line), ">%s", engine.input().text());
  textLarge(c, 4, kInputY + 2, line, kInk, kBg);
  int cursorX = 4 + utf8GlyphCount(line) * kLargeW + 1;
  c.rect(cursorX, kInputY + 2, 2, Layout::kLargeH, kInk);

  if (menuOpen) drawMenu(c, menuSelection);
}
