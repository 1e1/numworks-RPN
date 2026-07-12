#include "input_field.h"

extern "C" {
#include <string.h>
}

void InputField::clear() {
  m_length = 0;
  m_text[0] = '\0';
}

bool InputField::append(const char* text) {
  if (text == nullptr) return false;
  int added = (int)strlen(text);
  if (added == 0) return false;
  if (m_length + added >= Capacity) return false;
  memcpy(m_text + m_length, text, added);
  m_length += added;
  m_text[m_length] = '\0';
  return true;
}

bool InputField::backspace() {
  if (m_length == 0) return false;
  m_length--;
  m_text[m_length] = '\0';
  return true;
}
