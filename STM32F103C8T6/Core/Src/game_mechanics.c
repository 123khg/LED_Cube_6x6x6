#include "game_mechanics.h"
#include "main.h"
#include <math.h>
#include <stdlib.h> // For rand() and srand()

/*
========================================================================
 NOTE ON REMOVED HARDWARE / FRAMEWORK CODE
========================================================================

The following Arduino / ESP32 features were REMOVED and MUST be re-implemented
by you on STM32 or another platform:

- Serial.*, String
- Dabble / GamePad
- pinMode, digitalWrite
- SPIClass, vspi->*
- micros(), uwTick
- RNG(), constrain()

They are replaced by TODO notes where applicable.

========================================================================
*/


//-------------------------------------------------------
//                    INITIALIZATION
//-------------------------------------------------------
void clearCanvas(uint8_t (*canvas)[6][6]) {
  memset(canvas, 0, 6 * sizeof(*canvas));
}

void initCounters(void) {
  // Timing
  spiCounter.interval = 1000000 / (spi.fps * 6); // micros
  game.interval = 600; // millis
  foodCounter.interval = 150; // millis
  fireworksCounter.interval = 250; // millis
  // These are for ESP, no need
//  dabble.interval = 2; // millis
//  gameDebugCounter.interval = dabbleDebugCounter.interval = 3000;
  serialDebugCounter.interval = 3000; // millis
//  STM32Counter.interval = 500; // millis


  // Initialize Counters
  cycles = 1U << spi.pwmRes;
  spiCounter.time = micros();
  game.time = uwTick;
  foodCounter.time = uwTick;
  fireworksCounter.time = uwTick;
  // These are for ESP, no need
//  dabble.time = uwTick;
//  gameDebugCounter.time = uwTick;
//  dabbleDebugCounter.time = uwTick;
  serialDebugCounter.time = uwTick;
}

void gameSetup(void) {
  srand(uwTick);
  // Setup fireworks
  clearCanvas(fireworksCanvas);
  // Setup labels
  clearCanvas(labelCanvas);
  // Setup game
  clearCanvas(gameCanvas);

  bodySize = 3;
  snakeDir = RNG(0, 3);

  initSnake();
  initFood();
  initCounters();
}

void initSnake(void) {
  snake[0].x = RNG(1,4);
  snake[0].y = RNG(1,4);
  snake[0].z = 0;

  Coords coordIncrease = {0, 0, 0}; // X, Y, Z
  if (snakeDir == FORWARD)  coordIncrease.x = -1;
  if (snakeDir == BACKWARD) coordIncrease.x = 1;
  if (snakeDir == RIGHT)    coordIncrease.y = 1;
  if (snakeDir == LEFT)     coordIncrease.y = -1;
  if (snakeDir == UP)       coordIncrease.z = -1;
  if (snakeDir == DOWN)     coordIncrease.z = 1;

  for (int i = 1; i < bodySize; i++) {
    snake[i].x = snake[i-1].x + i * coordIncrease.x;
    snake[i].y = snake[i-1].y + i * coordIncrease.y;
    snake[i].z = snake[i-1].z + i * coordIncrease.z;
  }
}

void initFood(void) {
  bool check;
  do {
    food.x = RNG(0, 5);
    food.y = RNG(0, 5);
    food.z = RNG(0, 5);
    check = false;

    for (int i = 0; i < bodySize; ++i) {
      if (snake[i].x == food.x &&
          snake[i].y == food.y &&
          snake[i].z == food.z) {
        check = true;
        break;
      }
    }
  } while (check);
}

//-------------------------------------------------------
//                    INPUT AND UPDATE
//-------------------------------------------------------

