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

    bool setBGTile(int, unsigned char, bool);
    SpriteOAM * OAM(unsigned char);
    FamiRegisters * registers();

    SDL_Surface * getBGVRAM(){ return m_bgVRAM;}
    SDL_Surface * getSPRVRAM(){ return m_sprVRAM; }

    unsigned char * getNameTable(bool);

    unsigned char getTile(int, bool);

private:

    // Name Table 1 - Top Left
    unsigned char m_nameTable1[0x3C0]; // (256 * 240 / 8)

    // Name Table 2 - Top Right
    unsigned char m_nameTable2[0x3C0]; // (256 * 240 / 8)

    // Name Table 3 - Mirror of Top Left
    // Name Table 4 - Mirror of Top Right

    // Sprite OAM
    SpriteOAM m_spriteOAM[OAM_CNT]; // 64 sprites

    // Sprite VRAM - $0xFF sprites in VRAM, or 255 sprites (8x8)
    SDL_Surface * m_sprVRAM;    // Sprites - where we copy from

    // Background VRAM - $0xFF backgrounds in VRAM, or 255 tiles (8x8)
    SDL_Surface * m_bgVRAM;     // Background - where we copy from

    // Famicom Registers
    FamiRegisters m_famiRegisters;
};
