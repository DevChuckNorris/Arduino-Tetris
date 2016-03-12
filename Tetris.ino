#include <Adafruit_GFX.h>
#include <Adafruit_TFTLCD.h>
#include <TouchScreen.h>

#define LCD_CS A3
#define LCD_CD A2
#define LCD_WR A1
#define LCD_RD A0
#define YP A3
#define XM A2
#define YM 9
#define XP 8

#define TS_MINX 150
#define TS_MINY 120
#define TS_MAXX 920
#define TS_MAXY 940
#define MINPRESSURE 10
#define MAXPRESSURE 1000

#define LCD_RESET A4

#define BLOCK_SIZE 15
#define PLAYFIELD_X 3
#define PLAYFIELD_Y 15

Adafruit_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

bool blocks[7][8] = {
  { true, true, true, true, false, false, false, false },
  { true, false, false, false, true, true, true, false },
  { false, false, true, false, true, true, true, false },
  { true, true, false, false, true, true, false, false },
  { false, true, true, false, true, true, false, false },
  { false, true, false, false, true, true, true, false },
  { true, true, false, false, false, true, true, false }
};

bool blockSize[] = {
  false,
  true,
  true,
  true,
  true,
  true,
  true
};

byte blockWidth[] = {
  4, 3, 3, 2, 3, 3, 3
};

short colors[] {
  0x079E,
  0x001E,
  0xF500,
  0xF780,
  0x0780,
  0xA01E,
  0xF000
};

short borderColors[] = {
  0x05DF,
  0x0017,
  0xF420,
  0xD6E0,
  0x0660,
  0x9015,
  0xA800
};

byte playField[10][20];

byte nextBlock = 1;
byte currentBlock = 0;

byte currentPosX = 0;
byte currentPosY = 0;

bool update = false;
long nextDrop = 0;

void setup() {
  Serial.begin(9600);
  
  for(int x = 0; x < 10; x++) {
    for(int y = 0; y < 20; y++) {
      playField[x][y] = 0xFF;
    }
  }
  
  tft.reset();

  uint16_t identifier = tft.readID();

  tft.begin(identifier);
  tft.reset();
  tft.setRotation(2);
  
  tft.fillScreen(0x0);
  
  tft.setCursor(tft.width() - 75, 8);
  tft.print("Next Block:");
  drawBlock(nextBlock, tft.width() - 75, 18);
  
  drawPlayField();
}

void loop() {
  if(update) {
    drawPlayField();
    update = false;
  }
  
  TSPoint p = ts.getPoint();
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);
  if (p.z > MINPRESSURE && p.z < MAXPRESSURE) {
    update = true;
    if(p.x <= 512) {
      currentPosX--;
      if(currentPosX < 0) currentPosX = 0;
    } else {
      currentPosX++;
      if(currentPosX > 10 - blockWidth[currentBlock]) currentPosX = 10 - blockWidth[currentBlock];
    }
  }
  
  //drawBlock(currentBlock, PLAYFIELD_X + currentPosX * BLOCK_SIZE, PLAYFIELD_Y + currentPosY * BLOCK_SIZE);
  //playField[random(0, 10)][random(0, 20)] = random(0, 7);
  
  if(millis() > nextDrop) {
    update = true;
    
    currentPosY++;
    if(!checkPos()) {
      
      makeSolid();
      currentBlock++;
      if(currentBlock > 6) currentBlock = 0;
      nextBlock++;
      if(nextBlock > 6) nextBlock = 0;
      if(currentPosX > 10 - blockWidth[currentBlock]) currentPosX = 10 - blockWidth[currentBlock];
      
      currentPosY = 0;
      
      if(!checkPos()) {
        tft.fillScreen(0x00);
        delay(100);
        tft.fillScreen(0xffff);
        delay(100);
        tft.fillScreen(0x00);
        delay(100);
        asm volatile("jmp 0");
      }
    }
    
    nextDrop = millis() + 500;
  }
  
  //delay(100);
}

bool checkPos() {
  if(currentPosY > 19 - blockSize[currentBlock]) return false;
  
  for(int x = 0; x < 4; x++) {
    for(int y = 0; y < 2; y++) {
      if(blocks[currentBlock][x + y * 4]) {
        if(playField[currentPosX + x][currentPosY + y] != 0xFF) {
          return false;
        }
      }
    }
  }
  
  return true;
}

void makeSolid() {
  for(int x = 0; x < 4; x++) {
    for(int y = 0; y < 2; y++) {
      if(blocks[currentBlock][x + y * 4]) {
        playField[currentPosX + x][currentPosY + y - 1] = currentBlock;
      }
    }
  }
}

bool isCurrent(byte x, byte y) {
  if(currentPosX > x || currentPosY > y) return false;
  if(currentPosX + 3 < x || currentPosY + 1 < y) return false;
  
  //return true;
  
  byte relX = x - currentPosX;
  byte relY = y - currentPosY;
  
  return blocks[currentBlock][relX + relY * 4];
}

void drawPlayField() {
  for(int x = 0; x < 10; x++) {
    for(int y = 0; y < 20; y++) {
      uint16_t screenX = PLAYFIELD_X + x * BLOCK_SIZE;
      uint16_t screenY = PLAYFIELD_Y + y * BLOCK_SIZE;
      
      if(isCurrent(x, y)) {
        drawSingle(currentBlock, screenX, screenY);
      } else {
        if(playField[x][y] == 0xFF) {
          tft.drawRect(screenX, screenY, BLOCK_SIZE, BLOCK_SIZE, 0xFFFF);
          tft.fillRect(screenX + 1, screenY + 1, BLOCK_SIZE - 2, BLOCK_SIZE - 2, isCurrent(x, y) ? 0xF00F : 0x0000);
        } else {
          drawSingle(playField[x][y], screenX, screenY);
        }
      }
    } 
  }
}

void drawBlock(byte block, int x, int y) {
  for(int a = 0; a < 4; a++) {
    for(int b = 0; b < 2; b++) {
      if(blocks[block][a + b * 4]) {
        tft.fillRect(x + a * BLOCK_SIZE, y + b * BLOCK_SIZE, BLOCK_SIZE, BLOCK_SIZE, colors[block]);
        tft.drawRect(x + a * BLOCK_SIZE, y + b * BLOCK_SIZE, BLOCK_SIZE, BLOCK_SIZE, borderColors[block]);
      }
    }
  }
}

void drawSingle(byte block, int x, int y) {
  tft.fillRect(x, y, BLOCK_SIZE, BLOCK_SIZE, colors[block]);
  tft.drawRect(x, y, BLOCK_SIZE, BLOCK_SIZE, borderColors[block]);
}