void getInput(void) {
  dabbleInput.forward  = i2c_rx_buf[0];
  dabbleInput.backward = i2c_rx_buf[1];
  dabbleInput.left     = i2c_rx_buf[2];
  dabbleInput.right    = i2c_rx_buf[3];
  dabbleInput.up       = i2c_rx_buf[4];
  dabbleInput.down     = i2c_rx_buf[5];
  dabbleInput.start    = i2c_rx_buf[6];
  dabbleInput.select   = i2c_rx_buf[7];

  /* Reset timer on button release */
//  if (!circlePressed)
//	  gameDebugCounter.time = uwTick;
//  if (!squarePressed)
//	  dabbleDebugCounter.time = uwTick;
  if (!dabbleInput.select)
	  serialDebugCounter.time = uwTick;

  /* Toggle gameDebug on sustained press */
//  if ((uwTick - gameDebugCounter.time) >= gameDebugCounter.interval) {
//	  gameDebug = !gameDebug;
//	  gameDebugCounter.time = uwTick;
//  }
//
//  /* Toggle dabbleDebug on sustained press */
//  if ((uwTick - dabbleDebugCounter.time) >= dabbleDebugCounter.interval) {
//	  dabbleDebug = !dabbleDebug;
//	  dabbleDebugCounter.time = uwTick;
//  }

  /* Toggle idle effects on sustained select press */
  if ((uwTick - serialDebugCounter.time) >= serialDebugCounter.interval) {
	  fireworks.effectIdle = true;
	  fireworks.effectIdx = NORMAL;
	  fireworks.effectFlag = true;
	  gameStart = false;
	  gameOver = true;
	  serialDebugCounter.time = uwTick;
  }

  // Start game
  if (dabbleInput.start && !gameStart) {
    gameStart = true;
    gameOver = false;
    fireworks.effectFlag = false;
    fireworks.effectIdle = false;
    fireworks.effectIdx  = NORMAL;
    game.time = uwTick;
  }

  // ============ Chống đi chéo =============
  int pressedCount = dabbleInput.up + dabbleInput.down +
		  dabbleInput.left + dabbleInput.right +
		  dabbleInput.forward + dabbleInput.backward;

  if (pressedCount != 1)
    return;

  // ============ xử lý hướng rắn ============
  if (dabbleInput.left && snakeDir != RIGHT)
      bufferDir = LEFT;
  else if (dabbleInput.right && snakeDir != LEFT)
      bufferDir = RIGHT;
  else if (dabbleInput.up && snakeDir != DOWN)
      bufferDir = UP;
  else if (dabbleInput.down && snakeDir != UP)
      bufferDir = DOWN;
  else if (dabbleInput.forward && snakeDir != BACKWARD)
      bufferDir = FORWARD;
  else if (dabbleInput.backward && snakeDir != FORWARD)
      bufferDir = BACKWARD;
}

void updateGameState(void) {
  Coords coordIncrease = {0, 0, 0}; // X, Y, Z
  snakeDir = bufferDir;

  if (snakeDir == FORWARD)  coordIncrease.x = 1;
  if (snakeDir == BACKWARD) coordIncrease.x = -1;
  if (snakeDir == RIGHT)    coordIncrease.y = -1;
  if (snakeDir == LEFT)     coordIncrease.y = 1;
  if (snakeDir == UP)       coordIncrease.z = 1;
  if (snakeDir == DOWN)     coordIncrease.z = -1;

  Coords newHead = {
    snake[0].x + coordIncrease.x,
    snake[0].y + coordIncrease.y,
    snake[0].z + coordIncrease.z
  };

  // Xuyên tường (wrap-around)
  if (newHead.x < 0) newHead.x = 5;
  if (newHead.x > 5) newHead.x = 0;
  if (newHead.y < 0) newHead.y = 5;
  if (newHead.y > 5) newHead.y = 0;
  if (newHead.z < 0) newHead.z = 5;
  if (newHead.z > 5) newHead.z = 0;

  bool ate =
    newHead.x == food.x &&
    newHead.y == food.y &&
    newHead.z == food.z;

  if (ate && bodySize < 216) {
    snake[bodySize] = snake[bodySize - 1];
    initFood();
  }

  for (int i = bodySize - 1; i > 0; i--) {
    if (snake[i].x == newHead.x &&
        snake[i].y == newHead.y &&
        snake[i].z == newHead.z) {
      gameOver = true;
      return;
    } else {
      snake[i] = snake[i-1];
    }
  }

  if (ate) bodySize++;
  snake[0] = newHead;
}

