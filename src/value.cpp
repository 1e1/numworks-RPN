#include "value.h"

#include "layout.h"

extern "C" {
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
}

// ------------------------------------------------------------------ rationals
namespace {
bool gInexact;  // set on arena OR integer overflow; the current op degrades to a double

long long gcd64(long long a, long long b) {
  if (a < 0) a = -a;
  if (b < 0) b = -b;
  while (b) { long long t = a % b; a = b; b = t; }
  return a;
}
bool mulOverflow(long long a, long long b, long long* o) { return __builtin_mul_overflow(a, b, o); }
bool addOverflow(long long a, long long b, long long* o) { return __builtin_add_overflow(a, b, o); }
struct Rat { long long n, d; };
Rat makeRat(long long n, long long d) {
  if (d < 0) { n = -n; d = -d; }
  long long g = gcd64(n, d);
  if (g) { n /= g; d /= g; }
  return Rat{n, d};
}
Rat ratMul(Rat a, Rat b) {
  long long n, d;
  if (mulOverflow(a.n, b.n, &n) || mulOverflow(a.d, b.d, &d)) { gInexact = true; return Rat{0, 1}; }
  return makeRat(n, d);
}
Rat ratAdd(Rat a, Rat b) {
  long long l, r, n, d;
  if (mulOverflow(a.n, b.d, &l) || mulOverflow(b.n, a.d, &r) || addOverflow(l, r, &n) || mulOverflow(a.d, b.d, &d)) {
    gInexact = true;
    return Rat{0, 1};
  }
  return makeRat(n, d);
}
Rat ratNeg(Rat a) { return Rat{-a.n, a.d}; }
Rat ratSub(Rat a, Rat b) { return ratAdd(a, ratNeg(b)); }
Rat ratInv(Rat a) { return makeRat(a.d, a.n); }

// ------------------------------------------------------------------ arena
//
// Representation. An exact value is a polynomial in "atoms":
//
//     Poly   = sum of Terms
//     Term   = a rational coefficient c times a product of Factors
//     Factor = an atom (Base) raised to an integer exponent
//     Base   = an irrational atom: π, a variable, √(Poly) or 1/(Poly)
//
// So `3 + 2√2` is a Poly of two Terms: {c=3, no factors} and
// {c=2, factors=[√2 ^1]}. Radicals and inverses nest because a Base can itself
// reference another Poly (its radicand / denominator), which is how √(1+√2) and
// 1/(1+√2) are held.
//
// Storage. The four node types live in four flat, pre-sized pools and refer to
// each other by index rather than by pointer — no heap, no std::vector. A
// Term names its factors by a [fstart, fstart+fcount) slice of gFactors; a Poly
// names its terms by a [tstart, tstart+tcount) slice of gTerms. Slots are handed
// out by the take*() bump allocators and reclaimed wholesale by collect().
struct Base { int kind; char var; int rad; };   // kind: 0=π 1=var 2=√ 3=1/·; rad = radicand/denominator Poly
struct Factor { int base; int exp; };            // gBases[base] ^ exp
struct Term { Rat c; int fstart; int fcount; };  // c · Π gFactors[fstart .. fstart+fcount)
struct Poly { int tstart; int tcount; };         // Σ gTerms[tstart .. tstart+tcount)

#if PLATFORM_DEVICE
constexpr int MAXB = 256, MAXF = 512, MAXT = 512, MAXP = 256, KT = 64;
#else
constexpr int MAXB = 8000, MAXF = 8000, MAXT = 8000, MAXP = 8000, KT = 256;
#endif
constexpr int KF = 8;  // factors per term

Base gBases[MAXB];
Factor gFactors[MAXF];
Term gTerms[MAXT];
Poly gPolys[MAXP];
int gBaseUsed, gFactorUsed, gTermUsed, gPolyUsed;  // high-water mark of each pool

// Bump-allocate one slot; on exhaustion flag gInexact so the op degrades to a
// double instead of writing out of bounds.
int takeBase() { if (gBaseUsed >= MAXB) { gInexact = true; return MAXB - 1; } return gBaseUsed++; }
int takeFactor() { if (gFactorUsed >= MAXF) { gInexact = true; return MAXF - 1; } return gFactorUsed++; }
int takeTerm() { if (gTermUsed >= MAXT) { gInexact = true; return MAXT - 1; } return gTermUsed++; }
int takePoly() { if (gPolyUsed >= MAXP) { gInexact = true; return MAXP - 1; } return gPolyUsed++; }

struct TmpTerm { Rat c; Factor f[KF]; int nf; };
void addFactor(TmpTerm& t, Factor f) {
  if (t.nf < KF) t.f[t.nf++] = f; else gInexact = true;
}

int allocBase(int kind, char var, int rad) {
  int i = takeBase();
  gBases[i] = Base{kind, var, rad};
  return i;
}

// ------------------------------------------------------------------ compare
int cmpPoly(int pa, int pb);
int cmpBase(int a, int b) {
  const Base& x = gBases[a];
  const Base& y = gBases[b];
  if (x.kind != y.kind) return x.kind < y.kind ? -1 : 1;
  if (x.kind == 1) return x.var == y.var ? 0 : (x.var < y.var ? -1 : 1);
  if (x.kind == 2 || x.kind == 3) return cmpPoly(x.rad, y.rad);
  return 0;
}
int cmpFactor(const Factor& a, const Factor& b) {
  int c = cmpBase(a.base, b.base);
  if (c) return c;
  return a.exp == b.exp ? 0 : (a.exp < b.exp ? -1 : 1);
}
int cmpSigFT(const Factor* fa, int na, const Factor* fb, int nb) {
  int n = na < nb ? na : nb;
  for (int i = 0; i < n; i++) { int c = cmpFactor(fa[i], fb[i]); if (c) return c; }
  if (na != nb) return na < nb ? -1 : 1;
  return 0;
}
int cmpTermRef(int ta, int gcBaseUsed) {
  const Term& x = gTerms[ta];
  const Term& y = gTerms[gcBaseUsed];
  int c = cmpSigFT(&gFactors[x.fstart], x.fcount, &gFactors[y.fstart], y.fcount);
  if (c) return c;
  if (x.c.n != y.c.n) return x.c.n < y.c.n ? -1 : 1;
  return x.c.d == y.c.d ? 0 : (x.c.d < y.c.d ? -1 : 1);
}
int cmpPoly(int pa, int pb) {
  const Poly& x = gPolys[pa];
  const Poly& y = gPolys[pb];
  int n = x.tcount < y.tcount ? x.tcount : y.tcount;
  for (int i = 0; i < n; i++) { int c = cmpTermRef(x.tstart + i, y.tstart + i); if (c) return c; }
  if (x.tcount != y.tcount) return x.tcount < y.tcount ? -1 : 1;
  return 0;
}

// ------------------------------------------------------------------ normalize
int polyRat(Rat c);

long long intRadicand(int baseIdx) {
  const Base& b = gBases[baseIdx];
  if (b.kind != 2) return -1;
  const Poly& p = gPolys[b.rad];
  if (p.tcount != 1) return -1;
  const Term& t = gTerms[p.tstart];
  if (t.fcount != 0 || t.c.d != 1 || t.c.n < 0) return -1;
  return t.c.n;
}
void sortFactors(Factor* f, int n) {
  for (int i = 1; i < n; i++) {
    Factor key = f[i];
    int j = i - 1;
    while (j >= 0 && cmpFactor(f[j], key) > 0) { f[j + 1] = f[j]; j--; }
    f[j + 1] = key;
  }
}
void simplifyTmp(TmpTerm& t) {
  sortFactors(t.f, t.nf);
  Factor out[KF];
  int no = 0;
  for (int i = 0; i < t.nf; i++) {
    if (no > 0 && cmpBase(out[no - 1].base, t.f[i].base) == 0) out[no - 1].exp += t.f[i].exp;
    else if (no < KF) out[no++] = t.f[i];
    else gInexact = true;
  }
  t.nf = 0;
  for (int i = 0; i < no; i++) {
    Factor f = out[i];
    long long r = intRadicand(f.base);
    if (r >= 1) {
      while (f.exp >= 2) { t.c = ratMul(t.c, makeRat(r, 1)); f.exp -= 2; }
      while (f.exp <= -2) { t.c = ratMul(t.c, makeRat(1, r)); f.exp += 2; }
      if (f.exp == -1) { t.c = ratMul(t.c, makeRat(1, r)); f.exp = 1; }
    }
    if (f.exp != 0) t.f[t.nf++] = f;
  }
  // merge distinct integer radicals: √a·√b = √(ab), then extract squares
  long long radProd = 1;
  Factor keep[KF];
  int nk = 0;
  for (int i = 0; i < t.nf; i++) {
    long long r = intRadicand(t.f[i].base);
    if (r >= 1 && t.f[i].exp == 1) { if (mulOverflow(radProd, r, &radProd)) gInexact = true; }
    else keep[nk++] = t.f[i];
  }
  if (radProd > 1) {
    long long mult = 1, rest = radProd;
    for (long long k = 2; k * k <= rest; k++)
      while (rest % (k * k) == 0) { rest /= k * k; mult *= k; }
    t.c = ratMul(t.c, makeRat(mult, 1));
    if (rest > 1 && nk < KF) keep[nk++] = Factor{allocBase(2, 0, polyRat(makeRat(rest, 1))), 1};
  }
  for (int i = 0; i < nk; i++) t.f[i] = keep[i];
  t.nf = nk;
  sortFactors(t.f, t.nf);
}
int commit(TmpTerm* ts, int n) {
  for (int i = 0; i < n; i++) simplifyTmp(ts[i]);
  for (int i = 1; i < n; i++) {
    TmpTerm key = ts[i];
    int j = i - 1;
    while (j >= 0 && cmpSigFT(ts[j].f, ts[j].nf, key.f, key.nf) > 0) { ts[j + 1] = ts[j]; j--; }
    ts[j + 1] = key;
  }
  int poly = takePoly();
  gPolys[poly].tstart = gTermUsed;
  int tc = 0, i = 0;
  while (i < n) {
    Rat c = ts[i].c;
    int k = i + 1;
    while (k < n && cmpSigFT(ts[i].f, ts[i].nf, ts[k].f, ts[k].nf) == 0) { c = ratAdd(c, ts[k].c); k++; }
    if (c.n != 0) {
      int fstart = gFactorUsed;
      for (int j = 0; j < ts[i].nf; j++) { int fi = takeFactor(); gFactors[fi] = ts[i].f[j]; }
      int ti = takeTerm();
      gTerms[ti] = Term{c, fstart, ts[i].nf};
      tc++;
    }
    i = k;
  }
  gPolys[poly].tcount = tc;
  return poly;
}
int polyRat(Rat c) {
  TmpTerm t; t.c = c; t.nf = 0;
  return commit(&t, (c.n == 0) ? 0 : 1);
}
void loadTerm(int termIdx, TmpTerm& out) {
  const Term& t = gTerms[termIdx];
  out.c = t.c;
  out.nf = 0;
  for (int i = 0; i < t.fcount; i++) addFactor(out, gFactors[t.fstart + i]);
}
}  // namespace

