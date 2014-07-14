//
// FamiEngine main header
//
//   This engine partially emulates how an NES handles a few of its drawing,
//  sound, and input routine. Use freely, spread freely.
//
#pragma once

#include "famicore.h"
#include "famisound.h"
#include "famisets.h"

enum {
    VIDEO_NTSC = 0,
    VIDEO_PAL = 1
};

// Famicom update return enums
enum {
    FAMI_IDLE = 0,
    FAMI_RESET,
    FAMI_EXIT
};

class FamiCore; // forward declaration

class FamiEngine
{
public:
    FamiEngine();
    ~FamiEngine();
    void setVideo(bool); // NTSC or PAL
    void setBg(unsigned int);    // Set Background Color
    bool init(char*, int, int, bool);    // Window Title, width, height, fullscreen?
    int update();

    void setBGData(int, unsigned char, bool); // tile to change to, and which one?

    void loadBG(SDL_Surface *,int,unsigned char); // Surface to copy from, starting tile
    void loadSPR(SDL_Surface *,int,unsigned char); // Surface to copy from, starting tile

    bool loadNSF(char * filename) { if ( m_famiSound != NULL ) return m_famiSound->playNSF(filename); return false;}
    void muteNSF(){ if ( m_famiSound != NULL ) m_famiSound->muteNSF(); }
    void stopNSF(){ if ( m_famiSound != NULL ) m_famiSound->stopNSF(); }
    int loadWAV(char * filename) { if ( m_famiSound != NULL ) return m_famiSound->loadWav(filename); return -1;}
    void playWAV(int wav) { if ( m_famiSound != NULL ) m_famiSound->playWav(wav); }
    void pauseWAV(int wav) { if ( m_famiSound != NULL ) m_famiSound->pauseWav(wav); }
    void stopWAV(int wav) { if ( m_famiSound != NULL ) m_famiSound->stopWav(wav); }
    void unloadWav(int wav) { if ( m_famiSound != NULL ) m_famiSound->unloadWav(wav); }
    bool isWavPlaying(int wav){ if ( m_famiSound != NULL ) return m_famiSound->isPlaying(wav); return false; }

    bool setOAM(unsigned char, unsigned char, unsigned char, unsigned char, bool, bool, bool);
    bool setPositionOAM(unsigned char, unsigned char, unsigned char);
    bool setTileOAM(unsigned char, unsigned char);

    void setScrollX(int scrX);
    void setScrollY(int scrY);
    void setScrollXNT(int scrX);
    void setScrollYNT(int scrY);

    unsigned char getJoy1(){ return m_famiCore->registers()->joy1; }
    unsigned char getJoy2(){ return m_famiCore->registers()->joy2; }

    void setJoy1Num(int);
    void setJoy2Num(int);

    void setJoy1Button(int, int);
    void setJoy2Button(int, int);

    void setKey1Button(int, int);
    void setKey2Button(int, int);

    // Play Sound Engine, FamiSound handles itself via OpenAL

    SDL_Window * m_sdlScreen;
    SDL_Surface * m_sdlSurface;

private:
    int initGL();

    void updateRender();
    int  updateInput();
    void updateSound();
    void waitTimer();

    FamiCore * m_famiCore;      // Core NES types
    FamiSound * m_famiSound;    //
    unsigned int m_famiWidth;
    unsigned int m_famiHeight;
    unsigned int m_renderHeight; // NTSC = 224, PAL = 240
    int m_screenWidth;
    int m_screenHeight;
    int gLastFrame;
    int gLastTick;
    int m_startTime;

    // SDL goodies
    int m_joy1Num;
    SDL_Joystick * m_joy1;
    int m_joy2Num;
    SDL_Joystick * m_joy2;

    int m_joy1Buttons[4]; // A, B, Select, Start
    int m_joy2Buttons[4];

    int m_key1Buttons[8]; // A, B, Select, Start, Up, Down, Left, Right
    int m_key2Buttons[8];

    // OpenGL goodies
    GLuint m_bgTex;         // Background texture

    SDL_Surface * m_nametable1Surf;
    SDL_Surface * m_nametable2Surf;

    GLuint m_nametable1Tex;
    GLuint m_nametable2Tex;
    GLuint m_sprTex;        // Sprites texture
};
