extern "C" {
#include <stdint.h>
}

#include "eadkpp.h"
#include "keymap.h"
#include "menu.h"
#include "rpn.h"
#include "view.h"

// Required EADK app metadata.
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

using EADK::Keyboard::Event;

int main(int argc, char* argv[]) {
  (void)argc;
  (void)argv;

  Engine engine;
  View view;

  bool menuOpen = false;
  int menuSelection = 0;

  view.draw(engine, menuOpen, menuSelection);

  while (true) {
    int32_t timeout = 10000;
    Event event = EADK::Keyboard::getEvent(&timeout);

    if (menuOpen) {
      // Toolbox pop-up navigation.
      if (event == Event::Up) {
        menuSelection =
            (menuSelection + kMenuEntryCount - 1) % kMenuEntryCount;
      } else if (event == Event::Down) {
        menuSelection = (menuSelection + 1) % kMenuEntryCount;
      } else if (event == Event::Ok || event == Event::Exe) {
        engine.execute(kMenuEntries[menuSelection].command);
        menuOpen = false;
      } else if (event == Event::Toolbox || event == Event::Back) {
        menuOpen = false;
      }
      view.draw(engine, menuOpen, menuSelection);
      continue;
    }

    if (event == Event::Toolbox) {
      menuOpen = true;
      menuSelection = 0;
      view.draw(engine, menuOpen, menuSelection);
      continue;
    }
    if (event == Event::Back) {
      // Back is handled by the OS to leave the app; nothing to do here.
      continue;
    }

    const char* text = Keymap::textForEvent(event);
    if (text != nullptr) {
      engine.appendText(text);
    } else {
      Command command = Keymap::commandForEvent(event);
      if (command != Command::None) {
        engine.execute(command);
      }
    }

    view.draw(engine, menuOpen, menuSelection);
  }
}