// ------------------------------------------------------------------ ValueImpl
struct ValueImpl {
  static int add(int pa, int pb) {
    TmpTerm ts[KT];
    int n = 0;
    const Poly& A = gPolys[pa];
    const Poly& B = gPolys[pb];
    for (int i = 0; i < A.tcount && n < KT; i++) loadTerm(A.tstart + i, ts[n++]);
    for (int i = 0; i < B.tcount && n < KT; i++) loadTerm(B.tstart + i, ts[n++]);
    if (A.tcount + B.tcount > KT) gInexact = true;
    return commit(ts, n);
  }
  static int mul(int pa, int pb) {
    TmpTerm ts[KT];
    int n = 0;
    const Poly& A = gPolys[pa];
    const Poly& B = gPolys[pb];
    for (int i = 0; i < A.tcount; i++)
      for (int j = 0; j < B.tcount; j++) {
        if (n >= KT) { gInexact = true; return commit(ts, n); }
        const Term& a = gTerms[A.tstart + i];
        const Term& b = gTerms[B.tstart + j];
        TmpTerm& t = ts[n++];
        t.c = ratMul(a.c, b.c);
        t.nf = 0;
        for (int k = 0; k < a.fcount; k++) addFactor(t, gFactors[a.fstart + k]);
        for (int k = 0; k < b.fcount; k++) addFactor(t, gFactors[b.fstart + k]);
      }
    return commit(ts, n);
  }
  static int powN(int p, int n) {
    int r = polyRat(makeRat(1, 1));
    for (int i = 0; i < n && !gInexact; i++) r = mul(r, p);
    return r;
  }
  static int inverse(int p) {
    const Poly& P = gPolys[p];
    if (P.tcount == 0) return polyRat(makeRat(0, 1));
    // Single term: invert the coefficient and negate every exponent
    // (1/(k·√m) = (1/k)·√m^-1), which the simplifier rationalizes.
    if (P.tcount == 1) {
      TmpTerm t;
      loadTerm(P.tstart, t);
      t.c = ratInv(t.c);
      for (int i = 0; i < t.nf; i++) t.f[i].exp = -t.f[i].exp;
      return commit(&t, 1);
    }
    // Two terms of the form a + b·√r: rationalize with the conjugate,
    //   1/(a + b√r) = (a - b√r) / (a² - b²r),
    // which stays exact whenever the denominator a² - b²r is non-zero.
    if (P.tcount == 2) {
      int rIdx = -1, sIdx = -1;  // indices of the rational term and the b√r term
      long long r = -1;          // the radicand under the square root
      for (int i = 0; i < 2; i++) {
        const Term& t = gTerms[P.tstart + i];
        if (t.fcount == 0) rIdx = P.tstart + i;
        else if (t.fcount == 1 && gFactors[t.fstart].exp == 1) {
          long long ri = intRadicand(gFactors[t.fstart].base);
          if (ri >= 1) { sIdx = P.tstart + i; r = ri; }
        }
      }
      if (rIdx >= 0 && sIdx >= 0) {
        Rat a = gTerms[rIdx].c, b = gTerms[sIdx].c;
        Rat d = ratSub(ratMul(a, a), ratMul(ratMul(b, b), makeRat(r, 1)));  // a² - b²r
        if (d.n != 0) {
          TmpTerm ts[2];
          ts[0].c = ratMul(a, ratInv(d)); ts[0].nf = 0;           //  a/d
          ts[1].c = ratMul(ratNeg(b), ratInv(d)); ts[1].nf = 1;   // -b/d · √r
          ts[1].f[0] = Factor{allocBase(2, 0, polyRat(makeRat(r, 1))), 1};
          return commit(ts, 2);
        }
      }
    }
    // Anything else: keep it exact but unevaluated as the atom 1/(p).
    TmpTerm t;
    t.c = makeRat(1, 1); t.nf = 1;
    t.f[0] = Factor{allocBase(3, 0, p), 1};
    return commit(&t, 1);
  }
  static int sqrtPoly(int p) {
    const Poly& P = gPolys[p];
    if (P.tcount == 1 && gTerms[P.tstart].fcount == 0) {
      Rat c = gTerms[P.tstart].c;
      if (c.n >= 0) {
        long long radicand;
        if (mulOverflow(c.n, c.d, &radicand)) { gInexact = true; return polyRat(makeRat(0, 1)); }
        long long mult = 1, rest = radicand;
        for (long long k = 2; k * k <= rest; k++)
          while (rest % (k * k) == 0) { rest /= k * k; mult *= k; }
        if (rest == 1) return polyRat(makeRat(mult, c.d));
        TmpTerm t;
        t.c = makeRat(mult, c.d); t.nf = 1;
        t.f[0] = Factor{allocBase(2, 0, polyRat(makeRat(rest, 1))), 1};
        return commit(&t, 1);
      }
    }
    TmpTerm t;
    t.c = makeRat(1, 1); t.nf = 1;
    t.f[0] = Factor{allocBase(2, 0, p), 1};
    return commit(&t, 1);
  }
};

