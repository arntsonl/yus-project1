// Famicom Engine Core
#pragma once

#include "famisets.h"

enum {
    FC_NAMETABLE1 = 0,
    FC_NAMETABLE2 = 1
};

class FamiCore
{
public:
    FamiCore();
    ~FamiCore();

    bool setBGTile(int, unsigned int, bool);
    SpriteOAM * OAM(unsigned char);
    FamiRegisters * registers();
    void loadBGVRAM(SDL_Renderer *, SDL_Surface*);
    void loadSPRVRAM(SDL_Renderer *, SDL_Surface*);
    unsigned int * getNameTable(bool);
    unsigned int getTile(int, bool);

// Move these out of public when we don't want to debug
    SDL_Texture * m_sprVRAM;    // Sprites - where we copy from
    SDL_Texture * m_bgVRAM;     // Background - where we copy from

private:

    // Name Table 1 - Top Left, Name Table 3 - Mirror of Top left
    unsigned int m_nameTable1[0x3C0]; // (256 * 240 / 8)

    // Name Table 2 - Top Right, Name Table 4 - Mirror of Top Right
    unsigned int m_nameTable2[0x3C0]; // (256 * 240 / 8)

    // Sprite OAM
    SpriteOAM m_spriteOAM[OAM_CNT]; // 64 sprites

    // Famicom Registers
    FamiRegisters m_famiRegisters;
};
