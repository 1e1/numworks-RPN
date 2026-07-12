#pragma once

/* Minimal 2D math layout engine, backend-agnostic.
 *
 * A Value is turned into a small tree of boxes (text, horizontal run, fraction,
 * radical, superscript). Each box is measured in a TeX-like model: a width and
 * an extent above/below a horizontal axis, so pieces of different heights line
 * up on a common midline. Drawing is done against an abstract Canvas, so the
 * same layout code feeds the device screen (EADK) and a host PNG dumper. */

class Canvas {
 public:
  virtual ~Canvas() {}
  // Draw text with its top-left at (x, y). large selects the 10x18 vs 7x14 font.
  // Colours are 0xRRGGBB.
  virtual void text(int x, int y, const char* s, bool large, unsigned fg,
                    unsigned bg) = 0;
  // Fill a coloured rectangle (backgrounds, fraction bars, radical vinculum).
  virtual void rect(int x, int y, int w, int h, unsigned color) = 0;
};

class Layout {
 public:
  // Font metrics (Epsilon: large glyph 10x18, small 7x14).
  static constexpr int kLargeW = 10;
  static constexpr int kLargeH = 18;
  static constexpr int kSmallW = 7;
  static constexpr int kSmallH = 14;

  Layout() : m_count(0) {}

  // Builders return a node id (>= 0), or -1 when the pool is exhausted.
  int text(const char* s, bool large);
  int hbox(const int* kids, int n);
  int frac(int top, int bot);
  int radical(int inner);
  int superscript(int base, int exp);

  // Geometry (valid after the node and its children were built).
  int width(int node) const { return node < 0 ? 0 : m_nodes[node].w; }
  int above(int node) const { return node < 0 ? 0 : m_nodes[node].above; }
  int below(int node) const { return node < 0 ? 0 : m_nodes[node].below; }
  int height(int node) const { return above(node) + below(node); }

  // Draw the node with its axis at (x, axisY), top-left x growing right.
  // fg/bg are the ink and background colours (0xRRGGBB) for glyphs and bars.
  void draw(int node, Canvas& canvas, int x, int axisY, unsigned fg,
            unsigned bg) const;

 private:
  static constexpr int kMaxNodes = 96;
  static constexpr int kMaxKids = 20;
  enum Kind { Text, HBox, Frac, Radical, Superscript };
  struct Node {
    Kind kind;
    char text[16];
    bool large;
    int a, b;             // children for Frac(top,bot)/Superscript(base,exp)/Radical(inner in a)
    int kids[kMaxKids];   // children for HBox
    int nkids;
    int w, above, below;  // measured box
  };
  int alloc(Kind k);
  Node m_nodes[kMaxNodes];
  int m_count;
};
