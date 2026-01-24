/*
  This project uses ESP32 2.0.14 Board, newer versions of ESP32 won't be compatible with DabbleESP32
  Due to changes like ledcAttachPin to ledcAttach and many other code refactors
  DabbleESP32 version 1.5.1 (newest)
*/

#include <DabbleESP32.h>
#define BLUETOOTH_NAME "ESP32_Snake"

//################ STM32 I2C COMMS ################
#include <Wire.h>
#define SDA 21
#define SCL 22
#define STM32CheckPin 4 // Input pull-down
#define I2C_STM32_ADDR 0x2A

//################ MBI5026 ################
#include <SPI.h>
#define MHz 1000000
// Active HIGHs
#define MOSI 23 
#define MISO 19 // NOT USED
#define SCK 18
#define LATCH 5 // SS/Chip Select
// Active LOW
#define OE 17

//################ P-MOSFETs ################
// Active LOW
#define FET0 13
#define FET1 27
#define FET2 26
#define FET3 25
#define FET4 33
#define FET5 32

uint8_t FET[6] = {FET0, FET1, FET2, FET3, FET4, FET5};

/*
  LƯU Ý: HỆ TRỤC TỌA ĐỘ CỦA LED CUBE ĐƯỢC ĐẶT TẠI GỐC DƯỚI PHẢI, GẦN PHÍA NGƯỜI XEM (Đặt POV ở người xem cục LED Cube)
  
  LED Cube ở đây
          X        Z
            \      |
              \    |
                \  |
          Y ______\|
              Người xem đứng ở đây (chìa tay phải ra là hiểu)

  SỬ DỤNG QUY TẮC BÀN TAY PHẢI (chỉa 3 ngón vuông góc với nhau: ngón cái, ngón trỏ, ngón giữa)
  -> NGÓN CÁI: trục Z, NGÓN TRỎ: trục X, NGÓN GIỮA: trục Y
  => Trục X là vị trí của dãy led ngang (đếm từ gần ra xa)
  => Trục Y là vị trí của dãy led dọc (đếm trừ phải sang trái)
  => Trục Z là mặt phẳng LED (đếm từ dưới lên trên)
*/

struct Counter {
  uint32_t interval; // millis/micros
  uint32_t time;
};
Counter spiCounter, dabble, game, foodCounter, fireworksCounter, STM32Counter; // Modify at setup()

//################ GAME SETTINGS ################
// ESP32 Drive/Bluetooth-only mode
bool STM32CheckFlag = false;
bool STM32CheckPinRead;

// I2C Settings
uint8_t I2CData[8];
uint8_t I2CDataIdx = 0;
// If STM32 is plugged in, 3.3V will be connected to
// this pin and signal ESP32 to switch to Bluetooth-only mode

// SPI Settings
struct SPIConfig {
  uint32_t clock;
  uint8_t bitOrder;
  uint8_t mode;
  uint8_t fps;
  uint8_t pwmRes; // 0 - 7
  uint8_t minDuty; // %
  uint8_t maxBodyDuty; // % - To make the head stand out the more gradient it has
};
SPIConfig spi = {4*MHz, MSBFIRST, SPI_MODE0, 60, 8, 5, 50};
SPIClass *vspi = NULL;
SPISettings settings(spi.clock, spi.bitOrder, spi.mode);
uint8_t SPIdata[6];

//################ DABBLE ################
struct DabbleInput_t {
  bool forward;
  bool backward;
  bool left;
  bool right;
  bool up;
  bool down;
  bool start;
  bool select;
};
DabbleInput_t dabbleInput;
bool gameDebug = false;
bool dabbleDebug = false;
bool serialDebug = true;
Counter gameDebugCounter, dabbleDebugCounter, serialDebugCounter;
bool dabbleEnable = true;

// In-game
bool gameStart = false;
bool gameOver = false;
int highscore;

// End-game
struct Fireworks {
  bool start;
  bool resetGameFlag;
  bool effectFlag;
  bool effectIdle;
  bool effectFinished;
  int effectIdx;
};
Fireworks fireworks = {false, false, false, false, false, 0};

// Idle
enum FireworksEffects {
  NORMAL,
  RANDOM,
  RAIN,
  FRAME,
  FULL,
  CORNER,
  CLEAR
};

//################ Canvas ################
uint8_t gameCanvas[6][6][6];
uint8_t labelCanvas[6][6][6];
uint8_t fireworksCanvas[6][6][6];

