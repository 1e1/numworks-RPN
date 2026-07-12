#pragma once

#include "rpn.h"

/* Entries of the Toolbox pop-up: the lower-frequency stack operations that do
 * not deserve a dedicated key. Kept in one place so the view and the main loop
 * agree on the list. */
struct MenuEntry {
  const char* label;
  Command command;
};

static const MenuEntry kMenuEntries[] = {
    {"SWAP  (swap L1/L2)", Command::Swap},
    {"OVER  (copy L2)", Command::Over},
    {"ROT   (L3 to top)", Command::Rot},
    {"ROLL up", Command::RollUp},
    {"ROLL down", Command::RollDown},
    {"DUP   (copy L1)", Command::Dup},
    {"PICK  (input = level)", Command::Pick},
    {"1/x   (inverse)", Command::Inv},
    {"->Dec (approx L1)", Command::ToDecimal},
    {"CLEAR stack", Command::Clear},
    {"Angle: RAD / DEG", Command::ToggleAngle},
};

static constexpr int kMenuEntryCount =
    (int)(sizeof(kMenuEntries) / sizeof(kMenuEntries[0]));
