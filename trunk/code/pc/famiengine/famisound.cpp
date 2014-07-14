#include "famisound.h"
#include ".\festalon\driver.h" // NSF player

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static FamiSound * famiPtr; // used for callback
//static SDL_mutex * lockPtr;

// CWave - Interface used by OpenAL examples
CWaves *		pWaveLoader = NULL;

FamiSound::FamiSound()
{
    // Init our variables maybe?
    m_playingMusic = false; // Is the NSF loaded & playing
    m_pausedMusic = false;  // Is the NSF loaded & paused?
    m_sampleRate = 44100;
    m_bufferSize = 4096;
    m_playingAudio = 1;
    famiPtr = this; // help
    m_shutdownStarted = 0;
}

FamiSound::~FamiSound()
{
}

bool FamiSound::initSound()
{
    // Create our sound buffers
    m_tmpBuffer = new unsigned int[m_bufferSize];
    m_soundBuffer = new unsigned char[m_bufferSize];
    m_musicBuffer = new unsigned char[m_bufferSize];
    m_finalBuffer = new unsigned char[m_bufferSize];

    memset(m_finalBuffer, 0, m_bufferSize);

    memset(m_wavData, 0, sizeof(soundData)*MAX_WAVS); // NULL all pointers

    m_bufferAvailable = m_bufferSize;
    m_musicBufferStart = m_musicBuffer;

    m_holdoverSize = 0;

	pContext = NULL;
	pDevice = NULL;
	ALint numDevices;
	ALint numDefaultDevice;
    char *defaultDevice=NULL;
    char *deviceList=NULL;
    char *devices[16];

    if (alcIsExtensionPresent(NULL, (ALCchar*)"ALC_ENUMERATION_EXT") == AL_TRUE)
    { // try out enumeration extension
        deviceList = (char *)alcGetString(NULL, ALC_DEVICE_SPECIFIER);
        numDevices = 0;
        if (strlen(deviceList))
        {
            numDevices = 1;
            defaultDevice = (char *)alcGetString(NULL, ALC_DEFAULT_DEVICE_SPECIFIER);
            numDefaultDevice = 0;

            for (unsigned short numD = 0; numD < 16; numD++)
            {
                fprintf(stderr, "Driver List: Sound Driver %s\n", deviceList);
                devices[numD] = deviceList;
                if (defaultDevice && strcmp(devices[numD], defaultDevice) == 0)
                {
                    numDefaultDevice = numD;
                }
                deviceList += strlen(deviceList);
                if (deviceList[0] == 0){
                    if (deviceList[1] == 0){
                        break;
                    }
                    else{
                        numDevices++;
                        deviceList++;
                    }
                }
            }
        }
    }
    else
    {
        fprintf(stderr, "*ERROR* Could not find ALC Enumeration, leaving sound init!");
        return false; // probably crashes here..
    }

    // Open the default device we just found..
    pDevice = alcOpenDevice(defaultDevice);

    // Create context
    pContext = alcCreateContext(pDevice, NULL);

    // Set active context
    alcMakeContextCurrent(pContext);

    // Clear Error Code
    alGetError();

    // Create sound buffer and source
    alGenBuffers(NUMBUFFERS, bufferID);
    alGenSources(1, &sourceID);

    // Set the source and listener to the same location
    alListener3f(AL_POSITION, 0.0f, 0.0f, 0.0f);
    alSource3f(sourceID, AL_POSITION, 0.0f, 0.0f, 0.0f);

    // Start playing source with 4 empty buffers
    /*for(iLoop = 0; iLoop < NUMBUFFERS; iLoop++)
    {
        alBufferData(bufferID[iLoop], AL_FORMAT_STEREO16, m_finalBuffer, m_bufferSize, m_sampleRate);
        alSourceQueueBuffers(sourceID, 1, &bufferID[iLoop]);
    }*/
    alBufferData(bufferID[0], AL_FORMAT_STEREO16, m_finalBuffer, m_bufferSize, m_sampleRate);
    alSourceQueueBuffers(sourceID, 1, &bufferID[0]);

    alSourcePlay(sourceID);

    // Create a new Wave handler class
    pWaveLoader = new CWaves();

    // Setup FESTALON NSF player with sample rate
	FESTAI_Sound(m_sampleRate);
	FESTAI_SetSoundQuality(0);
	FESTAI_SetVolume(100);

    return true;
}

