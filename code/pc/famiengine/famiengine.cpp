#include "famiengine.h"

#include <queue>

// Keep this here?
bool fullscreen;

#define FAMICOM_FPS 60 // 60 fps to match original NES

//#define TEXTURE_RATIO .03125f
//#define TEXTURE_RATIO .125f
#define TEXTURE_RATIO (float(TILE_SIZE) / float(VRAM_SIZE))

// 8 x 8 tiles, (0xFF tiles)
#define VID_NTSC 224
#define VID_PAL 240

FamiEngine::FamiEngine()
{
    m_famiWidth = 256;      // both NTSC and PAL respect this
    m_famiHeight = 240;
    m_renderHeight = VID_NTSC;
    m_screenWidth = 512;    // default for now
    m_screenHeight = 480;

    // Aspect used for converting from 256x240 to our screen
    m_screenWidthAspect = m_screenWidth / m_famiWidth;
    m_screenHeightAspect = m_screenHeight / m_renderHeight;

    m_famiCore = NULL;      // nothing yet
    m_famiSound = NULL;
}

FamiEngine::~FamiEngine()
{
    // delete everything
    if ( m_famiCore != NULL )
        delete m_famiCore;

    if ( m_famiSound != NULL )
    {
        m_famiSound->shutdownSound();
        delete m_famiSound;
    }

    if ( m_joy1 )
    {
        SDL_JoystickClose(m_joy1);
    }
    if ( m_joy2 )
    {
        SDL_JoystickClose(m_joy2);
    }

    SDL_DestroyRenderer( m_sdlRenderer );
    SDL_DestroyWindow(m_sdlScreen);

    // Shutdown
    IMG_Quit();
    SDL_Quit(); // Tell SDL to quit
}

void FamiEngine::setBg(unsigned int newColor)
{
    m_famiCore->registers()->bgColor = newColor;
    SDL_SetRenderDrawColor( m_sdlRenderer, (newColor&0xFF0000)>>16, (newColor&0x00FF00)>>8, (newColor&0x0000FF), 0xFF );
}

bool FamiEngine::init(char * title)
{
   /* Initialize SDL */
    if ( SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_AUDIO) < 0 ) {
        return false;
    }

    // Init our sound engine
    m_famiSound = new FamiSound();
    m_famiSound->initSound();

    Uint32 flags;
    flags = SDL_WINDOW_SHOWN;

    SDL_SetHint( SDL_HINT_RENDER_SCALE_QUALITY, "1" );  //Set texture filtering to linear
    m_sdlScreen = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, m_screenWidth, m_screenHeight, flags);
    m_sdlRenderer = SDL_CreateRenderer(m_sdlScreen, -1, SDL_RENDERER_ACCELERATED);

    // Default background to white
    SDL_SetRenderDrawColor( m_sdlRenderer, 0xFF, 0xFF, 0xFF, 0xFF );

    SDL_ShowCursor(false); // hide our cursor

    //Initialize PNG loading
    int imgFlags = IMG_INIT_PNG;
    IMG_Init( imgFlags );

    // Init our engine
    m_famiCore = new FamiCore();

    // Default the background
    m_famiCore->registers()->bgColor = 0x000000; // black

    // Zero out the joystick input
    m_famiCore->registers()->joy1 = m_famiCore->registers()->joy2 = 0;

    // Init the joysticks, these can be changed later by the player
    int numJoys = SDL_NumJoysticks();
    if (numJoys > 0 )   // Auto-Assign player 1 to joystick 1
    {
        m_joy1Num = 0;
        m_joy1 = SDL_JoystickOpen(m_joy1Num);
        if ( numJoys > 1 )  // Auto-Assign player 2 to joystick 2
        {
            m_joy2Num = 1;
            m_joy2 = SDL_JoystickOpen(m_joy2Num);
        }
        else
        {
            m_joy2 = NULL;
        }
    }
    else
    {
        m_joy1 = m_joy2 = NULL;
    }

    // Default for an Xbox 360 controller
    m_joy1Buttons[K_A] = m_joy2Buttons[K_A] = 0;
    m_joy1Buttons[K_B] = m_joy2Buttons[K_B] = 2;
    m_joy1Buttons[K_SELECT] = m_joy2Buttons[K_SELECT] = 6;
    m_joy1Buttons[K_START] = m_joy2Buttons[K_START] = 7;

    memset(m_key2Buttons, 0, sizeof(int)*8);    // joystick 2 has no default for now?

    // Default
    m_key1Buttons[K_A] = SDLK_x;
    m_key1Buttons[K_B] = SDLK_z;
    m_key1Buttons[K_SELECT] = SDLK_a;
    m_key1Buttons[K_START] = SDLK_s;
    m_key1Buttons[K_UP] = SDLK_UP;
    m_key1Buttons[K_DOWN] = SDLK_DOWN;
    m_key1Buttons[K_LEFT] = SDLK_LEFT;
    m_key1Buttons[K_RIGHT] = SDLK_RIGHT;

    // init some timing stuff
    m_startTime = SDL_GetTicks();

    return true;
}

