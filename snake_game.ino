/* Snake Game using ESP32, OLED Display with IR remote controlling
 * The retro game Snake is now available for ESP32.
 * Control your snake with the IR remote to eat apples and achieve high scores.
 * Many levels to play!
 * Date: 08/07/2024
 * TODO: Explore using a linked-list structure for the snake instead of an array.
 * TODO: Two-player mode: Consider using separate IR remotes or adding joystick controls.
 * TODO: Modulize everything.
 */

#include <SPI.h>                // Communication
#include <Wire.h>
#include <Adafruit_GFX.h>       // OLED Display
#include <Adafruit_SH110X.h>
#include <IRremote.h>           // How about making your own IR receiver library? It's worth a try.

// COMMAND CODE of 17-key remote / FPT Remote / 20-key remote
// You should recheck command code by yourself.
#define UP_KEY          0x18    //18, 47, 18
#define LEFT_KEY        0x08    //08, 48, 10
#define RIGHT_KEY       0x5A    //5A, 4A, 4A
#define DOWN_KEY        0x52    //52, 4B, 5A
#define OK_KEY          0x1C    //1C, 49, A8

#define i2c_Address     0x3C
#define IR_PIN          25
#define SCREEN_WIDTH    128
#define SCREEN_HEIGHT   64
#define MAP_WIDTH       16      // divided by 8
#define MAP_HEIGHT      8

#define SNAKE_MAX_LENGTH 127
int8_t snake_x[SNAKE_MAX_LENGTH];
int8_t snake_y[SNAKE_MAX_LENGTH];
uint8_t snake_length = 1;
uint8_t score = 0;

enum direction{RIGHT = 0, UP, LEFT, DOWN};
uint8_t snake_dir = RIGHT;      // default direction
 
uint8_t food_x, food_y;

uint8_t levelIndex = 0;
int game_speed = 320;           // quite average

