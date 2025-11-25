#include <DabbleESP32.h>

#define BLUETOOTH_NAME "ESP32_Snake"

/*
  LƯU Ý: HỆ TRỤC TỌA ĐỘ CỦA LED CUBE ĐƯỢC ĐẶT TẠI GỐC DƯỚI PHẢI, GẦN PHÍA NGƯỜI XEM (Đặt POV ở người xem cục LED Cube)
  X        Z
    \      |
      \    |
        \  |
  Y ______\|

          Người xem (chìa tay phải ra là hiểu)

  SỬ DỤNG QUY TẮC BÀN TAY PHẢI (chỉa 3 ngón vuông góc với nhau: ngón cái, ngón trỏ, ngón giữa)
  -> NGÓN CÁI: trục Z, NGÓN TRỎ: trục X, NGÓN GIỮA: trục Y
  => Trục X là vị trí của dãy led ngang (đếm từ gần ra xa)
  => Trục Y là vị trí của dãy led dọc (đếm trừ phải sang trái)
  => Trục Z là mặt phẳng LED (đếm từ dưới lên trên)
*/

//################ GAME SETTINGS ################
float brightness;
unsigned int previousTime = millis();
int updateInterval = 1200; // miliseconds
int highscore;
bool gameStart = false;
bool gameOver = false;

//################ CANVAS ################
int debugDebounce = 10;
bool debug = true;
int debugFlag = 0;
bool dabbleDebug = false;
int dabbleDebugFlag = 0;
int canvas[6][6][6];

// Binary maps for 0-9 and A-Z based on ASCII decimal code
typedef uint8_t Glyph6[6][6];
const Glyph6 FONT6[128] = {

  /* 0–47: blank */
  #define BLANK {{0,0,0,0,0,0},{0,0,0,0,0,0},{0,0,0,0,0,0},{0,0,0,0,0,0},{0,0,0,0,0,0},{0,0,0,0,0,0}}

  BLANK,BLANK,BLANK,BLANK,BLANK,BLANK,BLANK,BLANK,BLANK,BLANK,BLANK,BLANK,
  BLANK,BLANK,BLANK,BLANK,BLANK,BLANK,BLANK,BLANK,BLANK,BLANK,BLANK,BLANK,
  BLANK,BLANK,BLANK,BLANK,BLANK,BLANK,BLANK,BLANK,BLANK,BLANK,BLANK,BLANK,
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
int bodySize = 3;

enum Directions {
  FORWARD,
  BACKWARD,
  RIGHT,
  LEFT,
  UP,
  DOWN
};
int snakeDir = random(0, 3);
int bufferDir = snakeDir;

//################ FOOD ################
Coords food;

void setup() {
  Serial.begin(115200);
  Dabble.begin(BLUETOOTH_NAME);
  Serial.println("Dabble ready. Waiting for connection...");
  clearCanvas();
  initSnake();   
  initFood();
}

void loop() {
  getInput();

  // Pre-game
  if (!gameStart && !gameOver)
    Serial.println("Game hasn't started yet, press 'Start' to start =)).");

  // In-game
  else if (gameStart && !gameOver && millis() - previousTime >= updateInterval) { // Cứ mỗi chu kì Interval thì nó sẽ update game
    updateGameState();
    previousTime = millis(); // Đặt lại thời gian hiện tại
  } 

  // Game Over and Fireworks
  else if (gameStart && gameOver) {
    if (bodySize >= 6) {
      showFireworks();  // Khi rắn đủ dài thì hiển thị pháo hoa
    }
    Serial.println("Game Over");
    gameStart = false;
  }

  // Restart game *Once*
  else if (!gameStart && gameOver) {
    // Nếu chưa chạy → bắt đầu ván mới
    clearCanvas();
    initSnake();   
    initFood();
    bodySize = 3;
    snakeDir = random(0, 3);
    gameOver = false;
  }

  delay(50);
}

void clearCanvas() {
  for (int z = 0; z < 6; z++) {
    for (int y = 0; y < 6; y++) {
      for (int x = 0; x < 6; x++) {
        canvas[z][y][x] = 0;
      }
    }
  }
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
  if (GamePad.isCirclePressed()) debugFlag += 1;
  else debugFlag = 0;
  if (GamePad.isSquarePressed()) dabbleDebugFlag += 1;
  else dabbleDebugFlag = 0;

  if (debugFlag >= debugDebounce) {
    debug = !debug;
    Serial.println("Game debug toggled: " + String(debug));
    debugFlag = 0;
  }
  if (dabbleDebugFlag >= debugDebounce) {
    dabbleDebug = !dabbleDebug;
    Serial.println("Dabble debug toggled: " + String(dabbleDebug));
    dabbleDebugFlag = 0;
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
  if (start) gameStart = true;

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

  // Update canvas
  clearCanvas();
  renderScene();
  // SPIPWM();

  // Serial debug
  if (debug) debugPrint();
}

//mức sáng: 0 = tắt; 1 = thân sáng vừa; 2 = đầu và táo sáng nhất
void renderScene() {
  // Thân rắn = 1, Đầu rắn = 2
  for (int i = 0; i < bodySize; i++) {    
    Coords p = snake[i];
    if (i == 0) canvas[p.z][p.y][p.x] = 2;
    else canvas[p.z][p.y][p.x] = 1;
  }
  // Táo = 2
  canvas[food.z][food.y][food.x] = 2;
}

void SPIPWM(int freq, int res, float duty) { // Phuoc Khang
  // pending
}

void renderChar() {
  // pending
}

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
  // Prints Canvas
  Serial.println("Canvas Z level: 0 - 5. Don't ask why this works lmao.");

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

//------------------ HIỆU ỨNG PHÁO HOA ------------------
void showFireworks() {
  for (int t = 0; t < 10; t++) { // 10 đợt nổ
    clearCanvas(); // Xóa toàn bộ LED

    int centerX = random(1,5);
    int centerY = random(1,5);
    int centerZ = random(1,5);

    for (int i = 0; i < 12; i++) {
      int dx = random(-2,3);
      int dy = random(-2,3);
      int dz = random(-2,3);
      int x = constrain(centerX + dx, 0, 5);
      int y = constrain(centerY + dy, 0, 5);
      int z = constrain(centerZ + dz, 0, 5);
      canvas[z][y][x] = 2;  // LED sáng mạnh
    }

    printCanvas();  // hoặc hàm render LED thực tế
    delay(250);
  }

  clearCanvas();
  printCanvas(); // Tắt LED sau pháo hoa
}