// Note: Be Smart, don't open a non-existent here.
void FamiEngine::setJoy1Num(int j)
{
    if ( m_joy1 )
    {
        SDL_JoystickClose(m_joy1);
    }
    m_joy1Num = j;
    m_joy1 = SDL_JoystickOpen(m_joy1Num);

}

void FamiEngine::setJoy2Num(int j)
{
    if ( m_joy2 )
    {
        SDL_JoystickClose(m_joy2);
    }
    m_joy2Num = j;
    m_joy2 = SDL_JoystickOpen(m_joy2Num);
}

void FamiEngine::setJoy1Button(int joy_button, int newButton)
{
    if ( joy_button < 4 )
    {
        m_joy1Buttons[joy_button] = newButton;
    }
}

void FamiEngine::setJoy2Button(int joy_button, int newButton)
{
    if ( joy_button < 4 )
    {
        m_joy2Buttons[joy_button] = newButton;
    }
}

void FamiEngine::setKey1Button(int key_button, int newKey)
{
    if ( key_button < 8 )
    {
        m_key1Buttons[key_button] = newKey;
    }
}

void FamiEngine::setKey2Button(int key_button, int newKey)
{
    if ( key_button < 8 )
    {
        m_key2Buttons[key_button] = newKey;
    }
}

void FamiEngine::setVideo(bool videoMode)
{
    if ( videoMode == VIDEO_NTSC) // hide the top & bottom tiles in US
    {
        m_renderHeight = VID_NTSC;
    }
    else if ( videoMode == VIDEO_PAL) // Show everything in PAL
    {
        m_renderHeight = VID_PAL;
    }
    // Set our render to stretch this given mode
}

void FamiEngine::setBGData(int idx, unsigned int tile, bool nameTable)
{
    m_famiCore->setBGTile(idx, tile, nameTable);
}

// Surface to copy from, how many tiles we want to pull, tile index to copy into
void FamiEngine::loadSPR(const char * path)
{
    SDL_Surface * surf = IMG_Load(path);
    if ( surf == NULL )
    {
        printf("Could not load sprite surface %s\n", path);
        return;
    }
    m_famiCore->loadSPRVRAM(m_sdlRenderer, surf);
    SDL_FreeSurface(surf);
}

// Surface to copy from, how many tiles we want to pull, tile index to copy into
void FamiEngine::loadBG(const char * path)
{
    char buf[MAX_PATH];
    getcwd(buf, MAX_PATH);
    strcat(buf, path);
    SDL_Surface * surf = IMG_Load(buf);
    if ( surf == NULL )
    {
        printf("Could not load background surface %s\n", buf);
        return;
    }
    m_famiCore->loadBGVRAM(m_sdlRenderer, surf);
    SDL_FreeSurface(surf);
}


bool FamiEngine::setOAM(unsigned char sprIdx, unsigned char tile, unsigned char x, unsigned char y, bool behind, bool flipx, bool flipy)
{
    if ( sprIdx < OAM_CNT)
    {
        SpriteOAM * spr = m_famiCore->OAM(sprIdx);
        spr->tile = tile;
        spr->x = x;
        spr->y = y;
        spr->behind = behind;
        spr->flipX = flipx;
        spr->flipY = flipy;
        return true;
    }
    return false;
}

bool FamiEngine::setPositionOAM(unsigned char sprIdx, unsigned char x, unsigned char y)
{
    if ( sprIdx < OAM_CNT)
    {
        SpriteOAM * spr = m_famiCore->OAM(sprIdx);
        spr->x = x;
        spr->y = y;
        return true;
    }
    return false;
}

