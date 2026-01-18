#ifndef FONT_H
#define FONT_H

#include <stdint.h>

typedef uint8_t Glyph6[6][6];

extern const Glyph6 FONT[128];

extern const char startMsg[];
extern const char overMsg[];

#endif
