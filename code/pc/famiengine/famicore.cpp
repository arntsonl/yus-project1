#include "famicore.h"

FamiCore::FamiCore()
{
    memset(m_nameTable1, 0, 0x3C0);
    memset(m_nameTable2, 0, 0x3C0);
    memset(m_spriteOAM, 0, sizeof(SpriteOAM)*OAM_CNT);

    m_sprVRAM = SDL_CreateRGBSurface(0, VRAM_SIZE, VRAM_SIZE, 32, RMASK, GMASK, BMASK, AMASK);
    m_bgVRAM = SDL_CreateRGBSurface(0, VRAM_SIZE, VRAM_SIZE, 32, RMASK, GMASK, BMASK, AMASK);

    memset(&m_famiRegisters, 0, sizeof(FamiRegisters));
}

FamiCore::~FamiCore()
{
    // Free some of our memory
    SDL_FreeSurface(m_sprVRAM);
    SDL_FreeSurface(m_bgVRAM);
}

SpriteOAM * FamiCore::OAM(unsigned char sprIdx)
{
    if ( sprIdx < OAM_CNT)
        return &m_spriteOAM[sprIdx];
    return NULL;
}

bool FamiCore::setBGTile(int tileIdx, unsigned char tile, bool nameTable)
{
    unsigned char * writeTable = NULL;
    if ( nameTable == FC_NAMETABLE1 )
        writeTable = m_nameTable1;
    else if ( nameTable == FC_NAMETABLE2 )
        writeTable = m_nameTable2;
    if ( tileIdx < 0x3C0 && writeTable)
    {
        writeTable[tileIdx] = tile;
        return true; // everything went fine
    }
    return false;
}

unsigned char * FamiCore::getNameTable(bool nameTable)
{
    unsigned char * retTable = NULL;
    if ( nameTable == FC_NAMETABLE1 )
        retTable = m_nameTable1;
    else if ( nameTable == FC_NAMETABLE2 )
        retTable = m_nameTable2;
    return retTable;
}

unsigned char FamiCore::getTile(int idx, bool nameTable)
{
    unsigned char * readTable = NULL;
    if ( nameTable == FC_NAMETABLE1 )
        readTable = m_nameTable1;
    else if ( nameTable == FC_NAMETABLE2 )
        readTable = m_nameTable2;
    if ( idx < 0x3C0 && readTable)
    {
        return readTable[idx]; // everything went fine
    }
    return 0x0; // return tile 0 (no error?)
}

FamiRegisters * FamiCore::registers()
{
    return &m_famiRegisters;
}
