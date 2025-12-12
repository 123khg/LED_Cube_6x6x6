/*
  This project uses ESP32 2.0.14 Board, newer versions of ESP32 won't be compatible with DabbleESP32
  Due to changes like ledcAttachPin to ledcAttach and many other code refactors
  DabbleESP32 version 1.5.1 (newest)
*/

#include <DabbleESP32.h>
#define BLUETOOTH_NAME "ESP32_Snake"

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
  double interval;
// millis/micros
  unsigned long time;
  int effectIdx; // biến để quản lý chỉ số hiệu ứng
};
Counter spiCounter, dabble, game, foodCounter, fireworks;
// Modify at gameSetup()

//################ GAME SETTINGS ################
// SPI Settings
struct SPIConfig {
  int clock;
  int bitOrder;
  int mode;
  int fps;
  int res;
  int duty[5];
  int layerIdx;
  int propagationDelay;
};
SPIConfig spi = {4*MHz, MSBFIRST, SPI_MODE0, 60, 4, {0, 25, 50, 75, 100}, 0};
SPIClass *vspi = NULL;
SPISettings settings(spi.clock, spi.bitOrder, spi.mode);
uint8_t SPIdata[6];
double layerTime; // ms
// In-game
bool gameStart = false;
bool gameOver = false;
int highscore;
// End-game
bool fireworksStart = false;
int fireworksCount = 70; // 7 hiệu ứng * 10 lần mỗi hiệu ứng
int fireworksIdx = 0;

//################ DABBLE ################
int debugDebounce = 1000;
bool gameDebug = false;
int gameDebugFlag = 0;
bool dabbleDebug = false;
int dabbleDebugFlag = 0;
bool serialDebug = true;
int serialDebugFlag = 0;

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
char startMsg[] = "PRESS START TO START";
char overMsg[] = "THANKS FOR PLAYING";
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
  Dabble.begin(BLUETOOTH_NAME);
  Serial.println("Dabble ready. Waiting for connection...");

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

  gameSetup();
}