//-------------------------------------------------------
//                    DRAW TO CANVAS
//-------------------------------------------------------

void clearSnake(void) {
  for (int i = 0; i < bodySize; i++) {
    Coords p = snake[i];
    gameCanvas[p.z][p.y][p.x] = 0;
  }
}

// Đầu sáng nhất 100% duty rồi giảm dần xuống minDuty (30%)
void renderSnake(void) {
  float dutyDecrease =
    (spi.maxBodyDuty - spi.minDuty) / (bodySize - 1);

  for (int i = 0; i < bodySize; i++) {
    Coords p = snake[i];
    if (i == 0)
      gameCanvas[p.z][p.y][p.x] = 100;
    else
      gameCanvas[p.z][p.y][p.x] =
        (uint8_t)floor(spi.maxBodyDuty - (i - 1) * dutyDecrease);
  }
}

void renderFood(void) {
  gameCanvas[food.z][food.y][food.x] =
    (gameCanvas[food.z][food.y][food.x] > 0) ? 0 : 100;
}

void renderChar(void) {
  // pending
}

//-------------------------------------------------------
//                  HIỆU ỨNG PHÁO HOA
//-------------------------------------------------------
/* =========================
   SHARED IDLE EFFECT TIMER
   ========================= */

void checkIdleEffectFinished(uint8_t iterations)
{
    static uint8_t iterationCount = 0;

    if (iterationCount >= iterations) {
        fireworks.effectFinished = true;
        iterationCount = 0;
    } else {
        iterationCount++;
    }
}

/* =========================
   EFFECT 0: NORMAL FIREWORK
   ========================= */

void fireworks_Normal(void)
{
    clearCanvas(fireworksCanvas);

    int centerX = RNG(0, 6);
    int centerY = RNG(0, 6);
    int centerZ = RNG(0, 6);

    for (int i = 0; i < 24; i++) {
        int dx = RNG(-2, 3);
        int dy = RNG(-2, 3);
        int dz = RNG(-2, 3);

        int x = constrain(centerX + dx, 0, 5);
        int y = constrain(centerY + dy, 0, 5);
        int z = constrain(centerZ + dz, 0, 5);

        fireworksCanvas[z][y][x] = 100;
    }

    checkIdleEffectFinished(12);
}

/* =========================
   EFFECT 1: RANDOM ON/OFF
   ========================= */

void fireworks_Random(void)
{
    // memset is faster and clearer than triple loops
    memset(fireworksCanvas, 0, sizeof(fireworksCanvas));

    for (int z = 0; z < 6; z++)
        for (int y = 0; y < 6; y++)
            for (int x = 0; x < 6; x++)
                if (RNG(0, 2))
                    fireworksCanvas[z][y][x] = 100;

    checkIdleEffectFinished(12);
}

/* =========================
   EFFECT 2: RAIN
   ========================= */

void fireworks_Rain(void)
{
    for (int z = 0; z < 6; z++)
        for (int y = 0; y < 6; y++)
            for (int x = 0; x < 6; x++) {

                if (z == 0) {
                    fireworksCanvas[z][y][x] = 0;
                }
                else if (fireworksCanvas[z][y][x] > 0) {
                    fireworksCanvas[z][y][x] = 0;
                    fireworksCanvas[z - 1][y][x] = 100;
                }

                // New drops spawn only at top layer
                if (z == 5) {
                    if (RNG(0, 7) == 1)
                        fireworksCanvas[z][y][x] = 100;
                    else
                        fireworksCanvas[z][y][x] = 0;
                }
            }

    checkIdleEffectFinished(20);
}

/* =========================
   EFFECT 3: FRAME (OUTER PLANES)
   ========================= */

