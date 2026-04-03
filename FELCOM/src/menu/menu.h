#ifndef MENU_H
#define MENU_H

enum appMode {MODE_MENU, MODE_CHAT, MODE_RFTEST, MODE_PONG};

void menuSetup();
void menuLoop();

#endif