// ------------------------------------------------------ numeric evaluation
namespace {
double evalPoly(int p);
double evalBase(int baseIdx) {
  const Base& b = gBases[baseIdx];
  if (b.kind == 0) return 3.14159265358979323846;
  if (b.kind == 1) return NAN;
  if (b.kind == 2) return sqrt(evalPoly(b.rad));
  double d = evalPoly(b.rad);
  return d == 0.0 ? NAN : 1.0 / d;
}
double evalPoly(int p) {
  const Poly& P = gPolys[p];
  double sum = 0.0;
  for (int i = 0; i < P.tcount; i++) {
    const Term& t = gTerms[P.tstart + i];
    double v = (double)t.c.n / (double)t.c.d;
    for (int k = 0; k < t.fcount; k++) {
      const Factor& f = gFactors[t.fstart + k];
      v *= pow(evalBase(f.base), (double)f.exp);
    }
    sum += v;
  }
  return sum;
}
bool isZeroPoly(int p) { return gPolys[p].tcount == 0; }

// ------------------------------------------------------ garbage collection
//
// The pools never free individual nodes; instead a copying collector compacts
// them. gcCopy() copies one Poly (and everything it reaches) into a second set
// of pools, and collect() swaps the compacted copy back in. gcForward maps an
// old Poly index to its copy so shared sub-expressions are copied once and
// cycles are impossible (the tree is acyclic and built bottom-up).
Base gcBases[MAXB];
Factor gcFactors[MAXF];
Term gcTerms[MAXT];
Poly gcPolys[MAXP];
int gcBaseUsed, gcFactorUsed, gcTermUsed, gcPolyUsed;
int gcForward[MAXP];  // old Poly index -> new index, or -1 if not copied yet

int gcCopy(int p) {
  if (gcForward[p] >= 0) return gcForward[p];  // already copied
  const Poly& P = gPolys[p];
  // Copy nested radicands / denominators first, so their forwarded indices are
  // known by the time we reference them below (children before parents).
  for (int i = 0; i < P.tcount; i++) {
    const Term& t = gTerms[P.tstart + i];
    for (int k = 0; k < t.fcount; k++) {
      const Base& b = gBases[gFactors[t.fstart + k].base];
      if (b.kind == 2 || b.kind == 3) gcCopy(b.rad);
    }
  }
  int tstart = gcTermUsed;
  for (int i = 0; i < P.tcount; i++) {
    const Term& t = gTerms[P.tstart + i];
    int fstart = gcFactorUsed;
    for (int k = 0; k < t.fcount; k++) {
      const Factor& f = gFactors[t.fstart + k];
      const Base& b = gBases[f.base];
      int newBase = gcBaseUsed++;
      gcBases[newBase] = Base{b.kind, b.var, (b.kind == 2 || b.kind == 3) ? gcForward[b.rad] : -1};
      gcFactors[gcFactorUsed++] = Factor{newBase, f.exp};
    }
    gcTerms[gcTermUsed++] = Term{t.c, fstart, t.fcount};
  }
  int newPoly = gcPolyUsed++;
  gcPolys[newPoly] = Poly{tstart, P.tcount};
  gcForward[p] = newPoly;
  return newPoly;
}
}  // namespace

