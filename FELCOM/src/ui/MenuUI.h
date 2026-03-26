#ifndef MENU_UI_H
#define MENU_UI_H

#include <stdint.h>

enum MenuModeSelection : uint8_t {
    MenuPing = 0,
    MenuChat = 1,
};

void menuUIInitDisplay();
bool menuUIUpdateSelection(int8_t dirY);
MenuModeSelection menuUIGetSelection();

#endif