// Binary maps for 0-9 and A-Z based on ASCII decimal code
typedef uint8_t Glyph6[6][6];
const Glyph6 FONT[128] = {

  /* 0–47: blank */
  #define BLANK {{0,0,0,0,0,0},{0,0,0,0,0,0},{0,0,0,0,0,0},{0,0,0,0,0,0},{0,0,0,0,0,0},{0,0,0,0,0,0}}

  BLANK,BLANK,BLANK,BLANK,BLANK,BLANK,BLANK,BLANK,BLANK,BLANK,
  BLANK,BLANK,BLANK,BLANK,BLANK,BLANK,BLANK,BLANK,BLANK,BLANK,
  BLANK,BLANK,BLANK,BLANK,BLANK,BLANK,BLANK,BLANK,BLANK,BLANK,
  BLANK,BLANK,BLANK,BLANK,BLANK,BLANK,BLANK,BLANK,BLANK,BLANK,
  BLANK,BLANK,BLANK,BLANK,BLANK,BLANK,BLANK,BLANK,

  /* '0' = ASCII 48 */
  {{1,1,1,1,1,1},
  {1,0,0,0,0,1},
  {1,0,0,0,1,1},
  {1,1,0,0,0,1},
  {1,0,0,0,0,1},
  {1,1,1,1,1,1}},

  /* '1' */
  {{0,0,1,1,0,0},
  {0,1,0,1,0,0},
  {1,0,0,1,0,0},
  {0,0,0,1,0,0},
  {0,0,0,1,0,0},
  {1,1,1,1,1,1}},

  /* '2' */
  {{1,1,1,1,1,0},
  {0,0,0,0,0,1},
  {0,0,0,0,1,0},
  {0,0,0,1,0,0},
  {0,0,1,0,0,0},
  {1,1,1,1,1,1}},

  /* '3' */
  {{1,1,1,1,1,0},
  {0,0,0,0,0,1},
  {0,0,0,1,1,0},
  {0,0,0,0,0,1},
  {1,0,0,0,0,1},
  {0,1,1,1,1,0}},

  /* '4' */
  {{0,0,0,1,1,0},
  {0,0,1,0,1,0},
  {0,1,0,0,1,0},
  {1,0,0,0,1,0},
  {1,1,1,1,1,1},
  {0,0,0,0,1,0}},

  /* '5' */
  {{1,1,1,1,1,1},
  {1,0,0,0,0,0},
  {1,1,1,1,1,0},
  {0,0,0,0,0,1},
  {0,0,0,0,0,1},
  {1,1,1,1,1,0}},

  /* '6' */
  {{0,1,1,1,1,0},
  {1,0,0,0,0,0},
  {1,1,1,1,1,0},
  {1,0,0,0,0,1},
  {1,0,0,0,0,1},
  {0,1,1,1,1,0}},

  /* '7' */
  {{1,1,1,1,1,1},
  {0,0,0,0,0,1},
  {0,0,0,0,1,0},
  {0,0,0,1,0,0},
  {0,0,1,0,0,0},
  {0,0,1,0,0,0}},

  /* '8' */
  {{0,1,1,1,1,0},
  {1,0,0,0,0,1},
  {0,1,1,1,1,0},
  {1,0,0,0,0,1},
  {1,0,0,0,0,1},
  {0,1,1,1,1,0}},

  /* '9' */
  {{0,1,1,1,1,0},
  {1,0,0,0,0,1},
  {1,0,0,0,0,1},
  {0,1,1,1,1,1},
  {0,0,0,0,0,1},
  {0,1,1,1,1,0}},

  /* 58–64 blank */
  BLANK,BLANK,BLANK,BLANK,BLANK,BLANK,BLANK,

  /* 'A' = 65 */
  {{0,1,1,1,1,0},
  {1,0,0,0,0,1},
  {1,0,0,0,0,1},
  {1,1,1,1,1,1},
  {1,0,0,0,0,1},
  {1,0,0,0,0,1}},

  /* 'B' */
  {{1,1,1,1,1,0},
  {1,0,0,0,0,1},
  {1,1,1,1,1,0},
  {1,0,0,0,0,1},
  {1,0,0,0,0,1},
  {1,1,1,1,1,0}},

  /* 'C' */
  {{0,1,1,1,1,1},
  {1,0,0,0,0,0},
  {1,0,0,0,0,0},
  {1,0,0,0,0,0},
  {1,0,0,0,0,0},
  {0,1,1,1,1,1}},

  /* 'D' */
  {{1,1,1,1,0,0},
  {1,0,0,0,1,0},
  {1,0,0,0,0,1},
  {1,0,0,0,0,1},
  {1,0,0,0,1,0},
  {1,1,1,1,0,0}},

  /* 'E' */
  {{1,1,1,1,1,1},
  {1,0,0,0,0,0},
  {1,1,1,1,1,0},
  {1,0,0,0,0,0},
  {1,0,0,0,0,0},
  {1,1,1,1,1,1}},

  /* 'F' */
  {{1,1,1,1,1,1},
  {1,0,0,0,0,0},
  {1,1,1,1,1,0},
  {1,0,0,0,0,0},
  {1,0,0,0,0,0},
  {1,0,0,0,0,0}},

  /* 'G' */
  {{0,1,1,1,1,1},
  {1,0,0,0,0,0},
  {1,0,0,1,1,1},
  {1,0,0,0,0,1},
  {1,0,0,0,0,1},
  {0,1,1,1,1,1}},

  /* 'H' */
  {{1,0,0,0,0,1},
  {1,0,0,0,0,1},
  {1,1,1,1,1,1},
  {1,0,0,0,0,1},
  {1,0,0,0,0,1},
  {1,0,0,0,0,1}},

  /* 'I' */
  {{1,1,1,1,1,1},
  {0,0,1,1,0,0},
  {0,0,1,1,0,0},
  {0,0,1,1,0,0},
  {0,0,1,1,0,0},
  {1,1,1,1,1,1}},

  /* 'J' */
  {{0,0,0,0,1,1},
  {0,0,0,0,0,1},
  {0,0,0,0,0,1},
  {1,0,0,0,0,1},
  {1,0,0,0,0,1},
  {0,1,1,1,1,0}},

  /* 'K' */
  {{1,0,0,0,1,0},
  {1,0,0,1,0,0},
  {1,0,1,0,0,0},
  {1,1,0,0,0,0},
  {1,0,1,0,0,0},
  {1,0,0,1,0,0}},

  /* 'L' */
  {{1,0,0,0,0,0},
  {1,0,0,0,0,0},
  {1,0,0,0,0,0},
  {1,0,0,0,0,0},
  {1,0,0,0,0,0},
  {1,1,1,1,1,1}},

  /* 'M' */
  {{1,0,0,0,0,1},
  {1,1,0,0,1,1},
  {1,0,1,1,0,1},
  {1,0,0,0,0,1},
  {1,0,0,0,0,1},
  {1,0,0,0,0,1}},

  /* 'N' */
  {{1,0,0,0,0,1},
  {1,1,0,0,0,1},
  {1,0,1,0,0,1},
  {1,0,0,1,0,1},
  {1,0,0,0,1,1},
  {1,0,0,0,0,1}},

  /* 'O' */
  {{0,1,1,1,1,0},
  {1,0,0,0,0,1},
  {1,0,0,0,0,1},
  {1,0,0,0,0,1},
  {1,0,0,0,0,1},
  {0,1,1,1,1,0}},

  /* 'P' */
  {{1,1,1,1,1,0},
  {1,0,0,0,0,1},
  {1,1,1,1,1,0},
  {1,0,0,0,0,0},
  {1,0,0,0,0,0},
  {1,0,0,0,0,0}},

  /* 'Q' */
  {{0,1,1,1,1,0},
  {1,0,0,0,0,1},
  {1,0,0,0,0,1},
  {1,0,0,0,0,1},
  {1,0,0,1,0,1},
  {0,1,1,1,1,1}},

  /* 'R' */
  {{1,1,1,1,1,0},
  {1,0,0,0,0,1},
  {1,1,1,1,1,0},
  {1,0,1,0,0,0},
  {1,0,0,1,0,0},
  {1,0,0,0,1,0}},

  /* 'S' */
  {{0,1,1,1,1,1},
  {1,0,0,0,0,0},
  {0,1,1,1,1,0},
  {0,0,0,0,0,1},
  {1,0,0,0,0,1},
  {0,1,1,1,1,0}},

  /* 'T' */
  {{1,1,1,1,1,1},
  {0,0,1,1,0,0},
  {0,0,1,1,0,0},
  {0,0,1,1,0,0},
  {0,0,1,1,0,0},
  {0,0,1,1,0,0}},

  /* 'U' */
  {{1,0,0,0,0,1},
  {1,0,0,0,0,1},
  {1,0,0,0,0,1},
  {1,0,0,0,0,1},
  {1,0,0,0,0,1},
  {0,1,1,1,1,0}},

  /* 'V' */
  {{1,0,0,0,0,1},
  {1,0,0,0,0,1},
  {1,0,0,0,0,1},
  {0,1,0,0,1,0},
  {0,1,0,0,1,0},
  {0,0,1,1,0,0}},

  /* 'W' */
  {{1,0,0,0,0,1},
  {1,0,0,0,0,1},
  {1,0,0,1,0,1},
  {1,0,1,0,1,1},
  {1,1,0,0,1,1},
  {1,0,0,0,0,1}},

  /* 'X' */
  {{1,0,0,0,0,1},
  {0,1,0,0,1,0},
  {0,0,1,1,0,0},
  {0,0,1,1,0,0},
  {0,1,0,0,1,0},
  {1,0,0,0,0,1}},

  /* 'Y' */
  {{1,0,0,0,0,1},
  {0,1,0,0,1,0},
  {0,0,1,1,0,0},
  {0,0,1,1,0,0},
  {0,0,1,1,0,0},
  {0,0,1,1,0,0}},

  /* 'Z' */
  {{1,1,1,1,1,1},
  {0,0,0,0,1,0},
  {0,0,0,1,0,0},
  {0,0,1,0,0,0},
  {0,1,0,0,0,0},
  {1,1,1,1,1,1}},
};
const char startMsg[] = "PRESS START TO START";
const char overMsg[] = "THANKS FOR PLAYING";
// Chưa làm xong phần hiện lên led cube cái này

