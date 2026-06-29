#ifndef MENU_UI_H
#define MENU_UI_H

#include <stdint.h>

enum MenuModeSelection : uint8_t {
    MenuSyncTest,
    MenuAudio,
    MenuChat,
    MenuPong,
    MenuRFTest,
    MenuAbout,
};

void menuUIInitDisplay();
void menuUIUpdate();

bool menuUIUpdateSelection(int8_t dirY);
MenuModeSelection menuUIGetSelection();

#endif
