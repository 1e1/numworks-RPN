#pragma once

/* Symbolic-numeric value on the RPN stack: a general expression tree kept in a
 * canonical polynomial-in-atoms form  Σ coeff · Π atom^e  (atom ∈ π, variable,
 * √(sub-expression), 1/(sub-expression)), OR an inexact double, OR undefined.
 *
 * Exact where it can be — fractions, k√m, rational multiples of π and their
 * sums/products/integer powers, nested radicals (√(1+√2)) and conjugate
 * division (1/(1+√2) → √2−1) — and a decimal fallback otherwise, so a result is
 * never wrongly exact.
 *
 * Nodes live in a shared, fixed, index-based arena (no std::vector / shared_ptr
 * / exceptions). Reclaim with collect(); when the arena is exhausted the current
 * operation degrades to a decimal instead of corrupting memory. */
class Value {
 public:
  Value();  // zero (exact)

  static Value integer(long long n);
  static Value rational(long long num, long long den);
  static Value real(double x);
  static Value pi();
  static Value var(char name);
  static Value undefined();
  static Value parse(const char* text);

  bool isExact() const { return m_kind == 0; }
  bool isUndefined() const { return m_kind == 2; }
  bool isInteger() const;
  long long integerValue() const;
  double toDouble() const;

  Value negated() const;
  Value inverse() const;
  Value approx() const { return real(toDouble()); }

  static Value add(const Value& a, const Value& b);
  static Value sub(const Value& a, const Value& b);
  static Value mul(const Value& a, const Value& b);
  static Value div(const Value& a, const Value& b);
  static Value pow(const Value& a, const Value& b);
  static Value sqrtValue(const Value& a);

  int format(char* buffer, int size) const;
  int buildLayout(class Layout& layout) const;

  // Arena management (called by the engine).
  static int arenaUsage();                       // live polynomial count
  static bool arenaNearFull();                   // time to collect?
  static void collect(Value* roots, int count);  // compact, keeping roots

 private:
  explicit Value(int poly) : m_poly(poly), m_real(0.0), m_kind(0) {}
  int m_poly;
  double m_real;
  unsigned char m_kind;  // 0 exact, 1 real, 2 undefined
  friend struct ValueImpl;
};