void FamiSound::shutdownSound()
{
    m_shutdownStarted = 1; // signals our other mutex to die

    if ( m_playingMusic == true )
    {
        FESTAI_Close();
    }

    alSourceStop(sourceID);
    alDeleteSources(1, &sourceID);
    alDeleteBuffers(NUMBUFFERS, bufferID);

    //Get active context
    pContext=alcGetCurrentContext();

    //Get device for active context
    pDevice=alcGetContextsDevice(pContext);

    //Disable context
    alcMakeContextCurrent(NULL);

    //Release context(s)
    alcDestroyContext(pContext);

    //Close device
    alcCloseDevice(pDevice);

    // Clean Memory
    delete m_tmpBuffer;
    delete m_soundBuffer;
    delete m_musicBuffer;
    delete m_finalBuffer;

    // Unload all of our waves
    for(int i = 0; i < MAX_WAVS; i++)
        unloadWav(i);

    // Delete the wave handler class
    if(pWaveLoader)
        delete pWaveLoader;
}

bool FamiSound::playNSF(char* filename)
{
    fprintf(stderr, "Loading NSF: %s\n", filename);

	if ( m_playingMusic == false )
	{
		char * data = NULL;
		FILE *inPtr = fopen(filename, "rb");
		if(!inPtr)
		{
			return false;
		}

		fseek(inPtr, 0, SEEK_END);
		long dataSize = ftell(inPtr);
		fseek(inPtr, 0, SEEK_SET);
		data = new char[dataSize];
		fread(data, 1, dataSize, inPtr);
		fclose(inPtr);

		if(data != NULL) {
			FESTAI_Load((uint8*)data,dataSize);
			FESTAI_NSFControl(0,0);
			delete data;
			m_playingMusic = 1;
		}
	}
	else if ( m_playingMusic == true )
	{
	    // Unload
	    FESTAI_Close(); // free's memory
        char * data = NULL;
		FILE *inPtr = fopen(filename, "rb");
		if(!inPtr)
		{
			return false;
		}

		fseek(inPtr, 0, SEEK_END);
		long dataSize = ftell(inPtr);
		fseek(inPtr, 0, SEEK_SET);
		data = new char[dataSize];
		fread(data, 1, dataSize, inPtr);
		fclose(inPtr);

		if(data != NULL) {
			FESTAI_Load((uint8*)data,dataSize);
			FESTAI_NSFControl(0,0);
			delete data;
			m_pausedMusic = false; // unpause it if it is paused
		}
	}
	fprintf(stderr, "NSF will auto-play\n");

	return true;
}

// Flip on and off
void FamiSound::muteNSF()
{
    if ( m_pausedMusic == true )
        FESTAI_SetVolume(100);
    else if ( m_pausedMusic == false )
        FESTAI_SetVolume(0);
    m_pausedMusic = !m_pausedMusic;
}

void FamiSound::stopNSF()
{
    FESTAI_Close();
    m_pausedMusic = false;
    m_playingMusic = false;
}

// Load wav data into a buffer
int FamiSound::loadWav(char* fileName)
{
    fprintf(stderr, "Loading wav: %s\n", fileName);

    for(int i = 0 ; i < MAX_WAVS; i++)
    {
        if ( m_wavData[i].data == NULL && pWaveLoader->OpenWaveFile(fileName, &m_wavData[i].WaveID)==WR_OK)
        {
            pWaveLoader->GetWaveSize(m_wavData[i].WaveID, &m_wavData[i].ulDataSize);
            pWaveLoader->GetWaveFrequency(m_wavData[i].WaveID, &m_wavData[i].ulFrequency);
            pWaveLoader->GetWaveALBufferFormat(m_wavData[i].WaveID, &alGetEnumValue, &m_wavData[i].ulFormat);
            pWaveLoader->GetWaveFormatExHeader(m_wavData[i].WaveID, &m_wavData[i].wfex);
            pWaveLoader->SetWaveDataOffset(m_wavData[i].WaveID, 0);
            m_wavData[i].data = (unsigned char *)malloc(m_wavData[i].ulDataSize);
            long unsigned int bytesWritten;
            if ( pWaveLoader->ReadWaveData(m_wavData[i].WaveID, m_wavData[i].data, m_wavData[i].ulDataSize, &bytesWritten) == WR_OK)
            {
                m_wavData[i].dpos = 0;
                m_wavData[i].playSound = false;
                return i;
            }
            else
            {
                free(m_wavData[i].data);
                m_wavData[i].data = NULL;
                break;
            }
        }
    }
    return -1;
}

