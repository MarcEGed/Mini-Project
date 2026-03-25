#ifndef ENCRYPTION_H
#define ENCRYPTION_H

#include <config.h>
#include <stddef.h>

void encrypt(char* text, size_t len);
void decrypt(char* text, size_t len);

#endif