//-------------------------------------------------------
//                    INITIALIZATION
//-------------------------------------------------------
void clearCanvas(uint8_t (&canvas)[6][6][6]) {
  for (int z = 0; z < 6; z++)
    for (int y = 0; y < 6; y++)
      for (int x = 0; x < 6; x++)
        canvas[z][y][x] = 0;
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

  // Initialize Counters
  spiCounter = {0, 0};
  dabble = {3, 0};
  game = {600, 0};
  foodCounter = {150, 0};
  fireworks = {250, 0, 0}; // thêm 0 cho effectIdx
// Calculate SPI and PWM
  layerTime = (double) MHz / (spi.fps * 6.0); // micros
  spiCounter.interval = layerTime;
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

//-------------------------------------------------------
//                    INPUT AND UPDATE 
//-------------------------------------------------------
void getInput() { // Phuc Khang
  Dabble.processInput();

  // Read all 10 buttons for game controls and settings
  bool forward = GamePad.isUpPressed();
  bool backward = GamePad.isDownPressed();
  bool left = GamePad.isLeftPressed();
  bool right = GamePad.isRightPressed();
  bool up = GamePad.isTrianglePressed();
  bool down = GamePad.isCrossPressed();
  bool start  = GamePad.isStartPressed();
  bool select = GamePad.isSelectPressed();

  // Debug
  if (GamePad.isCirclePressed()) gameDebugFlag += 1;
  else gameDebugFlag = 0;
  if (GamePad.isSquarePressed()) dabbleDebugFlag += 1;
  else dabbleDebugFlag = 0;
  if (GamePad.isSelectPressed()) serialDebugFlag += 1;
  else serialDebugFlag = 0;

  if (gameDebugFlag >= debugDebounce) {
    gameDebug = !gameDebug;
    Serial.println("Print debug toggled: " + String(gameDebug));
    gameDebugFlag = 0;
  }
  if (dabbleDebugFlag >= debugDebounce) {
    dabbleDebug = !dabbleDebug;
    Serial.println("Dabble debug toggled: " + String(dabbleDebug));
    dabbleDebugFlag = 0;
  }
  if (serialDebugFlag >= debugDebounce) {
    serialDebug = !serialDebug;
    Serial.println("Serial debug toggled: " + String(serialDebug));
    serialDebugFlag = 0;
  }

  if (dabbleDebug) {
    Serial.print("U:"); Serial.print(up);
    Serial.print(" D:"); Serial.print(down);
    Serial.print(" L:"); Serial.print(left);
    Serial.print(" R:"); Serial.print(right);
    Serial.print(" F:"); Serial.print(forward);
    Serial.print(" B:"); Serial.print(backward);
    Serial.print(" S:"); Serial.print(start);
    Serial.print(" C:"); Serial.println(select);
  }

  // Start game
  if (start && !gameStart) {
    gameStart = true;
    game.time = millis();
  }

  // ============ Chống đi chéo =============
  // Đếm số nút được nhấn
  int pressedCount =  up + down + left + right + forward + backward;

  // Nếu 0 nhấn hoặc nhấn hơn 1 nút → bỏ qua, giữ nguyên hướng
  if (pressedCount != 1) {
    return;   // Giữ snakeDir như cũ
  }

  // ============ xử lý hướng rắn ============
  // Tránh quay 180 độ
  if (left  && snakeDir != RIGHT)           bufferDir = LEFT;
  else if (right && snakeDir != LEFT)       bufferDir = RIGHT;
  else if (up    && snakeDir != DOWN)       bufferDir = UP;
  else if (down  && snakeDir != UP)         bufferDir = DOWN;
  else if (forward && snakeDir != BACKWARD) bufferDir = FORWARD;
  else if (backward && snakeDir != FORWARD) bufferDir = BACKWARD;
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
//                    DRAW TO CANVAS
//-------------------------------------------------------
void clearSnake() {
  for (int i = 0; i < bodySize; i++) {    
    Coords p = snake[i];
    gameCanvas[p.z][p.y][p.x] = 0;
  }
}

//mức sáng: 0 = tắt; 1 = thân sáng vừa; 2 = đầu và táo sáng nhất
void renderSnake() { // Nhat Huy
  // Thân rắn = 1, Đầu rắn = 2
  for (int i = 0; i < bodySize; i++) {    
    Coords p = snake[i];
    if (i == 0) gameCanvas[p.z][p.y][p.x] = 2;
    else gameCanvas[p.z][p.y][p.x] = 1;
  }
}

void renderFood() { // Nhat Huy
  // Táo = 2
  // Làm cho táo nhấp nháy (Nếu chưa sáng thì sáng, nếu sáng thì tắt)
  gameCanvas[food.z][food.y][food.x] = (gameCanvas[food.z][food.y][food.x] > 0) ? 0 : 2;
}

void renderChar() {
  // pending
}

//-------------------------------------------------------
//                  HIỆU ỨNG PHÁO HOA
//-------------------------------------------------------

// Hiệu ứng 1: Bặt tắt ngẫu nhiên
void fireworks_Random() {
  clearCanvas(fireworksCanvas);
  for (int z = 0; z < 6; z++)
    for (int y = 0; y < 6; y++)
      for (int x = 0; x < 6; x++)
        // Tắt hoặc Bật ngẫu nhiên 
        fireworksCanvas[z][y][x] = random(0, 2); 
}

// Hiệu ứng 2: Mô phỏng mưa
void fireworks_Rain() {
  clearCanvas(fireworksCanvas);
  // Dùng `fireworksIdx` để mô phỏng giọt mưa rơi
  int layer = fireworks.fireworksIdx % 6; // Lớp hiện tại (0-5)
  int y_pos = random(0, 6);
  int x_pos = random(0, 6);
  fireworksCanvas[5 - layer][y_pos][x_pos] = 2; // Giọt mưa xuất hiện từ trên xuống
}

// Hiệu ứng 3: Bặc tất cả các LED ở 4 mặt bên ngoài
void fireworks_Frame6x6() {
  clearCanvas(fireworksCanvas);
  for (int z = 0; z < 6; z++) {
    for (int i = 0; i < 6; i++) {
      fireworksCanvas[z][0][i] = 2;
      fireworksCanvas[z][5][i] = 2;
      fireworksCanvas[z][i][0] = 2;
      fireworksCanvas[z][i][5] = 2;
    }
  }
}

// Hiệu ứng 4: Mở full LED
void fireworks_Full() {
  for (int z = 0; z < 6; z++)
    for (int y = 0; y < 6; y++)
      for (int x = 0; x < 6; x++)
        fireworksCanvas[z][y][x] = 2; 
}

// Hiệu ứng 5: tại khối 2x2x2 ở giữa nở ra mọi hướng rồi thu lại 2x2x2
void fireworks_CenterCube() {
  clearCanvas(fireworksCanvas);
  int f_rel = fireworksIdx % 10; 
  int I; 
  if (f_rel <= 4) {
    I = f_rel; 
  } else {
    I = 9 - f_rel; 
  }
  int R = I / 2; 
  int min_index = 2 - R;
  int max_index = 3 + R;
  for (int z = min_index; z <= max_index; z++) {
    for (int y = min_index; y <= max_index; y++) {
      for (int x = min_index; x <= max_index; x++) {
        fireworksCanvas[z][y][x] = 2; 
      }
    }
  }
}

// Hiệu ứng 6: Quét toàn bộ LED bằng 1 LED
void fireworks_Spiral() {
  clearCanvas(fireworksCanvas);
  int totalLeds = 216;
  int currentLed = fireworks.fireworksIdx % totalLeds;
  // Logic đơn giản: bật một LED duy nhất theo chỉ số tuần tự
  int z = currentLed / 36;
  int y = (currentLed % 36) / 6;
  int x = (currentLed % 36) % 6;
  
  fireworksCanvas[z][y][x] = 2;
}
void showFireworks() { // Gia Huy
  switch (fireworks.effectIdx) {
    case 1:
      fireworks_Random(); // Random On/Off
      break;
    case 2:
      fireworks_Rain(); // Rain
      break;
    case 3:
      fireworks_Frame6x6(); // Frame
      break;
    case 4:
      fireworks_Full(); // Full On
      break;
    case 5:
      fireworks_CenterCube(); // Center Cube
      break;
    case 6:
      fireworks_Spiral(); // Spiral (đơn giản)
      break;
    case 0:
    default: // Pháo hoa ngẫu nhiên
      clearCanvas(fireworksCanvas);
      int centerX = random(0,6);
      int centerY = random(0,6);
      int centerZ = random(0,5);
      for (int i = 0; i < 24; i++) {
        int dx = random(-2,3);
        int dy = random(-2,3);
        int dz = random(-2,3);
        int x = constrain(centerX + dx, 0, 5);
        int y = constrain(centerY + dy, 0, 5);
        int z = constrain(centerZ + dz, 0, 5);
        fireworksCanvas[z][y][x] = 2;
      }
      break;
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

void PWMCalc(int freq, int res, float duty) { // Phuoc Khang
  // 
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

void SPIControlHub(uint8_t (&canvas)[6][6][6]){ // Phuoc Khang
  SPIByteMapping(canvas[spi.layerIdx], SPIdata);
  // PWMCalc();
  SPIOutput(SPIdata, spi.layerIdx);
  
  if (micros() - spiCounter.time >= spiCounter.interval)
    spi.layerIdx++;
    if (spi.layerIdx > 5)
      spi.layerIdx = 0;
}
//-------------------------------------------------------
//                      DEBUGGING
//-------------------------------------------------------
void debugPrint() {
  for(int i = 0; i < 20; i++) Serial.println();
  Serial.println("Food coords: (" + String(food.x) + "," + String(food.y) + "," + String(food.z) + ")");
  Serial.println("SnakeDir: " + String(snakeDir));
  printSnake();
  printCanvas();
}

void printSnake() {
  Serial.print("Snake: ");
  for (int i = 0; i < bodySize; i++) {
    Serial.print(String(i) + " (" + String(snake[i].x) + "," + String(snake[i].y) + "," + String(snake[i].z) + ")" + '\t');
  }
  Serial.println();
}

void printCanvas() {
  // Prints gameCanvas
  Serial.println("gameCanvas Z level: 0 - 5. Don't ask why this works lmao.");

  // Funny loops, i know, it hurts my brain too but it worked so dont touch - PKhang
  for (int x = 5; x >= 0; x--) {
    for (int z = 0; z < 6; z++) {
      for (int y = 5; y >= 0; y--) {
        Serial.print(gameCanvas[z][y][x]);
        Serial.print(" ");
      }
      Serial.print('\t');
    }
    Serial.print('\n');
  }
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

void debugPinOutput() { // Phuoc Khang
  if (Serial.available() > 0) {
    bool valid = true;
    String cmdSelect = readWord();   // "fet", "mbi", "config", "debug", "game", "restart", {"w", "a", "s", "d", "q", "e"}
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
    
    // if move, skip mode and state
    if (cmdSelect != "move" && cmdSelect != "m") {
      cmdMode = readWord();
      /* {"0".."5"} - for fet
        {"data", "clock", "latch", "enable"} - for mbi
        {"spi", "gameInterval", "dabbleInterval", "foodInterval"} - for config
        {"game", "dabble", "serial"} - for debug
        {"start", "over"} - for game
      */

      // check cmdState for numbers
      cmdState = readWord();   // "1" or "0"
      // if "config spi", validate cmdState must be exactly one char and isdigit
      if (cmdMode == "spi" && (cmdState.length() != 1 || !isdigit(cmdState[0]))) {
        valid = false;
      }
      stateVal = cmdState.toInt();
    }

    if (valid && cmdSelect == "fet") {
      if (cmdMode.length() == 1 && isdigit(cmdMode[0])) {
        int index = cmdMode.toInt();
        if (index >= 0 && index < 6) {
          digitalWrite(FET[index], stateVal);
          Serial.println("FET pin " + cmdMode + " set to " + cmdState);
        } else {
          valid = false;
        }
      } else {
        valid = false;
      }
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
        fireworks.interval = cmdState.toInt();
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
        Serial.println("gameStart set to " + cmdState);
      }
      else if (cmdMode == "over") {
        gameOver = cmdState.toInt();
        Serial.println("gameOver set to " + cmdState);
      }
      else
        valid = false;
    }
    else if (valid) {
      char dir = cmdSelect[0];
      bool forward = 0, left = 0, backward = 0, right = 0, down = 0, up = 0;
      switch(dir) {
        case 'w':
          forward = true;
          break;
        case 'a':
          left = true;
          break;
        case 's':
          backward = true;
          break;
        case 'd':
          right = true;
          break;
        case 'q':
          down = true;
          break;
        case 'e':
          up = true;
          break;
        default:
          valid = false;
      }
      Serial.print("Move command received: " + String(dir) + '\t');
      Serial.println("W: " + String(forward) + " A: " + String(left) + " S: " + String(backward) + " D: " + String(right) + " Q: " + String(down) + " E: " + String(up));
      
      if (left  && snakeDir != RIGHT)           bufferDir = LEFT;
      else if (right && snakeDir != LEFT)       bufferDir = RIGHT;
      else if (up    && snakeDir != DOWN)       bufferDir = UP;
      else if (down  && snakeDir != UP)         bufferDir = DOWN;
      else if (forward && snakeDir != BACKWARD) bufferDir = FORWARD;
      else if (backward && snakeDir != FORWARD) bufferDir = BACKWARD;
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
    getInput();
    dabble.time = millis();
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
  else if (gameStart && gameOver && fireworksIdx < fireworksCount) {
    // Khi rắn đủ dài thì hiển thị hiệu ứng
    if (bodySize >= 6 && fireworksIdx == 0)
      fireworksStart = true;
      
    if (fireworksStart) {
      // 7 hiệu ứng, mỗi hiệu ứng chạy 10 frame (fireworksCount = 10)
      // Chuyển hiệu ứng sau mỗi 10 frame
      fireworks.effectIdx = (fireworksIdx / 10) % 7; 
      
      // Update hiệu ứng sau 250ms (fireworks.interval)
      if (millis() - fireworks.time >= fireworks.interval) {
        // Tăng chỉ số hiệu ứng
        fireworksIdx++;
        // Hiển thị hiệu ứng mới
        showFireworks();
        fireworks.time = millis();
      }
      
      // Tổng cộng 7 * 10 = 70 frame. Nếu đã chạy xong 70 frame (fireworksCount được đặt là 70)
      if (fireworksIdx >= 70) { // Giả sử chạy 7 hiệu ứng, mỗi hiệu ứng 10 lần.
        if (gameDebug) Serial.println("Game Over");
        fireworksStart = false;
        gameStart = false;
      }

      SPIControlHub(fireworksCanvas); // Hiển thị trên LED Cube

    } else {
      // Điều kiện bodySize < 6: chỉ hiển thị thông báo "Game Over" (hoặc hiệu ứng đơn giản)
      if (gameDebug) Serial.println("Game Over");
      fireworksIdx = fireworksCount; // Đặt index bằng count để thoát khỏi vòng lặp
      gameStart = false;
    }
  }
    else {
      if (gameDebug) Serial.println("Game Over");
      fireworksIdx = fireworksCount;
      gameStart = false;
    }
  }

  // Restart game *Once*
  else if (gameOver) {
    SPIControlHub(gameCanvas);
    if (gameStart) {
      if (gameDebug) Serial.println("Game Restarted");
      gameSetup();
      fireworksIdx = 0;
      gameOver = false;
    }
  }

  if (serialDebug) debugPinOutput();
}