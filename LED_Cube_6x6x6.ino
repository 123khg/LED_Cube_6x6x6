#include <DabbleESP32.h>  // Install "DabbleESP32" library

// Define your Bluetooth name
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
bool gamestart = false;   
bool gameover  = false;  

//################ CANVAS / MAP ################
int canvas[6][6][6];

//################ SNAKE ################
struct Coords {
  int x;
  int y;
  int z;
};
Coords snake[216];
int bodySize = 2;

enum Directions {
  FORWARD,
  BACKWARD,
  RIGHT,
  LEFT,
  UP,
  DOWN
};
int snakeDir = LEFT;

//################ FOOD ################
Coords food;

void setup() {
  Serial.begin(115200);
  // Initialize Dabble
  // Dabble.begin(BLUETOOTH_NAME);
  Serial.println("Dabble ready. Waiting for connection...");
}

void loop() {
  // getInput();
  updateGameState();


  // Debug 
  // printSnake();
  // printMap();

  delay(50);
}

void initMap() {
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

// void getInput() {
//   Dabble.processInput();

//   // Read 6 buttons: Up, Down, Left, Right, Start, Select
//   bool up     = GamePad.isUpPressed();
//   bool down   = GamePad.isDownPressed();
//   bool left   = GamePad.isLeftPressed();
//   bool right  = GamePad.isRightPressed();
//   bool start  = GamePad.isTrianglePressed();  // or use any custom mapping
//   bool select = GamePad.isCirclePressed();

//   Serial.print("U:"); Serial.print(up);
//   Serial.print(" D:"); Serial.print(down);
//   Serial.print(" L:"); Serial.print(left);
//   Serial.print(" R:"); Serial.print(right);
//   Serial.print(" S:"); Serial.print(start);
//   Serial.print(" C:"); Serial.println(select);

//   delay(100);
// }

void updateGameState() { // Nhat Huy
  // Nếu chưa chạy → bắt đầu ván mới
  if (!gamestart && !gameover) {
    initMap();
    initSnake();   
    initFood();    
    gamestart = true;
    gameover  = false;
    if (bodySize >= 6) {
      showFireworks();  // Khi rắn đủ dài thì hiển thị pháo hoa
    }
     return; 
  }

  if (gamestart && !gameover && millis() - previousTime >= updateInterval) { // Cứ mỗi chu kì Interval thì nó sẽ update game
    // Tạo vector chuyển động và đầu mới
    Coords coordIncrease = {0, 0, 0}; // X, Y, Z
    if (snakeDir == FORWARD) coordIncrease.x = 1;
    if (snakeDir == BACKWARD) coordIncrease.x = -1;
    if (snakeDir == RIGHT) coordIncrease.y = -1;
    if (snakeDir == LEFT) coordIncrease.y = 1;
    if (snakeDir == UP) coordIncrease.z = 1;
    if (snakeDir == DOWN) coordIncrease.z = -1;
    Coords newHead = {snake[0].x + coordIncrease.x, snake[0].y + coordIncrease.y, snake[0].z + coordIncrease.z};

    // Ăn táo
    bool ate = (newHead.x == food.x && newHead.y == food.y && newHead.z == food.z);
    if (ate && bodySize < 216) {
      bodySize++;      // rắn dài thêm 1
      initFood();      // spawn táo mới (không trùng rắn)
    }
    else {
      for (int i = bodySize - 1; i > 0; i--) {
        //  Chạm cơ thể → đứng yên
        if (snake[i].x == newHead.x && snake[i].y == newHead.y && snake[i].z == newHead.z) {
          gameover  = true;
          gamestart = false;
          return; // Cần điều chỉnh cái này !!!!!!!!!!!!!!!!!!
        }
        else snake[i] = snake[i-1]; // Cho rắn duy chuyển phía trước, đẩy đuôi lên đốt tiếp theo
      }
    }

    snake[0] = newHead;

    // Kiểm tra có đụng tường ko -> sang tường bên kia

    previousTime = millis(); // Đặt lại thời gian hiện tại
  }
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

}

void printSnake() {
  for (int i = 0; i < 5; i++){
    Serial.print(String(snake[i].x) + "," + String(snake[i].y) + "," + String(snake[i].z) + '\t');
  }
  Serial.println();
}

void printCanvas() {
  for (int i = 0; i < 20; i++) {
    Serial.println(); // Prints a newline character ('\r' and '\n')
  }

  // Prints Canvas
  for (int z = 0; z < 6; z++) {
    for (int y = 0; y < 6; y++) {
      for (int x = 0; x < 6; x++) {
        Serial.print(canvas[z][y][x]);
        Serial.print(" ");
      }
      Serial.print('\n');
    }
  }
}
//------------------ HIỆU ỨNG PHÁO HOA ------------------
void showFireworks() {
  for (int t = 0; t < 10; t++) { // 10 đợt nổ
    initMap(); // Xóa toàn bộ LED

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
    delay(180);
  }

  initMap();
  printCanvas(); // Tắt LED sau pháo hoa
}