// 0 = free space, 1 = wall, 2 = your start position
uint8_t currentLevel[MAP_HEIGHT][MAP_WIDTH];
uint8_t levels[][MAP_HEIGHT][MAP_WIDTH] = {
  {{2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
   {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
   {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
   {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
   {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
   {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
   {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
   {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},

  {{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
   {1, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
   {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
   {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
   {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
   {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
   {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
   {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}},

   {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
   {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
   {1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
   {2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
   {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1},
   {0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0},
   {0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0},
   {0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0}},

  {{1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1},
   {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
   {1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1},
   {0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
   {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
   {1, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1},
   {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
   {1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1}},

  {{0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 1, 2, 0, 0, 0, 0},
   {0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 0, 0, 0},
   {0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0},
   {0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0},
   {0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0},
   {0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0},
   {0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 0, 0, 0},
   {0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 0, 0, 0}},

  {{2, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0},
   {0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0},
   {0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0},
   {0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0},
   {0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0},
   {0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0},
   {0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0},
   {0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0}},
  // Add more levels here...
};

Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
IRrecv irrecv(IR_PIN);
decode_results results;

void setup() 
{
  // Initializing services
  Serial.begin(9600);
  delay(300); // wait for the OLED to power up  
  display.begin(i2c_Address, true); // Address 0x3C default  
  display.display();
  delay(1000);

  // Welcome screen
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0, 0);
  display.println("Snake Game\n Group 05\n=Viet Duc=");     // =D
  display.display();
  irrecv.enableIRIn();
  delay(500);
  display.display();
  delay(1000);
  // Random seed
  randomSeed(analogRead(0));
  // Initializing game
  setGameSpeed();
  levelIndex = random(0, sizeof(levels) / sizeof(levels[0]));
  memcpy(currentLevel, levels[levelIndex], sizeof(currentLevel));
  findStartPosition();
  placeFood();
}

void loop() // game logic
{
  delay(game_speed);  // this affects game speed
  if (irrecv.decode()) 
  {
    switch (irrecv.decodedIRData.command){
      case UP_KEY:
        if (snake_dir != DOWN) snake_dir = UP;
        break;
      case LEFT_KEY:
        if (snake_dir != RIGHT) snake_dir = LEFT;
        break;
      case DOWN_KEY:
        if (snake_dir != UP) snake_dir = DOWN;
        break;
      case RIGHT_KEY:
        if (snake_dir != LEFT) snake_dir = RIGHT;
    }
    irrecv.resume();
  }
  
  for (int i = snake_length - 1; i > 0; i--) {
    snake_x[i] = snake_x[i - 1];
    snake_y[i] = snake_y[i - 1];
  }

  switch (snake_dir) {
    case RIGHT:
      snake_x[0]++;
      break;
    case UP:
      snake_y[0]--;
      break;
    case LEFT:
      snake_x[0]--;
      break;
    case DOWN:
      snake_y[0]++;
      break;
    }

  // Zap to the opposite edge
  if (snake_x[0] < 0) {
    snake_x[0] = MAP_WIDTH - 1;
  } else if (snake_x[0] >= MAP_WIDTH) {
    snake_x[0] = 0;
  }

  if (snake_y[0] < 0) {
    snake_y[0] = MAP_HEIGHT - 1;
  } else if (snake_y[0] >= MAP_HEIGHT) {
    snake_y[0] = 0;
  }

  // Eat Food?
  if (snake_x[0] == food_x && snake_y[0] == food_y) {
    if (snake_length < SNAKE_MAX_LENGTH) {
      snake_length++;
    }
    score++;
    placeFood();
  }

  // Crash into wall?
  if (currentLevel[snake_y[0]][snake_x[0]] == 1)
    gameOver();
  // Crash into itself?
  for (int i = 1; i < snake_length; i++) {
    if (snake_x[0] == snake_x[i] && snake_y[0] == snake_y[i]) {
      gameOver();
    }
  }
  // Update display
  display.clearDisplay();
  drawWall();
  drawSnake();
  drawFood();
  display.display();
}

/* Place food randomly.
 * Food must not be placed inside walls or snake body.
 * Maybe not optimal enough?
 */
void placeFood()
{
  bool canBePlaced = false;
  do {
    canBePlaced = false;
    food_x = random(0, MAP_WIDTH);
    food_y = random(0, MAP_HEIGHT);
    for(int i = 0; i < snake_length; i++) {
      if (snake_x[i] == food_x && snake_y[i] == food_y || currentLevel[food_y][food_x] == 1) {
        canBePlaced = true;
        continue;
      }
    }
  } while(canBePlaced);
}

/* Draw walls
 * Wall = 8x8 pixel blocks
 */
void drawWall() {
  for (int i = 0; i < MAP_HEIGHT; i++)
    for (int j = 0; j < MAP_WIDTH; j++)
      if (currentLevel[i][j] == 1)
        display.fillRect(j * 8, i * 8, 8, 8, SH110X_WHITE);
}

/* Draw snake body
 * One node of snake = 6x6 pixel blocks
 */
void drawSnake() {
  for (int i = 0; i < snake_length; i++) {
    display.fillRect(snake_x[i] * 8 + 1, snake_y[i] * 8 + 1, 6, 6, SH110X_WHITE);
  }
}

/* Draw food
 * Food = round of pixel blocks, radius = 3
 */
void drawFood() {
  // Fill Round with radius = 3 in a center of 8x8 pixel block
  display.fillCircle(food_x * 8 + 4, food_y * 8 + 4, 3, SH110X_WHITE);
}

// LOSER!
void gameOver() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(20, 20);
  display.println("YOU LOSE!");
  display.setCursor(20, 40);
  display.printf("Score:%d", score);
  display.display();
  delay(2000);
  Reset();
}

// Choose game speed (1-10) from the beginning
void setGameSpeed() {
  uint8_t speed_level = 11 - game_speed/80;
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0, 0);
  display.println("Choose\nGame Speed");
  display.setCursor(0, 40);
  display.printf("Current:%02d", speed_level);
  display.display();
  
  while (true) {
    if (irrecv.decode()) {
      switch (irrecv.decodedIRData.command){
        case DOWN_KEY:
        case LEFT_KEY:
          game_speed += 80;
          if (game_speed > 800) game_speed = 800;   // min game speed = 1
          break;
        case UP_KEY:
        case RIGHT_KEY:
          game_speed -= 80;
          if (game_speed < 80) game_speed = 80;     // max game speed = 10
          break;
        case OK_KEY:
          return; // exit
      }
      irrecv.resume();
      speed_level = 11 - game_speed/80;             // update
      display.clearDisplay();
      display.setTextSize(2);
      display.setTextColor(SH110X_WHITE);
      display.setCursor(0, 0);
      display.printf("Current:%02d", speed_level);
      display.setCursor(0, 20);
      if (speed_level > 7) display.println("LIKE A\nDRAGON!");
      else display.println("TOO SLOW=(");           // =D
      display.display();
    }
  }
}

// Find [2] = start position in level map
void findStartPosition(){
  for (int i = 0; i < MAP_HEIGHT; i++) {
    for (int j = 0; j < MAP_WIDTH; j++) {
      if (currentLevel[i][j] == 2){
        snake_x[0] = j;
        snake_y[0] = i;
        return;
      }
    }
  }
}

// Reset before new game 
void Reset() {
  levelIndex = random(0, sizeof(levels) / sizeof(levels[0]));
  memcpy(currentLevel, levels[levelIndex], sizeof(currentLevel));
  findStartPosition();
  score = 0;
  snake_length = 1;
  snake_dir = RIGHT;
  placeFood();
}