// ------------------------------------------------------------------ API
Value::Value() : m_poly(polyRat(makeRat(0, 1))), m_real(0.0), m_kind(0) {}
Value Value::integer(long long n) { return Value(polyRat(makeRat(n, 1))); }
Value Value::rational(long long n, long long d) {
  if (d == 0) return undefined();
  return Value(polyRat(makeRat(n, d)));
}
Value Value::real(double x) { Value e; e.m_kind = 1; e.m_real = x; return e; }
Value Value::undefined() { Value e; e.m_kind = 2; return e; }
Value Value::pi() {
  TmpTerm t; t.c = makeRat(1, 1); t.nf = 1; t.f[0] = Factor{allocBase(0, 0, -1), 1};
  return Value(commit(&t, 1));
}
Value Value::var(char name) {
  TmpTerm t; t.c = makeRat(1, 1); t.nf = 1; t.f[0] = Factor{allocBase(1, name, -1), 1};
  return Value(commit(&t, 1));
}

Value Value::parse(const char* text) {
  if (text == nullptr || text[0] == '\0') return undefined();
  bool fractional = false;
  for (const char* p = text; *p; p++)
    if (*p == '.' || *p == 'e' || *p == 'E') { fractional = true; break; }
  if (!fractional) {
    errno = 0;
    char* end = nullptr;
    long long v = strtoll(text, &end, 10);
    if (errno == 0 && end != nullptr && *end == '\0') return integer(v);
  }
  char* end = nullptr;
  double d = strtod(text, &end);
  if (end == text) return undefined();
  return real(d);
}