//################ SNAKE ################
struct Coords {
  int x;
  int y;
  int z;
};
Coords snake[216];
int bodySize; // Modify at gameSetup()

enum Directions {
  FORWARD,
  BACKWARD,
  RIGHT,
  LEFT,
  UP,
  DOWN
};
int snakeDir, bufferDir;

//################ FOOD ################
Coords food;

void setup() {
  Serial.begin(115200);
  Wire.begin(SDA, SCL);
  Dabble.begin(BLUETOOTH_NAME);
  Serial.println("Dabble ready. Waiting for connection...");
  
  // Setup for timings and snake init
  gameSetup();

  Serial.println("Checking if STM32 is connected");
  pinMode(STM32CheckPin, INPUT_PULLDOWN);
  STM32Counter.time = millis();
  while (millis() - STM32Counter.time < STM32Counter.interval) {
    STM32CheckPinRead = digitalRead(STM32CheckPin);
    if (STM32CheckPinRead) {
      Serial.println("STM32 is connected, switching to Bluetooth-only mode");
      initCounters();
      STM32CheckFlag = true;
      break;
    }
  }
  if (!STM32CheckFlag) {
    Serial.println("STM32 is not connected, switching to Drive mode");
    pinMode(LATCH, OUTPUT);
    pinMode(OE, OUTPUT);
    vspi = new SPIClass();
    vspi->begin(SCK, MISO, MOSI, LATCH);

    digitalWrite(OE, 1);
    digitalWrite(LATCH, 0);

    for (int i = 0; i<6; i++) {
      pinMode(FET[i], OUTPUT);
      digitalWrite(FET[i], 1);
    }
  }
}

//-------------------------------------------------------
//                    INITIALIZATION
//-------------------------------------------------------
void clearCanvas(uint8_t (&canvas)[6][6][6]) {
  memset(canvas, 0, sizeof(canvas));
  // for (int z = 0; z < 6; z++)
  //   for (int y = 0; y < 6; y++)
  //     for (int x = 0; x < 6; x++)
  //       canvas[z][y][x] = 0;
}

void initSnake() {
  snake[0] = {random(1,4), random(1,4), 0};
  Coords coordIncrease = {0, 0, 0}; // X, Y, Z
  if (snakeDir == FORWARD) coordIncrease.x = -1;
  if (snakeDir == BACKWARD) coordIncrease.x = 1;
  if (snakeDir == RIGHT) coordIncrease.y = 1;
  if (snakeDir == LEFT) coordIncrease.y = -1;
  if (snakeDir == UP) coordIncrease.z = -1;
  if (snakeDir == DOWN) coordIncrease.z = 1;


  for (int i = 1; i < bodySize; i++)
    snake[i] = {snake[i-1].x + i*coordIncrease.x, snake[i-1].y + i*coordIncrease.y, snake[i-1].z + i*coordIncrease.z};
}

void initFood() { // Nhat Huy
  bool check;
  do {
    food.x = random(0, 5);
    food.y = random(0, 5);
    food.z = random(0, 5);
    check = false;

    for (int i=0; i < bodySize; ++i){
      if (snake[i].x==food.x && snake[i].y==food.y && snake[i].z==food.z){
        check = true;
        break;
      }
    }
  } while (check);
}

