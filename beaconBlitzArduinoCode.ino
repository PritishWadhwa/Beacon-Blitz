#include "LedControl.h"
#include<SoftwareSerial.h>
#include "Bounce2.h"
#include<binary.h>
#include<Timer.h>

#define DIN 11
#define CLK 12
#define CS 13
#define BTN 6
#define POT_PIN A0
#define BRICK_WT 2
#define ORB_WEIGHT 1
#define INTERV 80
#define CHK_FRQ 10
#define NUM_ARENAS 10

Timer t;
LedControl lc = LedControl(DIN, CLK, CS, 1);
SoftwareSerial bt(2, 3);
Bounce debouncer = Bounce();

boolean gameOn = true;

uint8_t spritePerimeter[NUM_ARENAS][8] = {
  {B00000000, B01010110, B01000010, B00000010, B01000000, B01000010, B01101010, B00000000},
  {B00000000, B01110110, B01000010, B00000010, B01000000, B01000010, B01111110, B00000000},
  {B00000000, B00000000, B01000010, B01000010, B01000010, B01000010, B01111110, B00000000},
  {B00000000, B01100010, B01000010, B01000010, B01000010, B01000010, B01000110, B00000000},
  {B00000000, B01111110, B00000010, B01000010, B00000010, B01000000, B01010110, B00000000},
  {B00000000, B01111110, B00000010, B01010010, B00010010, B01010000, B01110110, B00000000},
  {B00000000, B01111010, B00000000, B01010010, B00000010, B01010000, B01010110, B00000000},
  {B00000000, B01011010, B00001000, B01001010, B01001010, B01000000, B01011000, B00000000},
  {B00000000, B01000010, B00000000, B01011010, B01001110, B01000000, B01011100, B00000000},
  {B00000000, B01001010, B01011000, B01000010, B00000010, B01001010, B01101110, B00000000}
};

uint8_t grid[8][8];

const int GM_DUR = 60 * 1000;

int st_millis = 0;
int hit_count = 0;

void LedOn(int x, int y) {
  lc.setLed(0, y, x, true);
}
void LedOff(int x, int y) {
  lc.setLed(0, y, x, false);
}

void makePerimeter(int k) {
  for (int i = 1; i < 7; i++) {
    lc.setRow(0, i, spritePerimeter[k][i]);
  }
}

uint8_t orb_x = 5;
uint8_t orb_y = 5;

class Bullet
{
  public:
    uint8_t x;
    uint8_t y;
    char dir;
    boolean isActive = false;
    unsigned int p_tm;
    unsigned int n_tm;

    void shootAndUpdate() {
      if (!hasEnded()) {
        n_tm = millis();
        if ((n_tm - p_tm) >= INTERV) {
          LedOff(x, y);
          switch (dir) {
            case 'r':
              ++x;
              break;
            case 'l':
              --x;
              break;
            case 'f':
              --y;
              break;
            case 'b':
              ++y;
              break;
          }
          LedOn(x, y);
          p_tm = n_tm;
        }
      }
    }

    boolean hasEnded() {
      if (x > 7 || x < 0 || y > 7 || y < 0) {
        isActive = false;
        return false;
      }
      else if (grid[y][x] == BRICK_WT) {
        isActive = false;
        return false;
      }
      else if (grid[y][x] == ORB_WEIGHT) {
        ++hit_count;
        orb_scramble();
        return true;
      }
    }

    void spawn(uint8_t _x, uint8_t _y) {
      x = _x; y = _y;
      LedOn(x, y);
      p_tm = millis();
      isActive = true;
    }

};
Bullet b = Bullet();

class Player
{
  public:
    uint8_t x;
    uint8_t y;
    char action;
    boolean isVisible = true;

    Player(uint8_t _x, uint8_t _y): x(_x), y(_y) {}

    void movePl() {
        LedOff(x, y);
        grid[y][x] = 0;
        switch (action) {
          case 'R':
            ++x;
            break;
          case 'L':
            --x;
            break;
          case 'F':
            --y;
            break;
          case 'B':
            ++y;
            break;
        grid[y][x] = ORB_WEIGHT;
        if (isVisible) {
          LedOn(x, y);
        }
      }
    }

    void hide() {
      isVisible = !isVisible;
      lc.setLed(0, y, x, isVisible);
    }

    void scramble() {
      while (true) {
        LedOff(x, y);

        if (grid[rnd_r][rnd_c] == 0) {
          y = random(0, 7);
          x = random(0, 7);
          if (isVisible) {
            LedOn(x, y);
          }
          break;
      }
    }

    void spawn() {
      grid[y][x] = 1;
      LedOn(x, y);
    }
};
Player p = Player(0, 0);

void inputHandler() {
  if (bt.available() > 0) {
    char ch = bt.read();

    if (ch == 'f' || ch == 'b' || ch == 'r' || ch == 'l') {
      b.dir = ch;
      switch (ch) {
        case 'f':
          b.spawn(p.x, p.y - 1);
          break;
        case 'b':
          b.spawn(p.x, p.y + 1);
          break;
        case 'r':
          b.spawn(p.x + 1, p.y);
          break;
        case 'l':
          b.spawn(p.x - 1, p.y);
          break;
      }
    }
    else {
      p.action = ch;
      p.movePl();
    }
  }
}

void displayScore() {
  lc.clearDisplay(0);
  for (int i = 0; i < 8; i++) {
    lc.setRow(0, i, spriteScore[hit_count][i]);
  }
}

void setup() {
  pinMode(BTN, INPUT_PULLUP); pinMode(POT_PIN, INPUT);

  randomSeed(analogRead(1));
  Serial.begin(9600);

  lc.shutdown(0, false); lc.setIntensity(0, 1);

  while (true) {
    int potVal = map(analogRead(POT_PIN), 15, 960, 0, NUM_ARENAS - 1);
    makePerimeter(potVal);

    bt.begin(9600);
    if (bt.available() > 0) {
      if (bt.read() == 'g') {
        p.spawn();
        break;
      }
    }
  }

  t.every(2 * 1000, orb_scramble);

  bt.begin(9600);
  orb_spawn();
  st_millis = millis();
}

uint8_t i = 1;
void loop() {
  while (millis() - st_millis <= GM_DUR) {
    if (b.isActive) {
      b.shootAndUpdate();
    }
    t.update();
  }
  while (i--) {
    displayScore();
  }
}