bool Value::isInteger() const {
  if (m_kind != 0) return false;
  const Poly& P = gPolys[m_poly];
  if (P.tcount == 0) return true;  // zero
  return P.tcount == 1 && gTerms[P.tstart].fcount == 0 && gTerms[P.tstart].c.d == 1;
}
long long Value::integerValue() const {
  if (m_kind != 0 || gPolys[m_poly].tcount == 0) return 0;
  return gTerms[gPolys[m_poly].tstart].c.n;
}
double Value::toDouble() const {
  if (m_kind == 2) return NAN;
  if (m_kind == 1) return m_real;
  return evalPoly(m_poly);
}

Value Value::negated() const {
  if (m_kind == 2) return *this;
  if (m_kind == 1) return real(-m_real);
  return mul(integer(-1), *this);
}
Value Value::inverse() const {
  if (m_kind == 2) return undefined();
  if (m_kind == 0) {
    if (isZeroPoly(m_poly)) return undefined();
    gInexact = false;
    int p = ValueImpl::inverse(m_poly);
    if (gInexact) return real(1.0 / toDouble());
    return Value(p);
  }
  return m_real == 0.0 ? undefined() : real(1.0 / m_real);
}

Value Value::add(const Value& a, const Value& b) {
  if (a.m_kind == 2 || b.m_kind == 2) return undefined();
  if (a.m_kind == 0 && b.m_kind == 0) {
    gInexact = false;
    int p = ValueImpl::add(a.m_poly, b.m_poly);
    if (gInexact) return real(a.toDouble() + b.toDouble());
    return Value(p);
  }
  return real(a.toDouble() + b.toDouble());
}
Value Value::sub(const Value& a, const Value& b) { return add(a, b.negated()); }
Value Value::mul(const Value& a, const Value& b) {
  if (a.m_kind == 2 || b.m_kind == 2) return undefined();
  if (a.m_kind == 0 && b.m_kind == 0) {
    gInexact = false;
    int p = ValueImpl::mul(a.m_poly, b.m_poly);
    if (gInexact) return real(a.toDouble() * b.toDouble());
    return Value(p);
  }
  return real(a.toDouble() * b.toDouble());
}
Value Value::div(const Value& a, const Value& b) {
  if (a.m_kind == 2 || b.m_kind == 2) return undefined();
  if (a.m_kind == 0 && b.m_kind == 0) {
    if (isZeroPoly(b.m_poly)) return undefined();
    gInexact = false;
    int p = ValueImpl::mul(a.m_poly, ValueImpl::inverse(b.m_poly));
    if (gInexact) return real(a.toDouble() / b.toDouble());
    return Value(p);
  }
  double d = b.toDouble();
  return d == 0.0 ? undefined() : real(a.toDouble() / d);
}
Value Value::pow(const Value& a, const Value& b) {
  if (a.m_kind == 2 || b.m_kind == 2) return undefined();
  if (a.m_kind == 0 && b.isInteger()) {
    long long n = b.integerValue();
    if (n > 64 || n < -64) return real(::pow(a.toDouble(), (double)n));
    gInexact = false;
    int p = (n >= 0) ? ValueImpl::powN(a.m_poly, (int)n)
                     : (isZeroPoly(a.m_poly) ? -1 : ValueImpl::inverse(ValueImpl::powN(a.m_poly, (int)-n)));
    if (p < 0) return undefined();
    if (gInexact) return real(::pow(a.toDouble(), (double)n));
    return Value(p);
  }
  double base = a.toDouble();
  if (base < 0.0 && !b.isInteger()) return undefined();
  return real(::pow(base, b.toDouble()));
}
Value Value::sqrtValue(const Value& a) {
  if (a.m_kind == 2) return undefined();
  double d = a.toDouble();
  if (d < 0.0) return undefined();
  if (a.m_kind == 0) {
    gInexact = false;
    int p = ValueImpl::sqrtPoly(a.m_poly);
    if (gInexact) return real(::sqrt(d));
    return Value(p);
  }
  return real(::sqrt(d));
}

