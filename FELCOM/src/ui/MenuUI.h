#ifndef MENU_UI_H
#define MENU_UI_H

#include <stdint.h>

enum MenuModeSelection : uint8_t {
    MenuPong = 0,
    MenuChat = 1,
    MenuRFTest = 2,
};

void menuUIInitDisplay();
void menuUIUpdate();

bool menuUIUpdateSelection(int8_t dirY);
MenuModeSelection menuUIGetSelection();

#endif
