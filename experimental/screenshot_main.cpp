// Screenshot build (native simulator only): prefill a stack of representative
// values, draw once, then dump the framebuffer as a PPM to /out/shot.ppm.
extern "C" {
#include <stdint.h>
#include <stdio.h>
}

#include "../src/eadkpp.h"
#include "../src/rpn.h"
#include "../src/view.h"

const char eadk_app_name[]
#if PLATFORM_DEVICE
    __attribute__((section(".rodata.eadk_app_name")))
#endif
    = "RPN";
const uint32_t eadk_api_level
#if PLATFORM_DEVICE
    __attribute__((section(".rodata.eadk_api_level")))
#endif
    = 0;

static void feed(Engine& e, const char* digits) {
  for (const char* p = digits; *p; p++) {
    char s[2] = {*p, 0};
    e.appendText(s);
  }
}

int main(int argc, char* argv[]) {
  (void)argc;
  (void)argv;
  Engine e;
  // √(1+√2) — nested radical (new: exact)
  feed(e, "1"); e.execute(Command::EnterOrDup);
  feed(e, "2"); e.execute(Command::Sqrt); e.execute(Command::Add); e.execute(Command::Sqrt);
  // 1/(1+√2) = √2 − 1 — conjugate division (new: exact)
  feed(e, "1"); e.execute(Command::EnterOrDup);
  feed(e, "2"); e.execute(Command::Sqrt); e.execute(Command::Add); e.execute(Command::Inv);
  // (1+√2)² = 3 + 2√2
  feed(e, "1"); e.execute(Command::EnterOrDup);
  feed(e, "2"); e.execute(Command::Sqrt); e.execute(Command::Add);
  feed(e, "2"); e.execute(Command::Pow);
  // π/2
  e.execute(Command::PushPi);
  feed(e, "2"); e.execute(Command::Div);

  View view;
  view.draw(e, false, 0);

  const int w = 320, h = 240;
  static eadk_color_t buf[320 * 240];
  eadk_rect_t full = {0, 0, w, h};
  eadk_display_pull_rect(full, buf);
  FILE* f = fopen("/out/shot.ppm", "wb");
  if (f) {
    fprintf(f, "P6\n%d %d\n255\n", w, h);
    for (int i = 0; i < w * h; i++) {
      uint16_t c = buf[i];
      unsigned char rgb[3] = {
          static_cast<unsigned char>(((c >> 11) & 0x1F) << 3),
          static_cast<unsigned char>(((c >> 5) & 0x3F) << 2),
          static_cast<unsigned char>((c & 0x1F) << 3)};
      fwrite(rgb, 1, 3, f);
    }
    fclose(f);
  }
  return 0;
}