int Value::arenaUsage() { return gPolyUsed; }
// True once any pool is more than 3/4 full — the cue for the engine to collect.
bool Value::arenaNearFull() {
  return gPolyUsed * 4 > MAXP * 3 || gTermUsed * 4 > MAXT * 3 || gFactorUsed * 4 > MAXF * 3 ||
         gBaseUsed * 4 > MAXB * 3;
}
// Compact the arena, keeping only what the given roots reach. Each live root's
// m_poly is rewritten to its new index; everything unreachable is dropped.
void Value::collect(Value* roots, int count) {
  gcBaseUsed = gcFactorUsed = gcTermUsed = gcPolyUsed = 0;
  for (int i = 0; i < gPolyUsed; i++) gcForward[i] = -1;
  for (int i = 0; i < count; i++)
    if (roots[i].m_kind == 0) roots[i].m_poly = gcCopy(roots[i].m_poly);
  for (int i = 0; i < gcBaseUsed; i++) gBases[i] = gcBases[i];
  for (int i = 0; i < gcFactorUsed; i++) gFactors[i] = gcFactors[i];
  for (int i = 0; i < gcTermUsed; i++) gTerms[i] = gcTerms[i];
  for (int i = 0; i < gcPolyUsed; i++) gPolys[i] = gcPolys[i];
  gBaseUsed = gcBaseUsed; gFactorUsed = gcFactorUsed; gTermUsed = gcTermUsed; gPolyUsed = gcPolyUsed;
}