void fireworks_Frame(void)
{
    uint8_t (*canvas)[6][6] = fireworksCanvas;
    static uint8_t lastEffect = 255;

    // Reset state on effect change
    if (lastEffect != fireworks.effectIdx) {
        lastEffect = fireworks.effectIdx;
        clearCanvas(canvas);
    }

    bool xyCornerPlane = canvas[0][5][5];
    bool xyMidPlane    = canvas[0][2][2];
    bool yzCornerPlane = canvas[5][5][0];
    bool yzMidPlane    = canvas[2][2][0];
    bool zxCornerPlane = canvas[5][0][5];
    bool zxMidPlane    = canvas[2][0][2];

    // XY → Z
    if (xyCornerPlane && !xyMidPlane) {
        for (uint8_t z = 0; z < 6; z++) {
            if (!canvas[z][0][0]) {
                for (uint8_t i = 0; i < 6; i++) {
                    canvas[z][i][0] = 100;
                    canvas[z][i][5] = 100;
                    canvas[z][0][i] = 100;
                    canvas[z][5][i] = 100;
                }
                break;
            }
            if (z == 5) {
                clearCanvas(canvas);
                for (uint8_t i = 0; i < 6; i++) {
                    canvas[i][0][0] = 100;
                    canvas[i][5][0] = 100;
                    canvas[0][i][0] = 100;
                    canvas[5][i][0] = 100;
                }
            }
        }
    }

    // YZ → X
    else if (yzCornerPlane && !yzMidPlane) {
        for (uint8_t x = 0; x < 6; x++) {
            if (!canvas[0][0][x]) {
                for (uint8_t i = 0; i < 6; i++) {
                    canvas[i][0][x] = 100;
                    canvas[i][5][x] = 100;
                    canvas[0][i][x] = 100;
                    canvas[5][i][x] = 100;
                }
                break;
            }
            if (x == 5) {
                clearCanvas(canvas);
                for (uint8_t i = 0; i < 6; i++) {
                    canvas[0][0][i] = 100;
                    canvas[5][0][i] = 100;
                    canvas[i][0][0] = 100;
                    canvas[i][0][5] = 100;
                }
            }
        }
    }

    // ZX → Y
    else if (zxCornerPlane && !zxMidPlane) {
        for (uint8_t y = 0; y < 6; y++) {
            if (!canvas[0][y][0]) {
                for (uint8_t i = 0; i < 6; i++) {
                    canvas[i][y][0] = 100;
                    canvas[i][y][5] = 100;
                    canvas[0][y][i] = 100;
                    canvas[5][y][i] = 100;
                }
                break;
            }
            if (y == 5) {
                clearCanvas(canvas);
                for (uint8_t i = 0; i < 6; i++) {
                    canvas[0][0][i] = 100;
                    canvas[0][5][i] = 100;
                    canvas[0][i][0] = 100;
                    canvas[0][i][5] = 100;
                }
            }
        }
    }

    // Cube empty → initialize XY plane
    else {
        clearCanvas(canvas);
        for (uint8_t i = 0; i < 6; i++) {
            canvas[0][0][i] = 100;
            canvas[0][5][i] = 100;
            canvas[0][i][0] = 100;
            canvas[0][i][5] = 100;
        }
    }

    checkIdleEffectFinished(17);
}

/* =========================
   EFFECT 4: FULL ON
   ========================= */

void fireworks_Full(void)
{
    // WARNING: relies on uint8_t canvas
    memset(fireworksCanvas, 100, sizeof(fireworksCanvas));
    checkIdleEffectFinished(12);
}

/* =========================
   EFFECT 5: CORNER CUBE
   ========================= */