// Delete selected wav
void FamiSound::unloadWav(int idx)
{
    if ( idx < MAX_WAVS && m_wavData[idx].data != NULL )
    {
        free(m_wavData[idx].data);
        m_wavData[idx].data = NULL;
    }
}

// Set loaded wav play sound to true
void FamiSound::playWav(int idx)
{
    if ( idx < MAX_WAVS && m_wavData[idx].data != NULL )
    {
        m_wavData[idx].playSound = true;
    }
}

char * GetALErrorString(ALenum err)
{
    switch(err)
    {
        case AL_NO_ERROR:
            return "AL_NO_ERROR";
        break;

        case AL_INVALID_NAME:
            return "AL_INVALID_NAME";
        break;

        case AL_INVALID_ENUM:
            return "AL_INVALID_ENUM";
        break;

        case AL_INVALID_VALUE:
            return "AL_INVALID_VALUE";
        break;

        case AL_INVALID_OPERATION:
            return "AL_INVALID_OPERATION";
        break;

        case AL_OUT_OF_MEMORY:
            return "AL_OUT_OF_MEMORY";
        break;
    };
}

void FamiSound::pauseWav(int idx)
{
    if ( idx < MAX_WAVS && m_wavData[idx].data != NULL && m_wavData[idx].dpos != 0)
    {
        m_wavData[idx].playSound = !m_wavData[idx].playSound;
        //m_wavData[idx].dpos = 0; // don't overwrite the data pointer
    }
}

void FamiSound::stopWav(int idx)
{
    if ( idx < MAX_WAVS && m_wavData[idx].data != NULL )
    {
        m_wavData[idx].playSound = false;
        m_wavData[idx].dpos = 0;
    }
}

void FamiSound::update()
{
    // Update our sound buffer
    if ( m_shutdownStarted == 1)
    {
        return; // get out if we've triggered a shut down
    }

    iBuffersProcessed = 0;
    try {
        // Clear the last error
        alGetError ();

    /* At least some Windows implementations of OpenAL32.dll
    *  require AL_BUFFERS_PROCESSED to be queried prior to
    *  calling alSourceUnqueueBuffers(), or else no buffers
    *  will be unqueued and AL_INVALID_OPERATION will always
    *  be returned.
    */
        alGetSourcei(sourceID, AL_BUFFERS_QUEUED, &iBuffersQueued);

        alGetSourcei(sourceID, AL_BUFFERS_PROCESSED, &iBuffersProcessed);
        // here's the error! ^---

        //When using static buffers a crash may occur if a source is playing with a buffer that is about
			//to be deleted even though we stop the source and successfully delete the buffer. Crash is confirmed
			//on 2.2.1 and 3.1.2, however, it will only occur if a source is used rapidly after having its prior
			//data deleted. To avoid any possibility of the crash we wait for the source to finish playing.

        if ( iBuffersProcessed > 0 )
        {
            if ( m_playingMusic == true)
            {
                while(m_bufferAvailable > 0 && m_shutdownStarted == 0)
                {
                    updateMusic();
                }
            }

            ALuint uiBuffer = 0;
            alSourceUnqueueBuffers(sourceID, 1, &uiBuffer);
            alBufferData(uiBuffer, AL_FORMAT_STEREO16, m_finalBuffer, m_bufferSize, m_sampleRate);

            alSourceQueueBuffers(sourceID, 1, &uiBuffer);

            updateSoundSystem(iBuffersProcessed*m_bufferSize);
            m_bufferAvailable = m_bufferSize; // Reset our buffer available size

            m_musicBuffer = m_musicBufferStart;
            memset(m_musicBuffer, 0, m_bufferSize);
        }

        while( iBuffersProcessed )
        {
            ALuint uiBuffer = 0;
            alSourceUnqueueBuffers(sourceID, 1, &uiBuffer);
            alBufferData(uiBuffer, AL_FORMAT_STEREO16, m_finalBuffer, m_bufferSize, m_sampleRate);
            alSourceQueueBuffers(sourceID, 1, &uiBuffer);
            iBuffersProcessed--;
        }

        ALint iState;
        //ALint iQueuedBuffers;
        alGetSourcei(sourceID, AL_SOURCE_STATE, &iState);
        if(iState != AL_PLAYING)
        {
            //alGetSourcei(sourceID, AL_BUFFERS_QUEUED, &iQueuedBuffers);
            if (iBuffersQueued)
                alSourcePlay(sourceID);
        }
    }
    catch (...)
    {
        ALint error = alGetError();
        fprintf(stderr, "Error[%i]: %s\n", error, GetALErrorString(error));
    }
}

