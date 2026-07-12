#include "value.h"

extern "C" {
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
}

namespace {

int64_t gcd64(int64_t a, int64_t b) {
  if (a < 0) a = -a;
  if (b < 0) b = -b;
  while (b) {
    int64_t t = a % b;
    a = b;
    b = t;
  }
  return a;
}

// Overflow-checked 64-bit arithmetic. The target (32-bit Cortex-M7) has no
// __int128, so we rely on the compiler builtins and demote to double whenever
// an exact result would overflow int64.
inline bool mulOverflow(int64_t a, int64_t b, int64_t* out) {
  return __builtin_mul_overflow(a, b, out);
}
inline bool addOverflow(int64_t a, int64_t b, int64_t* out) {
  return __builtin_add_overflow(a, b, out);
}

int64_t isqrt64(int64_t n) {
  if (n < 0) return -1;
  int64_t r = (int64_t)sqrt((double)n);
  while (r > 0 && r * r > n) r--;
  while ((r + 1) * (r + 1) <= n) r++;
  return r;
}

}  // namespace

Value Value::rational(int64_t num, int64_t den) {
  if (den == 0) return undefined();
  if (den < 0) {
    num = -num;
    den = -den;
  }
  int64_t g = gcd64(num, den);
  if (g != 0) {
    num /= g;
    den /= g;
  }
  return Value(true, num, den, 0.0, false);
}

double Value::toDouble() const {
  if (m_undef) return NAN;
  if (m_exact) return (double)m_num / (double)m_den;
  return m_approx;
}

Value Value::negated() const {
  if (m_undef) return *this;
  if (m_exact) return Value(true, -m_num, m_den, 0.0, false);
  return real(-m_approx);
}

Value Value::inverse() const {
  if (m_undef) return *this;
  if (m_exact) return rational(m_den, m_num);
  if (m_approx == 0.0) return undefined();
  return real(1.0 / m_approx);
}

Value Value::add(const Value& a, const Value& b) {
  if (a.m_undef || b.m_undef) return undefined();
  if (a.isExact() && b.isExact()) {
    int64_t left, right, num, den;
    if (!mulOverflow(a.m_num, b.m_den, &left) &&
        !mulOverflow(b.m_num, a.m_den, &right) &&
        !addOverflow(left, right, &num) &&
        !mulOverflow(a.m_den, b.m_den, &den)) {
      return rational(num, den);
    }
  }
  return real(a.toDouble() + b.toDouble());
}

Value Value::sub(const Value& a, const Value& b) {
  return add(a, b.negated());
}

Value Value::mul(const Value& a, const Value& b) {
  if (a.m_undef || b.m_undef) return undefined();
  if (a.isExact() && b.isExact()) {
    int64_t num, den;
    if (!mulOverflow(a.m_num, b.m_num, &num) &&
        !mulOverflow(a.m_den, b.m_den, &den)) {
      return rational(num, den);
    }
  }
  return real(a.toDouble() * b.toDouble());
}

Value Value::div(const Value& a, const Value& b) {
  if (a.m_undef || b.m_undef) return undefined();
  if (a.isExact() && b.isExact()) {
    if (b.m_num == 0) return undefined();
    int64_t num, den;
    if (!mulOverflow(a.m_num, b.m_den, &num) &&
        !mulOverflow(a.m_den, b.m_num, &den)) {
      return rational(num, den);
    }
  }
  double d = b.toDouble();
  if (d == 0.0) return undefined();
  return real(a.toDouble() / d);
}

Value Value::pow(const Value& a, const Value& b) {
  if (a.m_undef || b.m_undef) return undefined();
  // Exact when the exponent is an integer in a reasonable range.
  if (a.isExact() && b.isExact() && b.m_den == 1 && b.m_num <= 63 &&
      b.m_num >= -63) {
    int64_t e = b.m_num;
    bool negativeExponent = e < 0;
    if (negativeExponent) e = -e;
    int64_t num = 1, den = 1;
    bool overflow = false;
    for (int64_t i = 0; i < e && !overflow; i++) {
      overflow = mulOverflow(num, a.m_num, &num) ||
                 mulOverflow(den, a.m_den, &den);
    }
    if (!overflow) {
      Value result = rational(num, den);
      return negativeExponent ? result.inverse() : result;
    }
    return real(::pow(a.toDouble(), b.toDouble()));
  }
  double base = a.toDouble();
  double exp = b.toDouble();
  if (base < 0.0 && b.m_den != 1) return undefined();  // complex result
  return real(::pow(base, exp));
}

Value Value::sqrtValue(const Value& a) {
  if (a.m_undef) return undefined();
  if (a.isExact() && a.m_num >= 0) {
    int64_t sn = isqrt64(a.m_num);
    int64_t sd = isqrt64(a.m_den);
    if (sn >= 0 && sd > 0 && sn * sn == a.m_num && sd * sd == a.m_den) {
      return rational(sn, sd);
    }
  }
  double d = a.toDouble();
  if (d < 0.0) return undefined();
  return real(sqrt(d));
}

Value Value::parse(const char* text) {
  if (text == nullptr || text[0] == '\0') return undefined();

  bool hasFractionalOrExponent = false;
  for (const char* p = text; *p; p++) {
    if (*p == '.' || *p == 'e' || *p == 'E') {
      hasFractionalOrExponent = true;
      break;
    }
  }

  if (!hasFractionalOrExponent) {
    // Try to keep it an exact integer (unless it overflows int64).
    errno = 0;
    char* end = nullptr;
    long long v = strtoll(text, &end, 10);
    if (errno == 0 && end != nullptr && *end == '\0') {
      return integer((int64_t)v);
    }
  }

  char* end = nullptr;
  double d = strtod(text, &end);
  if (end == text) return undefined();
  return real(d);
}

int Value::format(char* buffer, int bufferSize) const {
  if (bufferSize <= 0) return 0;
  if (m_undef) {
    return snprintf(buffer, bufferSize, "undef");
  }
  if (m_exact) {
    if (m_den == 1) {
      return snprintf(buffer, bufferSize, "%lld", (long long)m_num);
    }
    return snprintf(buffer, bufferSize, "%lld/%lld", (long long)m_num,
                    (long long)m_den);
  }
  return snprintf(buffer, bufferSize, "%.7g", m_approx);
}
