#include "config.h"

// ================= Counters =================
Counter spiCounter, dabble, game, foodCounter, fireworksCounter; // Modify at setup()

// ================= SPI =================
SPIConfig spi = {
    .clock       = 4500000,
    .bitOrder    = 1,
    .mode        = 0,
    .fps         = 60,
    .pwmRes      = 8,
    .minDuty     = 5,
    .maxBodyDuty = 50
};
uint8_t SPIdata[6];
uint16_t cycles;

FET_GPIO FET[6] = {
	{FET0_GPIO_Port, FET0_Pin},
	{FET1_GPIO_Port, FET1_Pin},
	{FET2_GPIO_Port, FET2_Pin},
	{FET3_GPIO_Port, FET3_Pin},
	{FET4_GPIO_Port, FET4_Pin},
	{FET5_GPIO_Port, FET5_Pin},
};

// ================= Game State =================
bool gameStart = false;
bool gameOver = false;
int highscore;

uint8_t gameCanvas[6][6][6];
uint8_t labelCanvas[6][6][6];
uint8_t fireworksCanvas[6][6][6];
Fireworks fireworks;

Coords snake[216];
Coords food;

int bodySize = 3;
Direction snakeDir;
Direction bufferDir;

// ================= Fireworks =================
Fireworks fireworks = {0, 0, 0, 0, 0, 0};