bool FamiEngine::setTileOAM(unsigned char sprIdx, unsigned char tile)
{
    if ( sprIdx < OAM_CNT)
    {
        SpriteOAM * spr = m_famiCore->OAM(sprIdx);
        spr->tile = tile;
        return true;
    }
    return false;
}

// Run our update loop
int FamiEngine::update()
{
    // Play Sound Engine, FamiSound handles itself via OpenAL
    updateSound();

    // Render our APU
    updateRender();

    // Update joystick/key input, check for SDL_Quit window close
    if ( updateInput() == FAMI_EXIT )
    {
        return FAMI_EXIT;
    }

    // Wait for NES like timing (30 fps)
    //waitTimer();

    return FAMI_IDLE;
}

void FamiEngine::updateSound()
{
    m_famiSound->update();
}

void FamiEngine::updateRender()
{
    //Clear screen
    SDL_RenderClear( m_sdlRenderer );

    //SDL_RenderCopy(m_sdlRenderer, m_famiCore->m_bgVRAM, NULL, NULL);

    SpriteOAM * spr;
    SDL_Rect src, dst;
    src.w = TILE_SIZE;
    src.h = TILE_SIZE;
    dst.w = TILE_SIZE * m_screenWidthAspect;
    dst.h = TILE_SIZE * m_screenHeightAspect;
    SDL_Texture * sprTex = m_famiCore->m_sprVRAM;
    std::queue<SpriteOAM*> secondDraw;
    for ( int i = 0; i < OAM_CNT; i++)
    {
        spr = m_famiCore->OAM(i);
        if ( spr->behind == true )
        {
            src.x = (i % TILE_WIDTH)*TILE_SIZE;
            src.y = (i / TILE_WIDTH)*TILE_SIZE;
            dst.x = spr->x * m_screenWidthAspect;
            dst.y = spr->y * m_screenHeightAspect;
            /*
            if ( spr->flipX == true && spr->flipY == true )
            {
            }
            else if ( spr->flipX == true )
            {
            }
            else if ( spr->flipY == true )
            {
            }
            */
            SDL_RenderCopy(m_sdlRenderer, sprTex, &src, &dst);
        }
        else
        {
            secondDraw.push(spr);
        }
    }

    // Get Scroll Registers
    FamiRegisters * fRegister = m_famiCore->registers();

    int xOffset, yOffset, l, t, r, b;
    l = t = r = b = 0;
    // Left, Top, Right, Bottom
    xOffset = -(fRegister->scrollX + (fRegister->scrollX_NT*8));// + ((fRegister->scrollXFlip*m_famiWidth));
    yOffset = -(fRegister->scrollY + (fRegister->scrollY_NT*8));// + ((fRegister->scrollYFlip*m_famiHeight));

    if ( m_renderHeight == VID_NTSC )
    {
        yOffset -= 8; // move another 8 tiles
    }

    if ( fRegister->scrollXFlip == true )
        // move regular x's to the right side
        l = m_famiWidth;
    else
        r = m_famiWidth;
    if ( fRegister->scrollYFlip == true )
        // move regular x's to the right side
        t = m_famiHeight;
    else
        b = m_famiHeight;

    // NAME TABLE 1
    unsigned int * nt1 = m_famiCore->getNameTable(0);
    unsigned int * nt2 = m_famiCore->getNameTable(1);
    for(int i = 0; i < 0x3C0; i++)
    {
        // NAME TABLE 1
        src.x = (nt1[i] % TILE_WIDTH) * TILE_SIZE;
        src.y = (nt1[i] / TILE_WIDTH) * TILE_SIZE;
        dst.x = (i % TILE_WIDTH) * TILE_SIZE * m_screenWidthAspect;
        dst.y = (i / TILE_WIDTH) * TILE_SIZE * m_screenHeightAspect;
        SDL_RenderCopy(m_sdlRenderer, m_famiCore->m_bgVRAM, &src, &dst);

        // NAME TABLE 3
        src.x = (nt1[i] % TILE_WIDTH) * TILE_SIZE;
        src.y = (nt1[i] / TILE_WIDTH) * TILE_SIZE;
        dst.x = (i % TILE_WIDTH) * TILE_SIZE * m_screenWidthAspect;
        dst.y = ((i / TILE_WIDTH) * TILE_SIZE + m_famiHeight) * m_screenHeightAspect;
        SDL_RenderCopy(m_sdlRenderer, m_famiCore->m_bgVRAM, &src, &dst);

        // NAME TABLE 2
        src.x = (nt2[i] % TILE_WIDTH) * TILE_SIZE;
        src.y = (nt2[i] / TILE_WIDTH) * TILE_SIZE;
        dst.x = ((i % TILE_WIDTH) * TILE_SIZE + m_famiWidth) * m_screenWidthAspect;
        dst.y = (i / TILE_WIDTH) * TILE_SIZE * m_screenHeightAspect;
        SDL_RenderCopy(m_sdlRenderer, m_famiCore->m_bgVRAM, &src, &dst);

        // NAME TABLE 4 - [Mirror of NAME TABLE 2]
        src.x = (nt2[i] % TILE_WIDTH) * TILE_SIZE;
        src.y = (nt1[i] / TILE_WIDTH) * TILE_SIZE;
        dst.x = ((i % TILE_WIDTH) * TILE_SIZE + m_famiWidth) * m_screenWidthAspect;
        dst.y = ((i / TILE_WIDTH) * TILE_SIZE + m_famiHeight) * m_screenHeightAspect;
        SDL_RenderCopy(m_sdlRenderer, m_famiCore->m_bgVRAM, &src, &dst);

    }
    // Now draw the front
    while ( !secondDraw.empty() )
    {
        spr = secondDraw.front();
        secondDraw.pop();
        src.x = (spr->tile % TILE_WIDTH)*TILE_SIZE;
        src.y = (spr->tile / TILE_WIDTH)*TILE_SIZE;
        dst.x = spr->x * m_screenWidthAspect;
        dst.y = spr->y * m_screenHeightAspect;
        /*
        if ( spr->flipX == true )
        {
            src.x = (i % TILE_WIDTH)*TILE_SIZE;
            src.y = (i / TILE_WIDTH)*TILE_SIZE;
            dst.x = spr->x * m_screenWidthAspect;
            dst.y = spr->y * m_screenHeightAspect;
        }
        else
        {
            src.x = (i % TILE_WIDTH)*TILE_SIZE;
            src.y = (i / TILE_WIDTH)*TILE_SIZE;
            dst->x = spr->x * m_screenWidthAspect;
            dst->y = spr->y * m_screenHeightAspect;
        }
        if ( spr->flipY == true )
        {
            src->x = (i % TILE_WIDTH)*TILE_SIZE;
            src->y = (i / TILE_WIDTH)*TILE_SIZE;
            dst->x = spr->x * m_screenWidthAspect;
            dst->y = spr->y * m_screenHeightAspect;
        }
        else
        {
            src->x = (i % TILE_WIDTH)*TILE_SIZE;
            src->y = (i / TILE_WIDTH)*TILE_SIZE;
            dst->x = spr->x * m_screenWidthAspect;
            dst->y = spr->y * m_screenHeightAspect;
        }
        */
        SDL_RenderCopy(m_sdlRenderer, sprTex, &src, &dst);
    }

    // lets see the BG image load
//    SDL_RenderCopy(m_sdlRenderer, m_famiCore->m_bgVRAM, NULL, NULL);

    //Update screen
    SDL_RenderPresent( m_sdlRenderer );
}