// No mixing!
void FamiSound::updateSoundSystem(int count)
{
    if(m_playingAudio == 0)
    {
		return;
    }

	updateSounds(count);
	memset(m_finalBuffer, 0, m_bufferSize);

    // First set final buffer to music
    if(m_playingMusic == 1)
        memcpy(m_finalBuffer, m_musicBufferStart, m_bufferSize);

    // Very basic WAV mixing going on here..
    for(int i = 0; i < m_bufferSize; i++)
    {
        m_finalBuffer[i] += m_soundBuffer[i];
    }
}

void FamiSound::updateSounds(int count)
{
    int i;

	unsigned int l,n;
	float f;

	memset(m_soundBuffer, 0, m_bufferSize);
	for(i=0;i<MAX_WAVS;i++)
	{
	    if ( m_wavData[i].data != NULL && m_wavData[i].playSound == true)
	    {
	        unsigned int amount = m_wavData[i].ulDataSize - m_wavData[i].dpos;
	        if (amount > count)
	        {
                amount = count;
	        }

            // Basic WAV mixing
            for(int j = 0; j < amount && (m_wavData[i].dpos+j) < m_wavData[i].ulDataSize; j++)
            {
                m_soundBuffer[j] += m_wavData[i].data[m_wavData[i].dpos+j];
            }

            m_wavData[i].dpos += amount;

            if ( m_wavData[i].dpos >= m_wavData[i].ulDataSize )
            {
                // kill it, reset it
                m_wavData[i].playSound = false;
                m_wavData[i].dpos = 0;
            }
	    }
	}
}

void FamiSound::updateMusic()
{
//    SDL_LockMutex(lockPtr);
    if ( m_bufferAvailable <= 0 )
    {
        return;
    }

    int count = 0;
    int copySize;
    int * tmp = NULL;
    short * tmp2 = NULL;

    // Carry over a buffer from the last update period
    if ( m_holdoverSize > 0)
    {
        memcpy(m_musicBuffer, m_tmpBuffer, m_holdoverSize); // Copy what was left into the music buffer
        m_musicBuffer += m_holdoverSize;                    // Increment the music buffer according to holdover
        m_bufferAvailable -= m_holdoverSize;                // Decrement available buffer by hold-over size
        m_holdoverSize = 0;                                 // Set new hold-over to 0
    }

    if ( m_playingAudio == 1 && m_musicBuffer != NULL )
    {
        tmp = FESTAI_Emulate(&count);           // Emulate the NSF/NES core
        int i;                                  // For stereo hack
        tmp2 = (short*)tmp;                     //  "     "
        for(i=1; i < count*2; i+= 2 )           // Create stereo by looping & doubling shorts
            tmp2[i] = tmp2[i-1];                // here..
        copySize = count*4;                     // count is 8-bit, buffer is 32-bit
        if ( copySize > m_bufferAvailable ){    // Copy more data than in the buffer
            copySize = m_bufferAvailable;       // Set copysize to buffer available (for memcpy later)
            m_holdoverSize = (count*4) - copySize;  // Set hold-over as (8-bit*4)-bufferAvailable
            memcpy(m_tmpBuffer, tmp+(copySize/4), m_holdoverSize); // Copy extra to tmpBuffer
        }

        memcpy(m_musicBuffer, tmp, copySize);   // Copy tmp with copy size
        m_musicBuffer += copySize;              // Increment music buffer by copy size
        m_bufferAvailable -= copySize;          // Decrement buffer available by copy size
    }
//    SDL_UnlockMutex(lockPtr);
}
