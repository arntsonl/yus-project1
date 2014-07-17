// Famicom Engine Data Sets
#pragma once

// Everything we'll need
#include <SDL.h>
#include <SDL_image.h>

// For getcwd
#ifdef WINDOWS
    #include <direct.h>
    #define GetCurrentDir _getcwd
#else
    #include <unistd.h>
    #define GetCurrentDir getcwd
 #endif

#define J_A           1
#define J_B           2
#define J_SELECT      4
#define J_START       8
#define J_UP          16
#define J_DOWN        32
#define J_LEFT        64
#define J_RIGHT       128

#define K_A           0
#define K_B           1
#define K_SELECT      2
#define K_START       3
#define K_UP          4
#define K_DOWN        5
#define K_LEFT        6
#define K_RIGHT       7

#define VRAM_SIZE 256
#define TILE_SIZE 8
#define TILE_WIDTH 32
#define TILE_DIMEN 32*32
#define OAM_CNT 64

// Sprite OAM data
struct SpriteOAM
{
    unsigned int tile; // Which sprite tile?
    unsigned char x;    // SpriteX (8-bit)
    unsigned char y;    // SpriteY (8-bit)
    bool behind;        // display behind background? (1-bit)
    bool flipX;         // flip the X coordinate (1-bit)
    bool flipY;         // flip the Y coordinate (1-bit)
};

// Various PPU like registers
struct FamiRegisters
{
    unsigned char scrollX;      // Scroll X register (3 Bits)
    unsigned char scrollY;      // Scroll Y register (3 bits)
    unsigned char scrollX_NT;   // Scroll X*8   (5 bits) 0-31 before wrap
    unsigned char scrollY_NT;   // Scroll Y*8   (5 bits) 0-29 before wrap
    bool scrollXFlip;           // Flips when scroll X wraps from 31 to 0 (1 bit)
    bool scrollYFlip;           // Flips when scroll Y wraps from 29 to 0 (1 bit)
    unsigned int bgColor;      // Background color? (3-bit), None, Red, Gree, Blue (0,1,2,3)
    bool hideSprites;           // Hide the sprites?
    unsigned char joy1;         // Joystick input for Joy1
    unsigned char joy2;         // Joystick input for Joy2
};

// Some default colors
#define FAMI_BGRED 0xFF0000FF
#define FAMI_BGBLK 0x000000FF

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
#define RMASK 0xFF000000
#define GMASK 0x00FF0000
#define BMASK 0x0000FF00
#define AMASK 0x000000FF
#else
#define RMASK 0x000000FF
#define GMASK 0x0000FF00
#define BMASK 0x00FF0000
#define AMASK 0xFF000000
#endif