void initCounters() {
  // Timing
  spiCounter.interval = MHz / (spi.fps * 6); // micros
  dabble.interval = 2; // millis
  game.interval = 600; // millis
  foodCounter.interval = 150; // millis
  fireworksCounter.interval = 250; // millis
  gameDebugCounter.interval = dabbleDebugCounter.interval = serialDebugCounter.interval = 3000; // millis
  STM32Counter.interval = 500; // millis

  spiCounter.time = micros();
  dabble.time = millis();
  game.time = millis();
  foodCounter.time = millis();
  fireworksCounter.time = millis();
  gameDebugCounter.time = dabbleDebugCounter.time = serialDebugCounter.time = millis();
}

void gameSetup() {
  // Setup fireworks
  clearCanvas(fireworksCanvas);
  // Setup labels
  clearCanvas(labelCanvas);
  // Setup game
  clearCanvas(gameCanvas);

  bodySize = 3;
  snakeDir = random(0, 3);
  initSnake();   
  initFood();

  initCounters();
}

//-------------------------------------------------------
//                    INPUT AND UPDATE 
//-------------------------------------------------------
void getInput() { // Phuc Khang
  Dabble.processInput();

  // Read all 10 buttons for game controls and settings
  dabbleInput.forward = GamePad.isUpPressed();
  dabbleInput.backward = GamePad.isDownPressed();
  dabbleInput.left = GamePad.isLeftPressed();
  dabbleInput.right = GamePad.isRightPressed();
  dabbleInput.up = GamePad.isTrianglePressed();
  dabbleInput.down = GamePad.isCrossPressed();
  dabbleInput.start  = GamePad.isStartPressed();
  dabbleInput.select = GamePad.isSelectPressed();

  // Send I2C to STM32 if connected
  if (STM32CheckFlag) {
    sendI2C();
    return;
  }

  // Debug
  if (!GamePad.isCirclePressed())
    gameDebugCounter.time = millis();
  if (!GamePad.isSquarePressed())
    dabbleDebugCounter.time = millis();
  if (!GamePad.isSelectPressed())
    serialDebugCounter.time = millis();

  if (millis() - gameDebugCounter.time >= gameDebugCounter.interval) {
    gameDebug = !gameDebug;
    Serial.println("Print debug toggled: " + String(gameDebug));
    gameDebugCounter.time = millis();
  }
  if (millis() - dabbleDebugCounter.time >= dabbleDebugCounter.interval) {
    dabbleDebug = !dabbleDebug;
    Serial.println("Dabble debug toggled: " + String(dabbleDebug));
    dabbleDebugCounter.time = millis();
  }
  if (millis() - serialDebugCounter.time >= serialDebugCounter.interval) {
    fireworks.effectIdle = true;
    fireworks.effectIdx = NORMAL;
    fireworks.effectFlag = true;
    gameStart = false;
    gameOver = true;
    // serialDebug = !serialDebug;
    // Serial.println("Serial debug toggled: " + String(serialDebug));
    // serialDebugCounter.time = millis();
  }

  if (dabbleDebug) {
    Serial.print("U:"); Serial.print(dabbleInput.up);
    Serial.print(" D:"); Serial.print(dabbleInput.down);
    Serial.print(" L:"); Serial.print(dabbleInput.left);
    Serial.print(" R:"); Serial.print(dabbleInput.right);
    Serial.print(" F:"); Serial.print(dabbleInput.forward);
    Serial.print(" B:"); Serial.print(dabbleInput.backward);
    Serial.print(" S:"); Serial.print(dabbleInput.start);
    Serial.print(" C:"); Serial.println(dabbleInput.select);
  }

  // Start game
  if (dabbleInput.start && !gameStart) {
    gameStart = true;
    gameOver = false;
    fireworks.effectFlag = false;
    fireworks.effectIdle = false;
    fireworks.effectIdx = NORMAL;
    game.time = millis();
  }

  // ============ Chống đi chéo =============
  // Đếm số nút được nhấn
  int pressedCount =  dabbleInput.up + dabbleInput.down + dabbleInput.left + dabbleInput.right + dabbleInput.forward + dabbleInput.backward;

  // Nếu 0 nhấn hoặc nhấn hơn 1 nút → bỏ qua, giữ nguyên hướng
  if (pressedCount != 1) {
    return;   // Giữ snakeDir như cũ
  }

  // ============ xử lý hướng rắn ============
  // Tránh quay 180 độ
  if (dabbleInput.left  && snakeDir != RIGHT)           bufferDir = LEFT;
  else if (dabbleInput.right && snakeDir != LEFT)       bufferDir = RIGHT;
  else if (dabbleInput.up    && snakeDir != DOWN)       bufferDir = UP;
  else if (dabbleInput.down  && snakeDir != UP)         bufferDir = DOWN;
  else if (dabbleInput.forward && snakeDir != BACKWARD) bufferDir = FORWARD;
  else if (dabbleInput.backward && snakeDir != FORWARD) bufferDir = BACKWARD;
}

void updateGameState() { // Nhat Huy
  // Tạo vector chuyển động và đầu mới
  Coords coordIncrease = {0, 0, 0}; // X, Y, Z
  snakeDir = bufferDir;
  if (snakeDir == FORWARD) coordIncrease.x = 1;
  if (snakeDir == BACKWARD) coordIncrease.x = -1;
  if (snakeDir == RIGHT) coordIncrease.y = -1;
  if (snakeDir == LEFT) coordIncrease.y = 1;
  if (snakeDir == UP) coordIncrease.z = 1;
  if (snakeDir == DOWN) coordIncrease.z = -1;
  Coords newHead = {snake[0].x + coordIncrease.x, snake[0].y + coordIncrease.y, snake[0].z + coordIncrease.z};

  // Xuyên tường (wrap-around)
  // Kiểm tra có đụng tường ko -> sang tường bên kia
  if (newHead.x < 0) newHead.x = 5;
  if (newHead.x > 5) newHead.x = 0;

  if (newHead.y < 0) newHead.y = 5;
  if (newHead.y > 5) newHead.y = 0;

  if (newHead.z < 0) newHead.z = 5;
  if (newHead.z > 5) newHead.z = 0;

  // Ăn táo
  bool ate = (newHead.x == food.x && newHead.y == food.y && newHead.z == food.z);
  if (ate && bodySize < 216) {
    snake[bodySize] = snake[bodySize - 1]; // Tạo cục mới là vị trí của cục cuối
    initFood(); // spawn táo mới (không trùng rắn)
  }
  // Đẩy cơ thể lên phía trước
  for (int i = bodySize - 1; i > 0; i--) {
    //  Chạm cơ thể → đứng yên
    if (snake[i].x == newHead.x && snake[i].y == newHead.y && snake[i].z == newHead.z) {
      gameOver = true;
      return;
    }
    else snake[i] = snake[i-1]; // Cho rắn duy chuyển phía trước, đẩy đuôi lên đốt tiếp theo
  }
  // Update bodySize sau để tránh cục mới tạo bị đẩy lên
  if (ate) bodySize++;
  // Tạo đầu mới
  snake[0] = newHead;
}

