#pragma once

extern "C" {
#include <stdint.h>
}

/* Value is the single numeric type flowing through the RPN stack.
 *
 * It is either:
 *   - EXACT: a reduced rational num/den (den > 0), used for +, -, *, / and
 *     integer powers so that 1/3 + 1/6 stays 1/2 instead of 0.5;
 *   - APPROX: an IEEE double, used as soon as a transcendental function
 *     (sin, ln, ...) or a non-representable result is involved.
 *
 * Every exact operation demotes to APPROX on overflow, so the engine never
 * silently produces a wrong exact result. */
class Value {
 public:
  Value() : m_num(0), m_den(1), m_approx(0.0), m_exact(false), m_undef(true) {}

  static Value integer(int64_t n) { return Value(true, n, 1, 0.0, false); }
  static Value rational(int64_t num, int64_t den);
  static Value real(double x) { return Value(false, 0, 1, x, false); }
  static Value undefined() { return Value(false, 0, 1, 0.0, true); }

  // Parse a user-typed decimal string ("12", "-3.5", "6e2"). Empty -> undefined.
  static Value parse(const char* text);

  bool isExact() const { return m_exact && !m_undef; }
  bool isUndefined() const { return m_undef; }
  int64_t numerator() const { return m_num; }
  int64_t denominator() const { return m_den; }
  double toDouble() const;

  Value negated() const;
  Value inverse() const;   // 1/x
  Value approx() const { return real(toDouble()); }

  static Value add(const Value& a, const Value& b);
  static Value sub(const Value& a, const Value& b);
  static Value mul(const Value& a, const Value& b);
  static Value div(const Value& a, const Value& b);
  static Value pow(const Value& a, const Value& b);
  static Value sqrtValue(const Value& a);

  // Writes a human-readable form into buffer; returns written length.
  int format(char* buffer, int bufferSize) const;

 private:
  Value(bool exact, int64_t num, int64_t den, double approx, bool undef)
      : m_num(num), m_den(den), m_approx(approx), m_exact(exact),
        m_undef(undef) {}

  int64_t m_num;
  int64_t m_den;
  double m_approx;
  bool m_exact;
  bool m_undef;
};
