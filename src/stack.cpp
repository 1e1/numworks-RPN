#include "stack.h"

const Value& Stack::peek(int level) const {
  static const Value undef = Value::undefined();
  if (level < 1 || level > m_depth) return undef;
  return m_values[m_depth - level];
}

bool Stack::push(const Value& v) {
  if (isFull()) return false;
  m_values[m_depth++] = v;
  return true;
}

Value Stack::pop() {
  if (isEmpty()) return Value::undefined();
  return m_values[--m_depth];
}

bool Stack::dup() {
  if (isEmpty() || isFull()) return false;
  m_values[m_depth] = m_values[m_depth - 1];
  m_depth++;
  return true;
}

bool Stack::drop() {
  if (isEmpty()) return false;
  m_depth--;
  return true;
}

bool Stack::swap() {
  if (m_depth < 2) return false;
  Value t = m_values[m_depth - 1];
  m_values[m_depth - 1] = m_values[m_depth - 2];
  m_values[m_depth - 2] = t;
  return true;
}

bool Stack::over() {
  if (m_depth < 2 || isFull()) return false;
  m_values[m_depth] = m_values[m_depth - 2];
  m_depth++;
  return true;
}

bool Stack::rot() {
  if (m_depth < 3) return false;
  Value third = m_values[m_depth - 3];
  m_values[m_depth - 3] = m_values[m_depth - 2];
  m_values[m_depth - 2] = m_values[m_depth - 1];
  m_values[m_depth - 1] = third;
  return true;
}

bool Stack::rollUp() {
  if (m_depth < 2) return false;
  Value top = m_values[m_depth - 1];
  for (int i = m_depth - 1; i > 0; i--) {
    m_values[i] = m_values[i - 1];
  }
  m_values[0] = top;
  return true;
}

bool Stack::rollDown() {
  if (m_depth < 2) return false;
  Value bottom = m_values[0];
  for (int i = 0; i < m_depth - 1; i++) {
    m_values[i] = m_values[i + 1];
  }
  m_values[m_depth - 1] = bottom;
  return true;
}

bool Stack::pick(int level) {
  if (level < 1 || level > m_depth || isFull()) return false;
  m_values[m_depth] = m_values[m_depth - level];
  m_depth++;
  return true;
}
