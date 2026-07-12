#pragma once

#include "layout.h"
#include "rpn.h"

/* Renders the whole calculator screen (title bar, stack with 2D layouts, the
 * level-1 decimal approximation, status line and input) against an abstract
 * Canvas. The device view and the host PNG harness share this exact code. */
namespace Screen {
constexpr int kWidth = 320;
constexpr int kHeight = 240;

void render(Canvas& canvas, const Engine& engine, bool menuOpen,
            int menuSelection);
}  // namespace Screen
