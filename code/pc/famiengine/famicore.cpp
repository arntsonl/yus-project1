#include "famicore.h"

FamiCore::FamiCore()
{
    memset(m_nameTable1, 0, 0x3C0);
    memset(m_nameTable2, 0, 0x3C0);
    memset(m_spriteOAM, 0, sizeof(SpriteOAM)*OAM_CNT);

    // Create our textures from this surface
    m_sprVRAM = NULL;
    m_bgVRAM = NULL;

    memset(&m_famiRegisters, 0, sizeof(FamiRegisters));
}

FamiCore::~FamiCore()
{
    // Free some of our memory
    if ( m_sprVRAM != NULL )
        SDL_DestroyTexture(m_sprVRAM);
    if ( m_bgVRAM != NULL )
        SDL_DestroyTexture(m_bgVRAM);
}

SpriteOAM * FamiCore::OAM(unsigned char sprIdx)
{
    if ( sprIdx < OAM_CNT)
        return &m_spriteOAM[sprIdx];
    return NULL;
}

bool FamiCore::setBGTile(int tileIdx, unsigned int tile, bool nameTable)
{
    unsigned int * writeTable = NULL;
    if ( nameTable == FC_NAMETABLE1 )
        writeTable = m_nameTable1;
    else if ( nameTable == FC_NAMETABLE2 )
        writeTable = m_nameTable2;
    if ( tileIdx < 0x3C0 && writeTable)
    {
        writeTable[tileIdx] = tile - 1;
        return true; // everything went fine
    }
    return false;
}

void FamiCore::loadBGVRAM(SDL_Renderer * renderer, SDL_Surface* surf)
{
    m_bgVRAM = SDL_CreateTextureFromSurface(renderer, surf);
}

void FamiCore::loadSPRVRAM(SDL_Renderer * renderer, SDL_Surface* surf)
{
    m_sprVRAM = SDL_CreateTextureFromSurface(renderer, surf);
}

unsigned int * FamiCore::getNameTable(bool nameTable)
{
    unsigned int * retTable = NULL;
    if ( nameTable == FC_NAMETABLE1 )
        retTable = m_nameTable1;
    else if ( nameTable == FC_NAMETABLE2 )
        retTable = m_nameTable2;
    return retTable;
}

unsigned int FamiCore::getTile(int idx, bool nameTable)
{
    unsigned int * readTable = NULL;
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
