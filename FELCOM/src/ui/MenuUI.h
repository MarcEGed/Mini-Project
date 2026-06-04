#ifndef MENU_UI_H
#define MENU_UI_H

#include <stdint.h>

enum MenuModeSelection : uint8_t {
    MenuChat = 0,
    MenuPong = 1,
    MenuRFTest = 2,
    MenuSyncTest = 3,
    MenuAbout = 4,
};

void menuUIInitDisplay();
void menuUIUpdate();

bool menuUIUpdateSelection(int8_t dirY);
MenuModeSelection menuUIGetSelection();

#endif
