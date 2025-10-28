// #include <DabbleESP32.h>  // Install "DabbleESP32" library

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

  initMap();
  initSnake();
  initFood();
}

void loop() {
  // getInput();
  updateGameState();


  // Debug 
  printSnake();
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

void updateGameState() {
  if (millis() - previousTime >= updateInterval) { // Cứ mỗi chu kì Interval thì nó sẽ update game
    // Cho rắn duy chuyển phía trước
    // Kiểm tra có đụng tường ko -> sang tường bên kia
    // Kiểm tra có chạm cơ thể ko
    // Kiểm tra có ăn táo ko -> update độ dài


    previousTime = millis(); // Đặt lại thời gian hiện tại
  }
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