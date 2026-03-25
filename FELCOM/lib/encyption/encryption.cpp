#include "encryption.h"

void xorCrypt(char* text, size_t len) {
    const uint8_t key[] = XOR_KEY;
    for (size_t i = 0; i < len; i++) text[i] ^= key[i % XOR_KEY_LEN];
}

void encrypt(char* text, size_t len) { xorCrypt(text, len); }

void decrypt(char* text, size_t len) { xorCrypt(text, len); }