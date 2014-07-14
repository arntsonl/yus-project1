#pragma once

#define NUMBUFFERS (4)

#include "AL/al.h"
#include "AL/alc.h"
#include "CWaves.h"

#include <vector>

// Gives us the ability to load a WAV or an NSF and play it back
#define MAX_WAVS 128

struct soundData
{
    unsigned char * data;
    unsigned int dpos;
    bool playSound;

    WAVEID WaveID;
    long unsigned int ulDataSize;
    long unsigned int ulFrequency;
    long unsigned int ulFormat;
    WAVEFORMATEX wfex;
};

#define BUFFER_SIZE   32768     // 32 KB buffers

class FamiSound
{
public:
    FamiSound();
    ~FamiSound();
    bool initSound();
    void shutdownSound();

    // Plays NSF through the Festalon NSF player
    bool playNSF(char*);// Load a new NSF, delete the old if it exists
    void muteNSF();    // Pause the NSF
    void stopNSF();     // Completely Unload the NSF

    // Load wav data into a buffer
    int loadWav(char*);
    void unloadWav(int);
    void playWav(int);
    void pauseWav(int);
    void stopWav(int);
    bool isPlaying(int i){ if ( i < MAX_WAVS && m_wavData[i].data != 0 ) { return m_wavData[i].playSound;} return false; }

    // Updates all the sources, playback, etc. This replaces the callback
    void update();

private:
    //SDL_AudioSpec m_audioSpec;
    bool m_playingMusic;
    bool m_pausedMusic;
    int m_sampleRate;
    int m_bufferSize;
    int m_playingAudio;

    // Four buffers for various sound bits (thanks Ivan!)
    unsigned int * m_tmpBuffer;
    unsigned char * m_soundBuffer;
    unsigned char * m_musicBuffer;
    unsigned char * m_finalBuffer;

    int m_bufferAvailable;
    unsigned char * m_musicBufferStart;

    int m_holdoverSize;

    void updateMusic();
    void updateSoundSystem(int);
    void updateSounds(int);

    //SDL_mutex * m_lockMutex;

    soundData m_wavData[MAX_WAVS];

    int m_totalNSFBuffer;

    ALCcontext *pContext;
    ALCdevice *pDevice;
    ALint iLoop;
    ALuint bufferID[NUMBUFFERS];
    ALuint sourceID;
    ALint iBuffersProcessed;
    ALint iBuffersQueued;

    int m_shutdownStarted;
};
