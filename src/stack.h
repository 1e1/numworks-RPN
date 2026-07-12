#pragma once

#include "value.h"

/* Fixed-capacity RPN stack. Level 1 is the top (last pushed), which is the one
 * displayed just above the input line, HP-48 style. */
class Stack {
 public:
  static constexpr int Capacity = 64;

  Stack() : m_depth(0) {}

  int depth() const { return m_depth; }
  bool isEmpty() const { return m_depth == 0; }
  bool isFull() const { return m_depth == Capacity; }

  // level starts at 1 (top). Out-of-range returns an undefined value.
  const Value& peek(int level) const;

  bool push(const Value& v);
  Value pop();  // undefined if empty

  bool dup();
  bool drop();
  bool swap();
  bool over();
  bool rot();       // level 3 -> level 1
  bool rollUp();    // top goes to the bottom
  bool rollDown();  // bottom comes to the top
  bool pick(int level);
  void clear() { m_depth = 0; }

 private:
  Value m_values[Capacity];  // index 0 is the bottom, m_depth-1 is the top
  int m_depth;
};
