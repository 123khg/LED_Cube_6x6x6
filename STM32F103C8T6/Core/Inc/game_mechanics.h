#ifndef GAME_MECHANICS_H
#define GAME_MECHANICS_H

#include "config.h"

// ================= Game Setup =================
#define RNG(start, end) rand() % (end - start) + start
#define constrain(amt, low, high) ((amt) < (low) ? (low) : ((amt) > (high) ? (high) : (amt)))
void clearCanvas(uint8_t (*p)[6][6]); // use memset(&ptr, uint8_t val, size_t size);
/*
FYI, i spent literally 2 hours asking ChatGPT and learning about pointers, arrays
and how an array is decayed down to a pointer like this:
void clearCanvas(uint8_t canvas[6][6][6]); is compiled as void clearCanvas(uint8_t (*p)[6][6]);
This is because when passing clearCanvas(gameCanvas), the root of an "array" is literally
a pointer to its first element since an array is a physically known memory slot.

So when implementing memset, there are 2 valid ways:

void clearCanvas(uint8_t (*p)[6][6]) { // Array-to-pointer decay
	memset(p, 0, 6 * sizeof(*p));
}
clearCanvas(gameCanvas); // I prefer this much more

Or:

void clearCanvas(uint8_t canvas[6][6][6], size_t size) {
	memset(canvas, 0, sizeof(canvas)); // This is wrong
	// Since "canvas" is a pointer, not an object
	// It returns "8" for a pointer is a byte
	memset(canvas, 0, size); // This is correct
}
clearCanvas(gameCanvas, sizeof(gameCanvas));
 */
void gameSetup(void);
void initSnake(void);
void initFood(void);

// ================= Input & Update =================
void getInput(void);
void updateGameState(void);

// ================= Draw to Canvas =================
void clearSnake(void); // use memset(&ptr, uint8_t val, size_t size);
void renderSnake(void);
void renderFood(void);
void renderChar(void);

// ================= Firework Effects =================
void checkIdleEffectFinished(uint8_t iterations);
void fireworks_Normal();
void fireworks_Random();
void fireworks_Rain();
void fireworks_Frame();
void fireworks_Full();
void fireworks_CornerCube();
void showFireworks();

// ================= SPI & PWM Controls =================
// ================= SPI / PWM =================
void SPIByteMapping(
    const uint8_t layer[6][6],
    uint8_t out[6]
);

void PWMCalc(
    uint8_t phase,
    uint8_t layer[6][6],
    uint8_t out[6][6]
);

void SPIOutput(
    uint8_t data[6],
    int FETidx
);

void SPIControlHub(
    uint8_t canvas[6][6][6]
);

#endif