int FamiEngine::updateInput()
{
    FamiRegisters * fRegister = m_famiCore->registers();

    // Get the joystick/key input and set our joysticks
    SDL_Event event;
    while ( SDL_PollEvent(&event) ) {
        switch (event.type) {
        case SDL_QUIT:
            return FAMI_EXIT;			// If So done=TRUE
        default:
            break;
        }

        if (event.type == SDL_KEYDOWN)
        {
            int keyDown = event.key.keysym.sym;
            if ( keyDown == m_key1Buttons[K_LEFT])
                fRegister->joy1 |= J_LEFT;
            else if ( keyDown == m_key1Buttons[K_RIGHT])
                fRegister->joy1 |= J_RIGHT;
            else if ( keyDown == m_key1Buttons[K_UP])
                fRegister->joy1 |= J_UP;
            else if ( keyDown == m_key1Buttons[K_DOWN])
                fRegister->joy1 |= J_DOWN;
            else if ( keyDown == m_key1Buttons[K_B])
                fRegister->joy1 |= J_B;
            else if ( keyDown == m_key1Buttons[K_A])
                fRegister->joy1 |= J_A;
            else if ( keyDown == m_key1Buttons[K_SELECT])
                fRegister->joy1 |= J_SELECT;
            else if ( keyDown == m_key1Buttons[K_START])
                fRegister->joy1 |= J_START;
            else if ( keyDown == m_key2Buttons[K_LEFT])
                fRegister->joy2 |= J_LEFT;
            else if ( keyDown == m_key2Buttons[K_RIGHT])
                fRegister->joy2 |= J_RIGHT;
            else if ( keyDown == m_key2Buttons[K_UP])
                fRegister->joy2 |= J_UP;
            else if ( keyDown == m_key2Buttons[K_DOWN])
                fRegister->joy2 |= J_DOWN;
            else if ( keyDown == m_key2Buttons[K_B])
                fRegister->joy2 |= J_B;
            else if ( keyDown == m_key2Buttons[K_A])
                fRegister->joy2 |= J_A;
            else if ( keyDown == m_key2Buttons[K_SELECT])
                fRegister->joy2 |= J_SELECT;
            else if ( keyDown == m_key2Buttons[K_START])
                fRegister->joy2 |= J_START;
            else if ( keyDown == SDLK_ESCAPE)
                return FAMI_EXIT;
        }
        else if ( event.type == SDL_KEYUP)
        {
            int keyUp = event.key.keysym.sym;
            if ( keyUp == m_key1Buttons[K_LEFT])
                fRegister->joy1 &= ~J_LEFT;
            else if ( keyUp == m_key1Buttons[K_RIGHT])
                fRegister->joy1 &= ~J_RIGHT;
            else if ( keyUp == m_key1Buttons[K_UP])
                fRegister->joy1 &= ~J_UP;
            else if ( keyUp == m_key1Buttons[K_DOWN])
                fRegister->joy1 &= ~J_DOWN;
            else if ( keyUp == m_key1Buttons[K_B])
                fRegister->joy1 &= ~J_B;
            else if ( keyUp == m_key1Buttons[K_A])
                fRegister->joy1 &= ~J_A;
            else if ( keyUp == m_key1Buttons[K_SELECT])
                fRegister->joy1 &= ~J_SELECT;
            else if ( keyUp == m_key1Buttons[K_START])
                fRegister->joy1 &= ~J_START;
            else if ( keyUp == m_key2Buttons[K_LEFT])
                fRegister->joy2 &= ~J_LEFT;
            else if ( keyUp == m_key2Buttons[K_RIGHT])
                fRegister->joy2 &= ~J_RIGHT;
            else if ( keyUp == m_key2Buttons[K_UP])
                fRegister->joy2 &= ~J_UP;
            else if ( keyUp == m_key2Buttons[K_DOWN])
                fRegister->joy2 &= ~J_DOWN;
            else if ( keyUp == m_key2Buttons[K_B])
                fRegister->joy2 &= ~J_B;
            else if ( keyUp == m_key2Buttons[K_A])
                fRegister->joy2 &= ~J_A;
            else if ( keyUp == m_key2Buttons[K_SELECT])
                fRegister->joy2 &= ~J_SELECT;
            else if ( keyUp == m_key2Buttons[K_START])
                fRegister->joy2 &= ~J_START;
        }
        else if ( event.type == SDL_JOYHATMOTION )
        {
            unsigned char * joy = NULL;
            if ( event.jhat.which == m_joy1Num )
            {
                joy = &fRegister->joy1;
            }
            else if ( event.jhat.which == m_joy2Num )
            {
                joy = &fRegister->joy2;
            }
            if ( joy != NULL )
            {
                if ( event.jhat.value == SDL_HAT_CENTERED )
                {
                    *joy &= ~J_UP;
                    *joy &= ~J_DOWN;
                    *joy &= ~J_LEFT;
                    *joy &= ~J_RIGHT;
                }
                else
                {
                    if ( event.jhat.value & SDL_HAT_UP )
                    {
                        *joy |= J_UP;
                        *joy &= ~J_DOWN;
                    }
                    else if ( event.jhat.value & SDL_HAT_DOWN )
                    {
                        *joy |= J_DOWN;
                        *joy &= ~J_UP;
                    }
                    else
                    {
                        *joy &= ~J_DOWN;
                        *joy &= ~J_UP;
                    }
                    if ( event.jhat.value & SDL_HAT_LEFT )
                    {
                        *joy |= J_LEFT;
                        *joy &= ~J_RIGHT;
                    }
                    else if ( event.jhat.value & SDL_HAT_RIGHT )
                    {
                        *joy |= J_RIGHT;
                        *joy &= ~J_LEFT;
                    }
                    else
                    {
                        *joy &= ~J_LEFT;
                        *joy &= ~J_RIGHT;
                    }
                }
            }
        }
        else if ( event.type == SDL_JOYAXISMOTION )
        {
            // do something
            if ( event.jaxis.which == m_joy1Num )
            {
            }
            else if ( event.jaxis.which == m_joy2Num )
            {
            }
        }
        else if ( event.type == SDL_JOYBUTTONDOWN )
        {
            if ( event.jbutton.which == m_joy1Num )
            {
                unsigned char butDown = event.jbutton.button;
                if ( butDown  == m_joy1Buttons[K_B])
                    fRegister->joy1 |= J_B;
                else if ( butDown  == m_joy1Buttons[K_A])
                    fRegister->joy1 |= J_A;
                else if ( butDown  == m_joy1Buttons[K_SELECT])
                    fRegister->joy1 |= J_SELECT;
                else if ( butDown  == m_joy1Buttons[K_START])
                    fRegister->joy1 |= J_START;
            }
            else if ( event.jbutton.which == m_joy2Num )
            {
                unsigned char butDown = event.jbutton.button;
                if ( butDown  == m_joy2Buttons[K_B])
                    fRegister->joy2 |= J_B;
                else if ( butDown  == m_joy2Buttons[K_A])
                    fRegister->joy2 |= J_A;
                else if ( butDown  == m_joy2Buttons[K_SELECT])
                    fRegister->joy2 |= J_SELECT;
                else if ( butDown  == m_joy2Buttons[K_START])
                    fRegister->joy2 |= J_START;
            }
        }
        else if ( event.type == SDL_JOYBUTTONUP )
        {
            if ( event.jbutton.which == m_joy1Num )
            {
                unsigned char butUp = event.jbutton.button;
                if ( butUp  == m_joy1Buttons[K_B])
                    fRegister->joy1 &= ~J_B;
                else if ( butUp  == m_joy1Buttons[K_A])
                    fRegister->joy1 &= ~J_A;
                else if ( butUp  == m_joy1Buttons[K_SELECT])
                    fRegister->joy1 &= ~J_SELECT;
                else if ( butUp  == m_joy1Buttons[K_START])
                    fRegister->joy1 &= ~J_START;
            }
            else if ( event.jbutton.which == m_joy2Num )
            {
                unsigned char butUp = event.jbutton.button;
                if ( butUp  == m_joy2Buttons[K_B])
                    fRegister->joy2 &= ~J_B;
                else if ( butUp  == m_joy2Buttons[K_A])
                    fRegister->joy2 &= ~J_A;
                else if ( butUp  == m_joy2Buttons[K_SELECT])
                    fRegister->joy2 &= ~J_SELECT;
                else if ( butUp  == m_joy2Buttons[K_START])
                    fRegister->joy2 &= ~J_START;
            }
        }
    }
    return FAMI_IDLE;
}