void fireworks_CornerCube(void)
{
    static int phase = 0;
    static uint8_t lastEffect = 255;

    if (lastEffect != fireworks.effectIdx) {
        lastEffect = fireworks.effectIdx;
        phase = 0;
    }

    clearCanvas(fireworksCanvas);

    int size;
    if (phase <= 5) {
        size = phase;
        for (int z = 0; z <= size; z++)
            for (int y = 0; y <= size; y++)
                for (int x = 0; x <= size; x++)
                    fireworksCanvas[z][y][x] = 100;
    } else {
        size = 11 - phase;
        int start = 5 - size;
        for (int z = start; z <= 5; z++)
            for (int y = start; y <= 5; y++)
                for (int x = start; x <= 5; x++)
                    fireworksCanvas[z][y][x] = 100;
    }

    phase++;
    if (phase > 11)
        phase = 0;

    checkIdleEffectFinished(23);
}

/* =========================
   EFFECT DISPATCHER
   ========================= */

void showFireworks(void)
{
    switch (fireworks.effectIdx) {
        case CLEAR:   clearCanvas(fireworksCanvas); break;
        case RANDOM:  fireworks_Random();           break;
        case RAIN:    fireworks_Rain();             break;
        case FRAME:   fireworks_Frame();            break;
        case FULL:    fireworks_Full();             break;
        case CORNER:  fireworks_CornerCube();       break;
        case NORMAL:  fireworks_Normal();           break;
        default:      break;
    }
}

//-------------------------------------------------------
//                       SPI PWM
//-------------------------------------------------------

void SPIByteMapping(const uint8_t layer[6][6], uint8_t out[6]) { // Phuoc Khang
  int bits[48];
  int idx = 0;

  // Flatten out the map into wiring order (see schematic)
  for (int y = 0; y < 6; y++)
    for (int x = 0; x < 6; x++)
      bits[idx++] = layer[y][x];

  // Adding the redundant 0s
  for (; idx < 48; idx++)
    bits[idx] = 0;

  // Because SPI sends the first byte of the array, that ends up in the last register so the order is reversed
  int reversed_bits[48];
  for (int i = 0; i < 48; i++)
    reversed_bits[i] = bits[47 - i];

  /* At this point, i realised i could've added 6 more FETs into the outputs of the MBI5026
     but i figured the wiring is gonna be problematic so i stuck with bit-banging through GPIOs only */

  uint8_t byteChunk = 0;
  int pos = 7;
  int o = 0;

  for (int b = 0; b < 48; b++) {
    byteChunk |= (reversed_bits[b] > 0) << pos;
    pos--;
    if (pos < 0) {
      out[o++] = byteChunk;
      byteChunk = 0;
      pos = 7;
    }
  }
}

void PWMCalc(uint8_t phase, uint8_t layer[6][6], uint8_t out[6][6]) {
  for (int y = 0; y < 6; y++) {
    for (int x = 0; x < 6; x++) {
      if (layer[y][x] == 0) {
        out[y][x] = 0;
      } else {
        // Map duty percentage to linearized temporal slots
        uint16_t onBits = (layer[y][x] * cycles) / 100;
        out[y][x] = ((phase * onBits) % cycles) < onBits;
      }
    }
  }
}


