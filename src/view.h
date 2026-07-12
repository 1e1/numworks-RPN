#pragma once

#include "rpn.h"

/* Renders the whole screen from the engine state. Simple and stateless: every
 * key press triggers a full redraw, which is plenty fast on the device. */
class View {
 public:
  void draw(const Engine& engine, bool menuOpen, int menuSelection);
};
