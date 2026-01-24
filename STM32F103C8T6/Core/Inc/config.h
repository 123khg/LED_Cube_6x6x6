#ifndef CONFIG_H
#define CONFIG_H

#include "main.h"
#include <stdint.h>
#include <stdbool.h>
#include "stm32f1xx.h" // For TIM2

// ================= Timing =================
typedef struct {
    uint32_t interval; // millis/micros
    uint32_t time;
} Counter;

// ================= Coordinates =================
typedef struct {
    int x;
    int y;
    int z;
} Coords;

// ================= Directions =================
typedef enum {
    FORWARD,
    BACKWARD,
    RIGHT,
    LEFT,
    UP,
    DOWN
} Direction;

// ================= SPI Config =================
typedef struct {
    uint32_t clock;
    uint8_t  bitOrder;
    uint8_t  mode;
    uint8_t  fps;
    uint8_t  pwmRes;
    uint8_t  minDuty;
    uint8_t  maxBodyDuty;
} SPIConfig;

// ================= Dabble Config =================
typedef struct {
    bool forward;
    bool backward;
    bool left;
    bool right;
    bool up;
    bool down;
    bool start;
    bool select;
} DabbleInput_t;

// ================= Fireworks =================
typedef enum {
  NORMAL,
  RANDOM,
  RAIN,
  FRAME,
  FULL,
  CORNER,
  CLEAR
} FireworksEffects;

typedef struct {
    bool start;
    bool resetGameFlag;
    bool effectFlag;
    bool effectIdle;
    bool effectFinished;
    FireworksEffects  effectIdx;
} Fireworks;

// ================= Global Game State =================
// Timings
extern Counter spiCounter, dabble, game, foodCounter, fireworksCounter;
extern Counter gameDebugCounter, dabbleDebugCounter, serialDebugCounter;
#define micros() ((uint32_t)TIM2->CNT)

// I2C
extern uint8_t i2c_rx_buf[8];

// SPI
extern SPIConfig spi;
#define SPIx SPI1
extern uint8_t SPIdata[6];
extern uint16_t cycles;
typedef struct {
	GPIO_TypeDef* port;
	uint32_t pin;
} FET_GPIO;
extern FET_GPIO FET[6];

// Dabble
extern DabbleInput_t dabbleInput;

// In-game
extern bool gameStart;
extern bool gameOver;
extern int highscore;

// End game
extern uint8_t gameCanvas[6][6][6];
extern uint8_t labelCanvas[6][6][6];
extern uint8_t fireworksCanvas[6][6][6];
extern Fireworks fireworks;

// Snake
extern Coords snake[216];
extern Coords food;

extern int bodySize;
extern Direction snakeDir;
extern Direction bufferDir;

#endif
