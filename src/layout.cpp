#include "layout.h"

extern "C" {
#include <string.h>
}

#include "utf8.h"

namespace {
constexpr int kFracPad = 2;   // horizontal padding around a fraction
constexpr int kGap = 3;       // vertical gap between a bar and num/den
constexpr int kOverline = 3;  // radical vinculum height allowance
}  // namespace

int Layout::alloc(Kind k) {
  if (m_count >= kMaxNodes) return -1;
  int id = m_count++;
  Node& n = m_nodes[id];
  n.kind = k;
  n.text[0] = '\0';
  n.large = true;
  n.a = n.b = -1;
  n.nkids = 0;
  n.w = n.above = n.below = 0;
  return id;
}

int Layout::text(const char* s, bool large) {
  int id = alloc(Text);
  if (id < 0) return -1;
  Node& n = m_nodes[id];
  strncpy(n.text, s, sizeof(n.text) - 1);
  n.text[sizeof(n.text) - 1] = '\0';
  n.large = large;
  int gw = large ? kLargeW : kSmallW;
  int gh = large ? kLargeH : kSmallH;
  n.w = utf8GlyphCount(n.text) * gw;
  n.above = gh / 2;
  n.below = gh - gh / 2;
  return id;
}

int Layout::hbox(const int* kids, int count) {
  int id = alloc(HBox);
  if (id < 0) return -1;
  Node& n = m_nodes[id];
  n.nkids = count > kMaxKids ? kMaxKids : count;
  int w = 0, above = 0, below = 0;
  for (int i = 0; i < n.nkids; i++) {
    n.kids[i] = kids[i];
    w += width(kids[i]);
    if (this->above(kids[i]) > above) above = this->above(kids[i]);
    if (this->below(kids[i]) > below) below = this->below(kids[i]);
  }
  n.w = w;
  n.above = above;
  n.below = below;
  return id;
}

int Layout::frac(int top, int bot) {
  int id = alloc(Frac);
  if (id < 0) return -1;
  Node& n = m_nodes[id];
  n.a = top;
  n.b = bot;
  int tw = width(top), bw = width(bot);
  n.w = (tw > bw ? tw : bw) + 2 * kFracPad;
  n.above = height(top) + kGap;
  n.below = height(bot) + kGap;
  return id;
}

int Layout::radical(int inner) {
  int id = alloc(Radical);
  if (id < 0) return -1;
  Node& n = m_nodes[id];
  n.a = inner;
  n.w = kLargeW + width(inner) + 2;  // check mark + radicand
  n.above = above(inner) + kOverline;
  n.below = below(inner);
  return id;
}

int Layout::superscript(int base, int exp) {
  int id = alloc(Superscript);
  if (id < 0) return -1;
  Node& n = m_nodes[id];
  n.a = base;
  n.b = exp;
  int raise = above(base) - 2;  // lift the exponent by most of the base height
  n.w = width(base) + width(exp);
  int expAbove = raise + above(exp);
  n.above = expAbove > above(base) ? expAbove : above(base);
  n.below = below(base);
  return id;
}

void Layout::draw(int node, Canvas& canvas, int x, int axisY, unsigned fg,
                  unsigned bg) const {
  if (node < 0) return;
  const Node& n = m_nodes[node];
  switch (n.kind) {
    case Text:
      canvas.text(x, axisY - n.above, n.text, n.large, fg, bg);
      break;
    case HBox: {
      int cx = x;
      for (int i = 0; i < n.nkids; i++) {
        draw(n.kids[i], canvas, cx, axisY, fg, bg);
        cx += width(n.kids[i]);
      }
      break;
    }
    case Frac: {
      // bar on the axis, numerator above, denominator below, both centred
      canvas.rect(x, axisY - 1, n.w, 2, fg);
      int topAxis = axisY - kGap - below(n.a);
      int botAxis = axisY + kGap + above(n.b);
      draw(n.a, canvas, x + (n.w - width(n.a)) / 2, topAxis, fg, bg);
      draw(n.b, canvas, x + (n.w - width(n.b)) / 2, botAxis, fg, bg);
      break;
    }
    case Radical: {
      canvas.text(x, axisY - n.above, "\xe2\x88\x9a", true, fg, bg);  // √
      int ix = x + kLargeW + 2;
      canvas.rect(ix, axisY - above(n.a) - kOverline + 1, width(n.a) + 1, 2, fg);
      draw(n.a, canvas, ix, axisY, fg, bg);
      break;
    }
    case Superscript: {
      draw(n.a, canvas, x, axisY, fg, bg);
      int raise = above(n.a) - 2;
      draw(n.b, canvas, x + width(n.a), axisY - raise, fg, bg);
      break;
    }
  }
}