//-------------------------------------------------------
//                       I2C STM32
//-------------------------------------------------------
void sendI2C() {
  I2CData[I2CDataIdx++] = dabbleInput.forward;
  I2CData[I2CDataIdx++] = dabbleInput.backward;
  I2CData[I2CDataIdx++] = dabbleInput.left;
  I2CData[I2CDataIdx++] = dabbleInput.right;
  I2CData[I2CDataIdx++] = dabbleInput.up;
  I2CData[I2CDataIdx++] = dabbleInput.down;
  I2CData[I2CDataIdx++] = dabbleInput.start;
  I2CData[I2CDataIdx++] = dabbleInput.select;

  // Serial.printf("Sending to address %x %d bytes: ", I2C_STM32_ADDR, I2CDataIdx);
  // for (int i = 0; i<I2CDataIdx; i++)
  //   Serial.printf("[%d, %d]", i, I2CData[i]);
  // Serial.println();

  Wire.beginTransmission(I2C_STM32_ADDR);
  Wire.write(I2CData, I2CDataIdx);
  Wire.endTransmission();
  I2CDataIdx = 0;
}

//-------------------------------------------------------
//                    DRAW TO CANVAS
//-------------------------------------------------------
void clearSnake() {
  for (int i = 0; i < bodySize; i++) {    
    Coords p = snake[i];
    gameCanvas[p.z][p.y][p.x] = 0;
  }
}

// Đầu sáng nhất 100% duty rồi giảm dần xuống minDuty (30%)
void renderSnake() { // Nhat Huy
  float dutyDecrease = (spi.maxBodyDuty - spi.minDuty)/(bodySize-1);
  for (int i = 0; i < bodySize; i++) {    
    Coords p = snake[i];
    if (i == 0) gameCanvas[p.z][p.y][p.x] = 100;
    else gameCanvas[p.z][p.y][p.x] = (uint8_t) floor(spi.maxBodyDuty - (i-1)*dutyDecrease);
  }
}

void renderFood() { // Nhat Huy
  // Làm cho táo nhấp nháy (Nếu chưa sáng thì sáng, nếu sáng thì tắt)
  gameCanvas[food.z][food.y][food.x] = (gameCanvas[food.z][food.y][food.x] > 0) ? 0 : 100;
}

void renderChar() {
  // pending
}

//-------------------------------------------------------
//                  HIỆU ỨNG PHÁO HOA
//-------------------------------------------------------
// DEMO GIỐNG THEGIOIDIDONG
// Shared idle-effect timer
void checkIdleEffectFinished(uint8_t iterations) {
  static uint8_t iterationCount = 0;

  if (iterationCount >= iterations) {
    fireworks.effectFinished = true;
    iterationCount = 0;
  }
  else
    iterationCount++;
}

// Hiệu ứng 0: Pháo hoa bình thường
void fireworks_Normal() {
  clearCanvas(fireworksCanvas);
  int centerX = random(0,6);
  int centerY = random(0,6);
  int centerZ = random(0,6);
  for (int i = 0; i < 24; i++) {
    int dx = random(-2,3);
    int dy = random(-2,3);
    int dz = random(-2,3);
    int x = constrain(centerX + dx, 0, 6);
    int y = constrain(centerY + dy, 0, 6);
    int z = constrain(centerZ + dz, 0, 6);
    fireworksCanvas[z][y][x] = 100;
  }
  checkIdleEffectFinished(12);
}

// Hiệu ứng 1: Bặt tắt ngẫu nhiên
void fireworks_Random() {
  clearCanvas(fireworksCanvas);
  for (int z = 0; z < 6; z++) {
    for (int y = 0; y < 6; y++) {
      for (int x = 0; x < 6; x++) {
        // Tắt hoặc Bật ngẫu nhiên
        int chance = random(0, 2);
        if (chance)
          fireworksCanvas[z][y][x] = 100; 
      }
    }
  }
  checkIdleEffectFinished(12);
}

// Hiệu ứng 2: Mô phỏng mưa
void fireworks_Rain() { // Y như mưa
  for (int z = 0; z < 6; z++) {
    for (int y = 0; y < 6; y++) {
      for (int x = 0; x < 6; x++) {
        if (z == 0)
          fireworksCanvas[z][y][x] = 0;
        else if (fireworksCanvas[z][y][x] > 0) {
          fireworksCanvas[z][y][x] = 0;
          fireworksCanvas[z-1][y][x] = 100;
        }

        if (z == 5) {
          int rand = random(0, 7);
          if (rand == 1)
            fireworksCanvas[z][y][x] = 100;
          else
            fireworksCanvas[z][y][x] = 0;
        }
      }
    }
  }
  checkIdleEffectFinished(20);
}