// ------------------------------------------------------------------ printing
namespace {
struct Cur { char* b; int size; int pos; };
void put(Cur& c, const char* s) { while (*s && c.pos < c.size - 1) c.b[c.pos++] = *s++; c.b[c.pos] = 0; }
void putI(Cur& c, long long v) { char t[24]; snprintf(t, sizeof(t), "%lld", v); put(c, t); }
void polyFmt(Cur& c, int p);
void baseFmt(Cur& c, int baseIdx) {
  const Base& b = gBases[baseIdx];
  if (b.kind == 0) { put(c, "\xcf\x80"); return; }
  if (b.kind == 1) { char s[2] = {b.var, 0}; put(c, s); return; }
  if (b.kind == 3) { put(c, "1/("); polyFmt(c, b.rad); put(c, ")"); return; }
  const Poly& r = gPolys[b.rad];
  if (r.tcount == 1 && gTerms[r.tstart].fcount == 0 && gTerms[r.tstart].c.d == 1) {
    put(c, "\xe2\x88\x9a"); putI(c, gTerms[r.tstart].c.n);
  } else { put(c, "\xe2\x88\x9a("); polyFmt(c, b.rad); put(c, ")"); }
}
void termFmt(Cur& c, int termIdx) {
  const Term& t = gTerms[termIdx];
  long long an = t.c.n < 0 ? -t.c.n : t.c.n;
  int posStart = c.pos;
  if (an != 1) putI(c, an);
  for (int i = 0; i < t.fcount; i++) {
    const Factor& f = gFactors[t.fstart + i];
    if (f.exp > 0) { baseFmt(c, f.base); if (f.exp > 1) { put(c, "^"); putI(c, f.exp); } }
  }
  if (c.pos == posStart) put(c, "1");
  bool hasDen = t.c.d != 1;
  for (int i = 0; i < t.fcount; i++) if (gFactors[t.fstart + i].exp < 0) hasDen = true;
  if (hasDen) {
    put(c, "/");
    int dStart = c.pos;
    if (t.c.d != 1) putI(c, t.c.d);
    for (int i = 0; i < t.fcount; i++) {
      const Factor& f = gFactors[t.fstart + i];
      if (f.exp < 0) { baseFmt(c, f.base); if (f.exp < -1) { put(c, "^"); putI(c, -f.exp); } }
    }
    if (c.pos == dStart) put(c, "1");
  }
}
void polyFmt(Cur& c, int p) {
  const Poly& P = gPolys[p];
  if (P.tcount == 0) { put(c, "0"); return; }
  for (int i = 0; i < P.tcount; i++) {
    bool neg = gTerms[P.tstart + i].c.n < 0;
    if (i == 0) { if (neg) put(c, "-"); }
    else put(c, neg ? " - " : " + ");
    termFmt(c, P.tstart + i);
  }
}
}  // namespace