void FamiEngine::setScrollX(int scrX)
{
    if ( scrX > 7 )
        scrX = 7;
    m_famiCore->registers()->scrollX = scrX;
}

void FamiEngine::setScrollY(int scrY)
{
    if ( scrY > 7 )
        scrY = 7;
    m_famiCore->registers()->scrollY = scrY;
}

void FamiEngine::setScrollXNT(int scrX)
{
    if ( scrX > 32 )
        scrX = 32;

    if ( scrX == 32 ) // wrap the X value
    {
        scrX = 0;
        m_famiCore->registers()->scrollXFlip = !m_famiCore->registers()->scrollXFlip;
    }
    m_famiCore->registers()->scrollX_NT = scrX;
}

void FamiEngine::setScrollYNT(int scrY)
{
    if ( scrY > 30 )
        scrY = 30;

    if ( scrY == 30 ) // wrap the Y value
    {
        scrY = 0;
        m_famiCore->registers()->scrollYFlip = !m_famiCore->registers()->scrollYFlip;
    }
    m_famiCore->registers()->scrollY_NT = scrY;
}

void FamiEngine::waitForVsync()
{
    if((SDL_GetTicks() - m_startTime) < 1000.0/FAMICOM_FPS)
    {
        SDL_Delay(int(1000.0/FAMICOM_FPS - (SDL_GetTicks() - m_startTime)));
    }

    m_startTime = SDL_GetTicks();
}