// Hiệu ứng 3: Bật tất cả các LED ở 4 mặt bên ngoài 
void fireworks_Frame() {
  uint8_t (&canvas)[6][6][6] = fireworksCanvas;
  static uint8_t lastEffect = 255;

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
  // If cube is empty, initiate xyPlane
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

// Hiệu ứng 4: Mở full LED
void fireworks_Full() {
  for (int z = 0; z < 6; z++)
    for (int y = 0; y < 6; y++)
      for (int x = 0; x < 6; x++)
        fireworksCanvas[z][y][x] = 100; 
  checkIdleEffectFinished(12);
}

// Hiệu ứng 5: Cube từ (0, 0, 0) lớn dần lên rồi nhỏ lại tại (5, 5, 5)
void fireworks_CornerCube() {
  static int phase = 0;
  static uint8_t lastEffect = 255;

  if (lastEffect != fireworks.effectIdx) {
    lastEffect = fireworks.effectIdx;
    phase = 0;
  }

  clearCanvas(fireworksCanvas);
  int size;

  // Grow 0 → 5, then shrink 5 → 0
  if (phase <= 5) {
    size = phase;
    // Growing cube anchored at (0,0,0)
    for (int z = 0; z <= size; z++)
      for (int y = 0; y <= size; y++)
        for (int x = 0; x <= size; x++)
          fireworksCanvas[z][y][x] = 100;
  } else {
    size = 11 - phase;
    // Shrinking cube anchored at (5,5,5)
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

void showFireworks() { // Gia Huy
  switch (fireworks.effectIdx) {
    case CLEAR:
      clearCanvas(fireworksCanvas);
      break;
    case RANDOM:
      fireworks_Random(); // Random On/Off
      break;
    case RAIN:
      fireworks_Rain(); // Rain
      break;
    case FRAME:
      fireworks_Frame(); // Frame
      break;
    case FULL:
      fireworks_Full(); // Full On
      break;
    case CORNER:
      fireworks_CornerCube(); // Corner Cube
      break;
    case NORMAL: // Pháo hoa ngẫu nhiên
      fireworks_Normal();
  }
}

//-------------------------------------------------------
//                       SPI PWM 
//-------------------------------------------------------
void SPIByteMapping(const uint8_t layer[6][6], uint8_t (&out)[6]) { // Phuoc Khang
  int bits[48];
  int idx = 0;

  // Flatten out the map into wiring order (see schematic)
  for (int y = 0; y < 6; y++)
    for (int x = 0; x < 6; x++)
      bits[idx++] = layer[y][x];
  // Adding the redundant 0s
  for (;idx<48;idx++)
    bits[idx] = 0;
  // Because SPI sends the first byte of the array, that ends up in the last register so the order is reversed
  int reversed_bits[48]; 
  for (int idx = 0; idx < 48; idx++)
    reversed_bits[idx] = bits[48-idx-1];
  /* At this point, i realised i could've added 6 more FETs into the outputs of the MBI5026
  but i figured the wiring is gonna be problematic so i stuck with bit-banging through GPIOs only */

  uint8_t byteChunk = 0;
  int pos = 7;
  int i = 0;
  for (int b = 0; b < 48; b++) {
    byteChunk |= (reversed_bits[b] > 0) << pos;
    // Serial.print(String(reversed_bits[b]));
    pos--;
    if (pos < 0) {
      // Serial.print('\t');
      // Serial.print(String(byteChunk) + '\t');
      out[i++] = byteChunk;
      byteChunk = 0;
      pos = 7;
    }
  }
  // Serial.println();
}

void PWMCalc(uint8_t phase, uint8_t (&layer)[6][6], uint8_t (&out)[6][6]) {
  static const uint16_t cycles = 1 << spi.pwmRes;

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

void SPIOutput(uint8_t data[6], int FETidx) { // Phuoc Khang
  // FET Active LOW
  // OE Active LOW
  // LATCH Active HIGH
  // SPI MSBFIRST
  // Remember to solder the remaining 2 unconnected inputs of SN74LS07 to ground, don't leave them floating!!!
  // -> well, they seem to work fine without being grounded anyways - Update: 11/12/2025
  for (int i = 0; i < 6; i++)
    digitalWrite(FET[i], 1);
  digitalWrite(OE, 1);
  digitalWrite(LATCH, 0);
  vspi->beginTransaction(settings);
  vspi->transfer(data, 6);
  digitalWrite(FET[FETidx], 0);
  digitalWrite(LATCH, 1);
  vspi->endTransaction();
  digitalWrite(OE, 0);
  digitalWrite(LATCH, 0);
}

void SPIControlHub(uint8_t (&canvas)[6][6][6]) {
  static uint8_t pwmPhase = 0;
  static uint8_t layerIdx = 0;
  static const uint16_t cycles = 1 << spi.pwmRes;
  uint8_t pwmLayer[6][6];

  PWMCalc(pwmPhase, canvas[layerIdx], pwmLayer);
  SPIByteMapping(pwmLayer, SPIdata);
  SPIOutput(SPIdata, layerIdx);

  pwmPhase++;
  if (pwmPhase >= cycles)
    pwmPhase = 0;

  if (micros() - spiCounter.time >= spiCounter.interval) {
    layerIdx++;
    if (layerIdx > 5)
      layerIdx = 0;
    spiCounter.time = micros();
  }
}

//-------------------------------------------------------
//                      DEBUGGING
//-------------------------------------------------------
void printSnake() {
  Serial.print("Snake: ");
  for (int i = 0; i < bodySize; i++) {
    Serial.print(String(i) + " (" + String(snake[i].x) + "," + String(snake[i].y) + "," + String(snake[i].z) + ")" + '\t');
  }
  Serial.println();
}

void printCanvas(uint8_t (&canvas)[6][6][6]) {
  // Funny loops, i know, it hurts my brain too but it worked so dont touch - PKhang
  for (int x = 5; x >= 0; x--) {
    for (int z = 0; z < 6; z++) {
      for (int y = 5; y >= 0; y--) {
        Serial.print(canvas[z][y][x]);
        Serial.print(" ");
      }
      Serial.print('\t');
    }
    Serial.print('\n');
  }
}

void debugPrint() {
  for(int i = 0; i < 20; i++) Serial.println();
  Serial.println("Food coords: (" + String(food.x) + "," + String(food.y) + "," + String(food.z) + ")");
  Serial.println("SnakeDir: " + String(snakeDir));
  printSnake();
  printCanvas(gameCanvas);
}

String readWord() {
  char c;
  String word = "";
  while (true) {
    if (Serial.available() > 0) {
      c = Serial.read();
      if (c == ' ' || c == '\n')
        break;
      word += c;
    }
    else
      return "";
  }
  // if (Arm.manualLog) Serial.print(word + "\n");
  return word;
}

void serialCommand() { // Phuoc Khang
  if (Serial.available() > 0) {
    bool valid = true;
    String cmdSelect = readWord(); // "fet", "mbi", "dabble", "config", "debug", "game", "effect", "restart", {"w", "a", "s", "d", "q", "e"}
    String cmdMode;
    String cmdState;
    int stateVal;
    // Special command
    if (cmdSelect == "restart") {
      // Flush serial buffer 
      while (Serial.available() > 0) 
        char temp = Serial.read();
      ESP.restart();
    }
    else if (cmdSelect == "")
      return;
    else if (STM32CheckFlag && (cmdSelect == "fet" || cmdSelect == "mbi" || cmdSelect == "config")) {
      Serial.println("STM32 is plugged in, do not fuck with this hardware command: " + cmdSelect);
      // Flush serial buffer
      while (Serial.available() > 0) 
        char temp = Serial.read();
      return;
    }
    
    // if move, skip mode and state
    if (cmdSelect != "move" && cmdSelect != "m") {
      cmdMode = readWord();
      /* {"0".."5"} - for fet
        {"data", "clock", "latch", "enable"} - for mbi
        {"0", "1"} - for dabble
        {"spi", "gameInterval", "dabbleInterval", "foodInterval"} - for config
        {"game", "dabble", "serial"} - for debug
        {"start", "over"} - for game
        {"normal", "random", "rain", "frame", "full", "corner", "spiral", "zigzag"} - for effect
      */
      // if fet, check cmdMode for numbers
      if ((cmdSelect == "fet" || cmdSelect == "dabble") && (cmdMode.length() != 1 || !isdigit(cmdMode[0])))
        valid = false;

      // if not effect & dabble, check cmdState for numbers
      if (cmdSelect != "effect" && cmdSelect != "dabble") {
        cmdState = readWord();   // "1" or "0" for spi
        // if "config spi", validate cmdState must be exactly one char and isdigit
        if ((cmdSelect == "fet" || cmdSelect == "mbi" || cmdMode == "spi") && (cmdState.length() != 1 || !isdigit(cmdState[0]))) {
          valid = false;
        }
        stateVal = cmdState.toInt();
      }
    }
    
    // Process command
    if (valid && cmdSelect == "fet") {
      int index = cmdMode.toInt();
      if (index >= 0 && index < 6) {
        digitalWrite(FET[index], stateVal);
        Serial.println("FET pin " + cmdMode + " set to " + cmdState);
      }
      else
        valid = false;
    }
    else if (valid && cmdSelect == "mbi") {
      if (cmdMode == "data") {
        digitalWrite(MOSI, stateVal);
        Serial.println("Data pin " + String(MOSI) + " set to " + cmdState);
      }
      else if (cmdMode == "clock") {
        digitalWrite(SCK, stateVal);
        Serial.println("Clock pin " + String(SCK) + " set to " + cmdState);
      }
      else if (cmdMode == "latch") {
        digitalWrite(LATCH, stateVal);
        Serial.println("Latch pin " + String(LATCH) + " set to " + cmdState);
      }
      else if (cmdMode == "enable") {
        digitalWrite(OE, stateVal);
        Serial.println("Enable pin " + String(OE) + " set to " + cmdState);
      }
      else
        valid = false;
    }
    else if (valid && cmdSelect == "dabble") {
      dabbleEnable = bool(cmdMode.toInt());
      Serial.println("Dabble enable set to: " + String(dabbleEnable));
    }
    else if (valid && cmdSelect == "config") {
      if (cmdMode == "spi") {
        if (cmdState == "1")
          vspi->begin(SCK, MISO, MOSI, LATCH);
        else if (cmdState == "0") {
          vspi->end();
          pinMode(MOSI, OUTPUT);
          pinMode(SCK, OUTPUT);
          pinMode(LATCH, OUTPUT);
        }
        Serial.println("SPI config set to " + cmdState);
      }
      else if (cmdMode.equalsIgnoreCase("gameInterval")) {
        game.interval = cmdState.toInt();
        Serial.println("Game interval set to " + cmdState);
      }
      else if (cmdMode.equalsIgnoreCase("dabbleInterval")) {
        dabble.interval = cmdState.toInt();
        Serial.println("Dabble interval set to " + cmdState);
      }
      else if (cmdMode.equalsIgnoreCase("foodInterval")) {
        foodCounter.interval = cmdState.toInt();
        Serial.println("Food interval set to " + cmdState);
      }
      else if (cmdMode.equalsIgnoreCase("fireworksInterval")) {
        fireworksCounter.interval = cmdState.toInt();
        Serial.println("Food interval set to " + cmdState);
      }
      else
        valid = false;
    }
    else if (valid && cmdSelect == "debug") {
      if (cmdMode == "game") {
        gameDebug = cmdState.toInt();
        Serial.println("gameDebug set to " + cmdState);
      }
      else if (cmdMode == "dabble") {
        dabbleDebug = cmdState.toInt();
        Serial.println("dabbleDebug set to " + cmdState);
      }
      else if (cmdMode == "serial") {
        serialDebug = cmdState.toInt();
        Serial.println("serialDebug set to " + cmdState);
      }
      else
        valid = false;
    }
    else if (valid && cmdSelect == "game") {
      if (cmdMode == "start") {
        gameStart = cmdState.toInt();
        if (gameStart) {
          fireworks.effectFlag = false;
          fireworks.effectIdle = false;
          fireworks.effectIdx = NORMAL;
          game.time = millis();
        }
        Serial.println("gameStart set to " + cmdState);
      }
      else if (cmdMode == "over") {
        gameOver = cmdState.toInt();
        Serial.println("gameOver set to " + cmdState);
      }
      else
        valid = false;
    }
    else if (valid && cmdSelect == "effect") {
      fireworks.effectIdle = false;
      fireworks.effectIdx = -1; // Temp value to compare
      if (cmdMode == "normal")
        fireworks.effectIdx = NORMAL;
      else if (cmdMode == "random")
        fireworks.effectIdx = RANDOM;
      else if (cmdMode == "rain")
        fireworks.effectIdx = RAIN;
      else if (cmdMode == "frame")
        fireworks.effectIdx = FRAME;
      else if (cmdMode == "full")
        fireworks.effectIdx = FULL;
      else if (cmdMode == "corner")
        fireworks.effectIdx = CORNER;
      else if (cmdMode == "idle") {
        fireworks.effectIdle = true;
        fireworks.effectIdx = NORMAL;
      }
      else if (cmdMode == "0")
        fireworks.effectIdx = CLEAR;
      else
        valid = false;

      if (fireworks.effectIdx == -1)
        fireworks.effectIdx = NORMAL;
      else {
        fireworks.effectFlag = true;
        gameStart = false;
        gameOver = true;
        Serial.println("Effects Mode: " + cmdMode);
      }
    }
    else if (valid) {
      char dir = cmdSelect[0];
      bool forward = 0, left = 0, backward = 0, right = 0, down = 0, up = 0;
      switch(dir) {
        case 'w':
          dabbleInput.forward = true;
          break;
        case 'a':
          dabbleInput.left = true;
          break;
        case 's':
          dabbleInput.backward = true;
          break;
        case 'd':
          dabbleInput.right = true;
          break;
        case 'q':
          dabbleInput.down = true;
          break;
        case 'e':
          dabbleInput.up = true;
          break;
        default:
          valid = false;
      }
      Serial.print("Move command received: " + String(dir) + '\t');
      Serial.println("W: " + String(dabbleInput.forward) + " A: " + String(dabbleInput.left) + " S: " + String(dabbleInput.backward) + " D: " + String(dabbleInput.right) + " Q: " + String(dabbleInput.down) + " E: " + String(dabbleInput.up));
      
      if (dabbleInput.left          && snakeDir != RIGHT)    bufferDir = LEFT;
      else if (dabbleInput.right    && snakeDir != LEFT)     bufferDir = RIGHT;
      else if (dabbleInput.up       && snakeDir != DOWN)     bufferDir = UP;
      else if (dabbleInput.down     && snakeDir != UP)       bufferDir = DOWN;
      else if (dabbleInput.forward  && snakeDir != BACKWARD) bufferDir = FORWARD;
      else if (dabbleInput.backward && snakeDir != FORWARD)  bufferDir = BACKWARD;
    }

    if (!valid) {
      Serial.println("Command not found: " + cmdSelect + " " + cmdMode + " " + cmdState);
      // Flush serial buffer
      while (Serial.available() > 0) 
        char temp = Serial.read();
    }
  }
}

//-------------------------------------------------------
//                      MAIN LOOP
//-------------------------------------------------------
void loop() {
  if (millis() - dabble.time >= dabble.interval) {
    if(dabbleEnable) getInput();
    if (STM32CheckFlag) sendI2C();
    STM32CheckPinRead = digitalRead(STM32CheckPin);
    dabble.time = millis();
  }
  
  if (serialDebug) serialCommand();

  if (!STM32CheckFlag) {
    // Hot swap STM32
    if (STM32CheckPinRead) {
        Serial.println("STM32 is connected while in Drive mode. Forcing emergency hard restart.");
        ESP.restart();
    }

    // Idle mode
    if (fireworks.effectFlag) {
      SPIControlHub(fireworksCanvas);

      if (fireworks.effectIdx == CLEAR)
        fireworks.effectFlag = false;

      if (millis() - fireworksCounter.time >= fireworksCounter.interval) {
        showFireworks();
        fireworksCounter.time = millis();
      }

      if (fireworks.effectFinished) {
        if (gameDebug)
          Serial.println("Finished Effect: " + String(fireworks.effectIdx));

        fireworks.effectFinished = false;

        if (fireworks.effectIdle) {
          fireworks.effectIdx++;

          if (fireworks.effectIdx >= CLEAR) {
            fireworks.effectIdx = NORMAL;
          }
        }
      }
    }

    // Pre-game
    if (!gameStart && !gameOver) {
      if (gameDebug) Serial.println("Game hasn't started yet, press 'Start' to start =)).");
    }

    // In-game
    else if (gameStart && !gameOver) {
      SPIControlHub(gameCanvas);

      // Cứ mỗi chu kì Interval thì nó sẽ update game
      if (millis() - game.time >= game.interval) {
        clearSnake();
        updateGameState();
        renderSnake();
        // Serial debug
        if (gameDebug) debugPrint();
        game.time = millis(); // Đặt lại thời gian hiện tại
      }

      // Render food separately to make it flicker
      if (millis() - foodCounter.time >= foodCounter.interval) {
        renderFood();
        foodCounter.time = millis();
      }
    } 

    // Game Over and Fireworks
    else if (gameStart && gameOver && !fireworks.resetGameFlag) {
      // Khi rắn đủ dài thì hiển thị pháo hoa
      if (bodySize >= 6 && !fireworks.start) {
        fireworks.effectIdx = 0;
        fireworks.start = true;
      }
      
      if (fireworks.start) {
        SPIControlHub(fireworksCanvas);
        if (millis() - fireworksCounter.time >= fireworksCounter.interval) {
          showFireworks();
          fireworksCounter.time = millis();
        }
        
        if (fireworks.effectFinished) {
          if (gameDebug) Serial.println("Game Over");
          fireworks.start = false;
          fireworks.effectFinished = false;
          fireworks.resetGameFlag = true;
          gameStart = false;
        }
      }
      else {
        if (gameDebug) Serial.println("Game Over");
        fireworks.effectFinished = false;
        fireworks.resetGameFlag = true;
        gameStart = false;
      }
    }

    // Restart game *Once*
    else if (gameOver && fireworks.resetGameFlag) {
      if (!fireworks.effectFlag)
        SPIControlHub(gameCanvas);

      if (gameStart) {
        if (gameDebug) Serial.println("Game Restarted");
        gameSetup();
        fireworks.resetGameFlag = false;
        gameOver = false;
      }
    }
  }
  else {
      // Hot swap STM32
      if (!STM32CheckPinRead) {
          Serial.println("STM32 is disconnected while in Bluetooth-only mode. Forcing emergency hard restart.");
          ESP.restart();
      }
  }
}