int Value::format(char* buffer, int size) const {
  if (size <= 0) return 0;
  if (m_kind == 2) return snprintf(buffer, size, "undef");
  if (m_kind == 1) return snprintf(buffer, size, "%.7g", m_real);
  Cur c{buffer, size, 0};
  polyFmt(c, m_poly);
  return c.pos;
}

// ------------------------------------------------------------ 2D layout
namespace {
int layInt(Layout& L, long long v, bool large) { char b[24]; snprintf(b, sizeof(b), "%lld", v); return L.text(b, large); }
int layPoly(Layout& L, int p);
int layBase(Layout& L, int baseIdx) {
  const Base& b = gBases[baseIdx];
  if (b.kind == 0) return L.text("\xcf\x80", true);
  if (b.kind == 1) { char s[2] = {b.var, 0}; return L.text(s, true); }
  if (b.kind == 3) return L.frac(L.text("1", true), layPoly(L, b.rad));
  const Poly& r = gPolys[b.rad];
  if (r.tcount == 1 && gTerms[r.tstart].fcount == 0 && gTerms[r.tstart].c.d == 1)
    return L.radical(layInt(L, gTerms[r.tstart].c.n, true));
  return L.radical(layPoly(L, b.rad));
}
int layAtom(Layout& L, const Factor& f) {
  int base = layBase(L, f.base);
  int e = f.exp < 0 ? -f.exp : f.exp;
  return e > 1 ? L.superscript(base, layInt(L, e, false)) : base;
}
int layTerm(Layout& L, int termIdx) {
  const Term& t = gTerms[termIdx];
  long long an = t.c.n < 0 ? -t.c.n : t.c.n;
  int num[16], den[16], nn = 0, nd = 0;
  if (an != 1) num[nn++] = layInt(L, an, true);
  if (t.c.d != 1) den[nd++] = layInt(L, t.c.d, true);
  for (int i = 0; i < t.fcount; i++) {
    const Factor& f = gFactors[t.fstart + i];
    if (f.exp > 0 && nn < 16) num[nn++] = layAtom(L, f);
    else if (f.exp < 0 && nd < 16) den[nd++] = layAtom(L, f);
  }
  int top = nn == 0 ? L.text("1", true) : (nn == 1 ? num[0] : L.hbox(num, nn));
  if (nd == 0) return top;
  int bot = nd == 1 ? den[0] : L.hbox(den, nd);
  return L.frac(top, bot);
}
int layPoly(Layout& L, int p) {
  const Poly& P = gPolys[p];
  if (P.tcount == 0) return L.text("0", true);
  int kids[40], nk = 0;
  for (int i = 0; i < P.tcount && nk < 38; i++) {
    bool neg = gTerms[P.tstart + i].c.n < 0;
    if (i == 0) { if (neg) kids[nk++] = L.text("-", true); }
    else kids[nk++] = L.text(neg ? " - " : " + ", true);
    kids[nk++] = layTerm(L, P.tstart + i);
  }
  return nk == 1 ? kids[0] : L.hbox(kids, nk);
}
}  // namespace

int Value::buildLayout(Layout& L) const {
  if (m_kind == 2) return L.text("undef", true);
  if (m_kind == 1) { char b[32]; snprintf(b, sizeof(b), "%.7g", m_real); return L.text(b, true); }
  return layPoly(L, m_poly);
}
