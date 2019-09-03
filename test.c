#include <stdlib.h>
#include <string.h>

// include NESLIB header
#include "neslib.h"

// include CC65 NES Header (PPU)
#include <nes.h>

#include "map1.h"

//#link "char.s"

/*{pal:"nes",layout:"nes"}*/
const unsigned char palette[16]={ 0x30,0x2c,0x3c,0x03,0x30,0x1a,0x1a,0x2a,0x30,0x06,0x16,0x26,0x30,0x09,0x19,0x29 };

const unsigned char catA[]={
	  8,-16,0x11,0|OAM_FLIP_H,
	  0,-16,0x12,0|OAM_FLIP_H,
	  8,- 8,0x21,0|OAM_FLIP_H,
	  8,-24,0x01,0|OAM_FLIP_H,
	  0,-24,0x01,0,
	  0,- 8,0x22,0|OAM_FLIP_H,
	- 8,-16,0x13,0|OAM_FLIP_H,
	- 8,- 8,0x23,0|OAM_FLIP_H,
	 16,-24,0x02,0|OAM_FLIP_H,
	- 8,-24,0x02,0,
	-16,-16,0x14,0|OAM_FLIP_H,
	-16,- 8,0x24,0|OAM_FLIP_H,
	 16,-16,0x10,0|OAM_FLIP_H,
	-16,-24,0x03,0|OAM_FLIP_H,
	128
};
const unsigned char catB[]={
	-16,-16,0x11,0,
	- 8,-16,0x12,0,
	-16,- 8,0x21,0,
	-16,-24,0x01,0,
	- 8,-24,0x01,0|OAM_FLIP_H,
	- 8,- 8,0x22,0,
	  0,-16,0x13,0,
	  0,- 8,0x23,0,
	-24,-24,0x02,0,
	  0,-24,0x02,0|OAM_FLIP_H,
	  8,-16,0x14,0,
	  8,- 8,0x24,0,
	-24,-16,0x10,0,
	  8,-24,0x03,0,
	128
};

#define tileSize 8
#define screenWidth 32
#define screenHeight 30
#define tileCount (screenWidth*screenHeight)
#define clamp(min,max,val) val<min?min:val>max?max:val

#define catFriction 1
#define maxCatSpeed 3
#define maxCatFallSpeed 5
#define catJumpStrength 10

typedef struct {
  int x;
  int y;
  int sX;
  int sY;
  const unsigned char* sprite;
} actor;

actor cat;

char tileEmpty(int x, int y, int* offsetX, int* offsetY) {
  int tX = x / tileSize;
  int tY = y / tileSize;
  int offset = tY * screenWidth + tX;
  char tileData = map1[offset];
  (*offsetX) = tX % tileSize;
  (*offsetY) = tY % tileSize;
  return tileData == 0x00;
}

void catCollided(char * collidedDown, char * collidedUp, char * coliddedLeft, char * coliddedRight) {
  int offsetX;
  int offsetY;
  *collidedDown = 
    !tileEmpty(cat.x, cat.y, &offsetX, &offsetY) |
    !tileEmpty(cat.x - tileSize, cat.y, &offsetX, &offsetY) |
    !tileEmpty(cat.x + tileSize, cat.y, &offsetX, &offsetY);
  if (*collidedDown) {
    cat.y = (cat.y / tileSize) * tileSize;
    cat.sY = 0;
  }
  *collidedUp = 
    !tileEmpty(cat.x, cat.y - tileSize, &offsetX, &offsetY) |
    !tileEmpty(cat.x - tileSize, cat.y - tileSize, &offsetX, &offsetY) |
    !tileEmpty(cat.x + tileSize, cat.y - tileSize, &offsetX, &offsetY);
  //if (*collidedUp) {
  //  cat.y += offsetY;
  //  cat.sY = 0;
  //}
  *coliddedRight = 
    !tileEmpty(cat.x + tileSize, cat.y - tileSize, &offsetX, &offsetY);
  if (*coliddedRight) {
    cat.x -= offsetX;
    cat.sX = 0;
  }
  *coliddedLeft = 
   !tileEmpty(cat.x - tileSize, cat.y - tileSize, &offsetX, &offsetY);
   if (*coliddedLeft) {
    cat.x += offsetX;
    cat.sX = 0;
  }
}

void main(void) {
  char pad;
  char oam_id;
  char catColDown;
  char catColUp;
  char catColLeft;
  char catColRight;
  char horizontalMove;

  cat.sprite = catA;
  cat.x = 32;
  cat.y = 32;

  vram_adr(NAMETABLE_A);
  vram_write(map1, sizeof(map1));
  vram_write(map1Attr, sizeof(map1Attr));
  
  oam_clear();
  pal_bg(palette);
  pal_spr(palette);
  ppu_on_all();

  while (1) {
    horizontalMove = 0;
    catCollided( &catColDown, &catColUp, &catColLeft, &catColRight);
    pad = pad_poll(0);
    if (pad & PAD_LEFT && !catColLeft) {
      if (cat.sX > -maxCatSpeed) {
        cat.sX--;
      }
      cat.sprite = catB;
      horizontalMove = 1;
    } 
    if (pad & PAD_RIGHT && !catColRight) {
      if (cat.sX < maxCatSpeed) {
        cat.sX++;
      }
      cat.sprite = catA;
      horizontalMove = 1;
    }
    if (pad & PAD_A && catColDown && !catColUp) {
      cat.sY -= catJumpStrength;
    }
    if (!horizontalMove) {
      if (cat.sX > 0) {
        cat.sX -= catFriction;
      } else if (cat.sX < 0) {
        cat.sX += catFriction;
      }
    }
    cat.x = clamp(0, screenWidth * tileSize, cat.x + cat.sX);
    cat.y = clamp(0, screenHeight * tileSize, cat.y + cat.sY);
    oam_id = 0;
    oam_id = oam_meta_spr(cat.x, cat.y, oam_id, cat.sprite);
    if (oam_id != 0) {
      oam_hide_rest(oam_id);
    }
    if (!catColDown && cat.sY < maxCatFallSpeed) {
      cat.sY++;
    }
    ppu_wait_frame();
  }
}