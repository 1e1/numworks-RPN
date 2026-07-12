#pragma once

/* The single-line text buffer the user types into before pushing to the stack.
 * ASCII only: digits, '.', '-', 'e' (exponent). */
class InputField {
 public:
  static constexpr int Capacity = 32;

  InputField() { clear(); }

  const char* text() const { return m_text; }
  int length() const { return m_length; }
  bool isEmpty() const { return m_length == 0; }

  void clear();
  bool append(const char* text);  // appends a short ASCII fragment
  bool backspace();               // removes the last character

 private:
  char m_text[Capacity];
  int m_length;
};
