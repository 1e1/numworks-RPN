#pragma once

/* Number of glyphs (Unicode code points) in a UTF-8 string: every byte that is
 * not a 0b10xxxxxx continuation byte starts a new glyph. Used for layout width,
 * so the multi-byte √ (3 bytes) and π (2 bytes) each count as a single column. */
inline int utf8GlyphCount(const char* s) {
  int n = 0;
  for (const unsigned char* p = (const unsigned char*)s; *p; p++)
    if ((*p & 0xC0) != 0x80) n++;
  return n;
}