//void SPIOutput(uint8_t data[6], int FETidx) { // Phuoc Khang
//  // FET Active LOW
//  // OE Active LOW
//  // LATCH Active HIGH
//  // SPI MSBFIRST
//  // Remember to solder the remaining 2 unconnected inputs of SN74LS07 to ground, don't leave them floating!!!
//  // -> well, they seem to work fine without being grounded anyways - Update: 11/12/2025
//
//	/* Disable output + unlatch */
//	OE_GPIO_Port->BSRR    = ((uint32_t)OE_Pin    << 16);   // OE HIGH (disable)
////	LATCH_GPIO_Port->BSRR = ((uint32_t)LATCH_Pin << 16);   // LATCH LOW
//
//	// Okay new level of coding here: we do it C bare metal (for speed) - Update: 19/1/2026
//	// Move from HAL_GPIO_WritePin to direct register access
//	// Basically the HAL just covers up the same thing with a better name
//	// BSRR means Bit Set/Reset Register, which is uint32_t
//	// From bit 0 to 15 is "SET" and bit 16 to 31 is "RESET"
//	// Let's have an example of the GPIO_Pin_3 which is 0b0000_0000_0000_1000
//	// When not shifted by bits, setting the register at port A: GPIOA->BSRR = GPIO_Pin_3
//	// Means the register is set to this 0000_0000_0000_0000 0000_0000_0000_1000 (SET lane)
//	// The register then commands the pin to HIGH then empties out the buffer
//	// When reset, we shift it leftwards by 16 bits: GPIOA->BSRR = GPIO_Pin_3 << 16
//	// That means 0000_0000_0000_1000 0000_0000_0000_0000 (RESET lane)
//	/* Disable all FET layers (ACTIVE LOW → drive HIGH) */
//	for (int i = 0; i < 6; i++) {
//		FET[i].port->BSRR = FET[i].pin;
//	}
//
//	// Instead of vspi->beginTransaction() like C++,
//	// We first check if the SPI has done sending out data before overriding the register with new byte
//	// SR means Status Register and DR means Data Register
//	// TXE means Tx Empty (Transmit Buffer Empty)
//	// BSY means Busy (Bits are still moving on the wire)
//	// while (!(SPIx->SR & SPI_SR_TXE)); -> wait until byte is sent to send next byte
//	// while (SPIx->SR & SPI_SR_BSY); -> wait until last bit is physically done
//
//	// Ensure SPI enabled
//	SPIx->CR1 |= SPI_CR1_SPE;
//
//	// Drain previous transaction
//	while (SPIx->SR & SPI_SR_BSY);
//
//	/* Push 6 bytes through SPI */
//	for (int i = 0; i < 6; i++) {
//		while (!(SPIx->SR & SPI_SR_TXE));
//		SPIx->DR = data[i];
//	}
//	while (SPIx->SR & SPI_SR_BSY);
//
//	// Latch new data
//	LATCH_GPIO_Port->BSRR = LATCH_Pin;      // LATCH HIGH
//	LATCH_GPIO_Port->BSRR = LATCH_Pin << 16;// LATCH LOW
//
//	// NOW enable the selected layer
//	FET[FETidx].port->BSRR = FET[FETidx].pin << 16;
//
//	// Finally enable outputs
//	OE_GPIO_Port->BSRR = OE_Pin << 16; // OE LOW
//}

void SPIOutput(uint8_t data[6], int FETidx)
{
    /* 1. Disable output */
    OE_GPIO_Port->BSRR = OE_Pin;   // OE HIGH (disable)

    /* 2. Disable all FET layers (ACTIVE LOW → drive HIGH) */
    for (int i = 0; i < 6; i++) {
        FET[i].port->BSRR = FET[i].pin;   // HIGH = OFF
    }

    /* 3. Transmit 6 bytes over SPI (blocking) */
    HAL_SPI_Transmit(&hspi1, data, 6, HAL_MAX_DELAY);

    /* 4. Latch shifted data */
    LATCH_GPIO_Port->BSRR = LATCH_Pin;          // LATCH HIGH
    LATCH_GPIO_Port->BSRR = LATCH_Pin << 16;    // LATCH LOW

    /* 5. Enable selected layer */
    FET[FETidx].port->BSRR = FET[FETidx].pin << 16;  // LOW = ON

    /* 6. Enable output */
    OE_GPIO_Port->BSRR = OE_Pin << 16;   // OE LOW (enable)
}

uint8_t pwmPhase = 0;
uint8_t layerIdx = 0;
uint8_t pwmLayer[6][6];
void SPIControlHub(uint8_t canvas[6][6][6]) {
  PWMCalc(pwmPhase, canvas[layerIdx], pwmLayer);
  SPIByteMapping(pwmLayer, SPIdata);
  SPIOutput(SPIdata, layerIdx);

  pwmPhase++;
  if (pwmPhase >= cycles)
    pwmPhase = 0;

  uint32_t now = micros();
  if ((now - spiCounter.time) >= spiCounter.interval) {
    layerIdx++;
    if (layerIdx > 5)
      layerIdx = 0;
    spiCounter.time = now;
  }
}
