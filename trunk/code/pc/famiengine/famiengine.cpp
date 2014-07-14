#include "famiengine.h"

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
    m_screenWidth = 640;    // default for now
    m_screenHeight = 480;
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

    glDeleteTextures( 1, &m_sprTex );
    glDeleteTextures( 1, &m_bgTex );
    glDeleteTextures( 1, &m_nametable1Tex );
    glDeleteTextures( 1, &m_nametable2Tex );

    if ( m_joy1 )
    {
        SDL_JoystickClose(m_joy1);
    }
    if ( m_joy2 )
    {
        SDL_JoystickClose(m_joy2);
    }

    // Shutdown
    SDL_Quit(); // Tell SDL to quit
}

bool CreateGLWindow(FamiEngine * fe, char* title, int width, int height, int bits, bool fullscreenflag)
{
    Uint32 flags;
    //int size;

    fullscreen = fullscreenflag;	// Set The Global Fullscreen Flag
    flags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN;
    if ( fullscreenflag ) {
        flags |= SDL_WINDOW_FULLSCREEN;
    }
    //Use OpenGL 2.1
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 2 );
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 1 );
    //SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 8 );
    //SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 8 );
    //SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 8 );
    //SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, bits);
    //SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1);

    //SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE, 1 );
    /*
    if ( SDL_SetVideoMode(width, height, bits, flags) == NULL )
    {
    return false;
    }

    SDL_WM_SetCaption(title, "opengl");
    */
    fe->m_sdlScreen = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, flags);
    fe->m_sdlSurface = SDL_GetWindowSurface(fe->m_sdlScreen);
    SDL_GL_CreateContext(fe->m_sdlScreen);
    SDL_ShowCursor(false); // hide our cursor

    //ReSizeGLScene(width, height);		// Set Up Our Perspective GL Screen
    return true;				// Success
}

void FamiEngine::setBg(unsigned int newColor)
{
    m_famiCore->registers()->bgColor = newColor;
}

int FamiEngine::initGL()			// All Setup For OpenGL Goes Here
{
	glViewport(0, 0,m_screenWidth, m_screenHeight);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0,m_famiWidth, m_renderHeight, 0, 0.0f, 1.0f); // 0,w,h,0 makes it top left,  0,w,0,h makes it bottom left
	//gluOrtho2D(0,240,0,248);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_TEXTURE_2D);
	glColor4f(1.0f, 1.0f, 1.0, 1.0f);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	return true;						// Initialization Went OK
}

static void copy_row(const unsigned char *start, const unsigned char *end, unsigned char *dest)
{
    if(start != end) for(;;)
    {
        // This is a quickie blending technique to blend alphas
        /*
        unsigned char a = start[3];
        unsigned char ia = ~a;
        unsigned char ias = ~dest[3];

        dest[0] = (start[0]*a+dest[0]*ia)>>8;
        dest[1] = (start[1]*a+dest[1]*ia)>>8;
        dest[2] = (start[2]*a+dest[2]*ia)>>8;
        dest[3] = ~((ias*ia)>>8);

        */
        // Straight copy
        dest[0] = start[0];
        dest[1] = start[1];
        dest[2] = start[2];
        dest[3] = start[3];

        if((start += 4) == end)
         return;

        dest += 4;
    }
}

bool FamiEngine::init(char * title, int width, int height, bool fullScreen)
{
   /* Initialize SDL */
    if ( SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_AUDIO) < 0 ) {
        return false;
    }

    // Init our sound engine
    m_famiSound = new FamiSound();
    m_famiSound->initSound();

    // Init our GL window
    if ( CreateGLWindow(this, title, width, height, 32, fullScreen) == false )
    {
        return false;
    }

    m_screenWidth = width;
    m_screenHeight = height;


    // Init all our screen stuff
    initGL();

    // Init our engine
    m_famiCore = new FamiCore();

    // Default the background
    m_famiCore->registers()->bgColor = 0x000000; // black

    // Zero out the joystick input
    m_famiCore->registers()->joy1 = m_famiCore->registers()->joy2 = 0;

    // Create our textures
    glGenTextures( 1, &m_bgTex);
    glGenTextures( 1, &m_nametable1Tex );
    glGenTextures( 1, &m_nametable2Tex );
    glGenTextures( 1, &m_sprTex );

    m_nametable1Surf = SDL_CreateRGBSurface(0, 256, 256, 32,
                        RMASK, GMASK, BMASK, AMASK);
    m_nametable2Surf = SDL_CreateRGBSurface(0, 256, 256, 32,
                        RMASK, GMASK, BMASK, AMASK);

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
    gLastTick = SDL_GetTicks();
    m_startTime = gLastTick;

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
    // Change the size of our projection
    glOrtho(0,m_famiWidth, m_renderHeight, 0, 0.0f, 1.0f);
}

void FamiEngine::setBGData(int idx, unsigned char tile, bool nameTable)
{
    m_famiCore->setBGTile(idx, tile, nameTable);
    SDL_Surface * surf;
    GLuint tex;

    // copy onto our name table
    if ( nameTable == FC_NAMETABLE1 )
    {
        surf = m_nametable1Surf;
        tex = m_nametable1Tex;
    }
    else
    {
        surf = m_nametable2Surf;
        tex = m_nametable2Tex;
    }

    // RGBA = 4 bytes per pixel
    SDL_Surface * vram = m_famiCore->getBGVRAM();

    unsigned int srcPixHeight = (tile/TILE_SIZE)*(vram->pitch*TILE_SIZE);
    unsigned int srcPtr = (unsigned int)vram->pixels + (srcPixHeight + (tile%TILE_SIZE)*32);// + (unsigned int)(((unsigned char)(tile/8))*32*4 + (tile%8)*(4*8));

    // debug
    unsigned int dstPixHeight = (idx/32)*(surf->pitch*TILE_SIZE);
    unsigned int dstPtr = (unsigned int)surf->pixels + (dstPixHeight + (idx%32)*32);

    for(int z = 0; z < TILE_SIZE;z++)
    {
        copy_row((const unsigned char*)srcPtr, (unsigned char*)(srcPtr + 32), (unsigned char*)dstPtr);
        srcPtr += vram->pitch;
        dstPtr += surf->pitch;
    }

    // Copy our VRAM to the background texture
    // Bind the texture object
    glBindTexture( GL_TEXTURE_2D, tex );

    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST  );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST  );

    // Edit the texture object's image data using the information SDL_Surface gives us
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, surf->w, surf->h, 0,
                      GL_RGBA, GL_UNSIGNED_BYTE, surf->pixels );
}

// Surface to copy from, how many tiles we want to pull, tile index to copy into
void FamiEngine::loadSPR(SDL_Surface * surf, int tileCopyCnt, unsigned char tileIdx)
{
    // Copy 8x8 at a time based on surface width & height
    if ( ((tileIdx+tileCopyCnt) < 0xFF) &&                  // Not over 0xFF tiles
            (surf->w % TILE_SIZE == 0) && (surf->h % TILE_SIZE == 0))     // Surface is divisible by 8
    {
        unsigned int x, y, xx, yy;
        SDL_Surface * dst = m_famiCore->getSPRVRAM();
        // Move through each tile plus the offset
        for(unsigned char i = tileIdx; i < (tileIdx+tileCopyCnt); i++)
        {
            x = (unsigned int)(i%TILE_SIZE) * TILE_SIZE;
            y = (unsigned int)(i/TILE_SIZE) * TILE_SIZE;

            xx = (unsigned int)((i-tileIdx)%(surf->w/TILE_SIZE)) *TILE_SIZE;
            yy = (unsigned int)((i-tileIdx)/(surf->h/TILE_SIZE)) *TILE_SIZE;

            unsigned int surfPtr = (unsigned int)surf->pixels + xx*4 + (yy*surf->pitch);
            unsigned int dstPtr = (unsigned int)dst->pixels + x*4 + (y*dst->pitch);
            for(int z = 0;z < TILE_SIZE;z++)
            {
                copy_row((const unsigned char*)surfPtr, (unsigned char*)(surfPtr + TILE_SIZE*4), (unsigned char*)dstPtr);
                surfPtr += surf->pitch;
                dstPtr += dst->pitch;
            }
        }

        // Copy our VRAM to the background texture
        // Bind the texture object
        glBindTexture( GL_TEXTURE_2D, m_sprTex );

        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST  );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST  );

        // Edit the texture object's image data using the information SDL_Surface gives us
        glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, dst->w, dst->h, 0,
                          GL_RGBA, GL_UNSIGNED_BYTE, dst->pixels );
    }
}

// Surface to copy from, how many tiles we want to pull, tile index to copy into
void FamiEngine::loadBG(SDL_Surface * surf, int tileCopyCnt, unsigned char tileIdx)
{
    // Copy some pixels!
    // Copy 8x8 at a time based on surface width & height
    //unsigned char tileOffY = (tileIdx/8) * 8;   // this looks like it should cancel, but because its an
                                                // unsigned char, it cannot cancel.
    if ( ((tileIdx+tileCopyCnt) < 0xFF) &&                  // Not over 0xFF tiles
            (surf->w % TILE_SIZE == 0) && (surf->h % TILE_SIZE == 0))    // Surface is divisible by 8
    {
        unsigned int x, y, xx, yy;
        SDL_Surface * dst = m_famiCore->getBGVRAM();
        // Move through each tile plus the offset
        for(unsigned char i = tileIdx; i < (tileIdx+tileCopyCnt); i++)
        {
            x = (unsigned int)(i%TILE_SIZE) * TILE_SIZE;
            y = (unsigned int)(i/TILE_SIZE) * TILE_SIZE;

            xx = (unsigned int)((i-tileIdx)%(surf->w/TILE_SIZE)) *TILE_SIZE;
            yy = (unsigned int)((i-tileIdx)/(surf->h/TILE_SIZE)) *TILE_SIZE;

            unsigned int surfPtr = (unsigned int)surf->pixels + xx*4 + (yy*surf->pitch);
            unsigned int dstPtr = (unsigned int)dst->pixels + x*4 + (y*dst->pitch);
            for(int z = 0;z < TILE_SIZE;z++)
            {
                copy_row((const unsigned char*)surfPtr, (unsigned char*)(surfPtr + TILE_SIZE*4), (unsigned char*)dstPtr);
                surfPtr += surf->pitch;
                dstPtr += dst->pitch;
            }
        }

        // Copy our VRAM to the background texture
        // Bind the texture object
        glBindTexture( GL_TEXTURE_2D, m_bgTex );

        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST  );
        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST  );

        // Edit the texture object's image data using the information SDL_Surface gives us
        glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, dst->w, dst->h, 0,
                          GL_RGBA, GL_UNSIGNED_BYTE, dst->pixels );

        // Reload our name tables
        unsigned char * nt1 = m_famiCore->getNameTable(FC_NAMETABLE1);
        unsigned char * nt2 = m_famiCore->getNameTable(FC_NAMETABLE2);
        for (int i = 0; i < 0x3C0; i++)
        {
            setBGData(i, nt1[i], FC_NAMETABLE1);
            setBGData(i, nt2[i], FC_NAMETABLE2);
        }
    }
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
    waitTimer();

    return FAMI_IDLE;
}

void FamiEngine::updateSound()
{
    m_famiSound->update();
}

void FamiEngine::updateRender()
{
    // Clear Screen, Depth Buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Draw the Background Color (default - black)
    glDisable(GL_TEXTURE_2D);
    unsigned int bColor = m_famiCore->registers()->bgColor; // background color
    glColor3ub((bColor&0xFF0000)>>24,(bColor&0x00FF00)>>16,(bColor&0x000FF)>>8);
	glBegin(GL_QUADS);
        glVertex3i(0, 0, 0);
        glVertex3i(0, m_famiHeight, 0);
        glVertex3i(m_famiWidth, m_famiHeight, 0);
        glVertex3i(m_famiWidth, 0, 0);
	glEnd();

    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	// Enable the textures again
    glEnable(GL_TEXTURE_2D);

    SpriteOAM * spr;
    float fx, fy, tx0, ty0, tx1, ty1;
    glBindTexture(GL_TEXTURE_2D, m_sprTex);
    // Find any sprites that need to go behind the background
    for(int i = 0; i < OAM_CNT; i++)
    {
        spr = m_famiCore->OAM(i);
        if (spr->behind == true )
        {
            fx = (int)(spr->tile%TILE_SIZE)*TEXTURE_RATIO;
            fy = (int)(spr->tile/TILE_SIZE)*TEXTURE_RATIO;
            if ( spr->flipX == true )
            {
                tx0 = fx+TEXTURE_RATIO;
                tx1 = fx;
            }
            else
            {
                tx0 = fx;
                tx1 = fx+TEXTURE_RATIO;
            }
            if ( spr->flipY == true )
            {
                ty0 = fy+TEXTURE_RATIO;
                ty1 = fy;
            }
            else
            {
                ty0 = fy;
                ty1 = fy+TEXTURE_RATIO;
            }
            // Draw this sprite
            glBegin(GL_QUADS);
                glTexCoord2f(tx0, ty0);
                glVertex3i(spr->x, spr->y, 0);
                glTexCoord2f(tx0, ty1);
                glVertex3i(spr->x, spr->y + 8, 0);
                glTexCoord2f(tx1, ty1);
                glVertex3i(spr->x+8, spr->y + 8, 0);
                glTexCoord2f(tx1, ty0);
                glVertex3i(spr->x+8, spr->y, 0);
            glEnd();
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
    glBindTexture(GL_TEXTURE_2D, m_nametable1Tex);
    glBegin(GL_QUADS);
        glTexCoord2f(.0f, .0f);
        glVertex3i(xOffset+l, yOffset+t, 0);
        glTexCoord2f(.0f, 1.0f);
        glVertex3i(xOffset+l, yOffset+t + 256, 0);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3i(xOffset+l + 256, yOffset+t + 256, 0);
        glTexCoord2f(1.0f, .0f);
        glVertex3i(xOffset+l + 256, yOffset+t, 0);
    glEnd();

    // NAME TABLE 3 - [Mirror of NAME TABLE 1]
    glBindTexture(GL_TEXTURE_2D, m_nametable1Tex);
    glBegin(GL_QUADS);
        glTexCoord2f(.0f, .0f);
        glVertex3i(xOffset+l, yOffset+b, 0);
        glTexCoord2f(.0f, 1.0f);
        glVertex3i(xOffset+l, yOffset+b + 256, 0);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3i(xOffset+l + 256, yOffset+b + 256, 0);
        glTexCoord2f(1.0f, .0f);
        glVertex3i(xOffset+l + 256, yOffset+b, 0);
    glEnd();

    // NAME TABLE 2
    glBindTexture(GL_TEXTURE_2D, m_nametable2Tex);
    glBegin(GL_QUADS);
        glTexCoord2f(.0f, .0f);
        glVertex3i(xOffset+r, yOffset+t, 0);
        glTexCoord2f(.0f, 1.0f);
        glVertex3i(xOffset+r, yOffset+t + 256, 0);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3i(xOffset+r + 256, yOffset+t + 256, 0);
        glTexCoord2f(1.0f, .0f);
        glVertex3i(xOffset+r + 256, yOffset+t, 0);
    glEnd();

    // NAME TABLE 4 - [Mirror of NAME TABLE 2]
    glBindTexture(GL_TEXTURE_2D, m_nametable2Tex);
    glBegin(GL_QUADS);
        glTexCoord2f(.0f, .0f);
        glVertex3i(xOffset+r, yOffset+b, 0);
        glTexCoord2f(.0f, 1.0f);
        glVertex3i(xOffset+r, yOffset+b + 256, 0);
        glTexCoord2f(1.0f, 1.0f);
        glVertex3i(xOffset+r + 256, yOffset+b + 256, 0);
        glTexCoord2f(1.0f, .0f);
        glVertex3i(xOffset+r + 256, yOffset+b, 0);
    glEnd();

    // Draw sprites in OAM that are not behind the background
    glBindTexture(GL_TEXTURE_2D, m_sprTex);
    // Find any sprites that need to go behind the background
    for(int i = 0; i < OAM_CNT; i++)
    {
        spr = m_famiCore->OAM(i);
        if (spr->behind == false )
        {
            fx = (int)(spr->tile%8)*TEXTURE_RATIO;
            fy = (int)(spr->tile/8)*TEXTURE_RATIO;
            if ( spr->flipX == true )
            {
                tx0 = fx+TEXTURE_RATIO;
                tx1 = fx;
            }
            else
            {
                tx0 = fx;
                tx1 = fx+TEXTURE_RATIO;
            }
            if ( spr->flipY == true )
            {
                ty0 = fy+TEXTURE_RATIO;
                ty1 = fy;
            }
            else
            {
                ty0 = fy;
                ty1 = fy+TEXTURE_RATIO;
            }
            // Draw this sprite
            glBegin(GL_QUADS);
                glTexCoord2f(tx0, ty0);
                glVertex3i(spr->x, spr->y, 0);
                glTexCoord2f(tx0, ty1);
                glVertex3i(spr->x, spr->y + 8, 0);
                glTexCoord2f(tx1, ty1);
                glVertex3i(spr->x+8, spr->y + 8, 0);
                glTexCoord2f(tx1, ty0);
                glVertex3i(spr->x+8, spr->y, 0);
            glEnd();
        }
    }

    //glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    //glFlush();
    //SDL_GL_SwapBuffers();
    SDL_GL_SwapWindow(m_sdlScreen);
    // Rendering done

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

void FamiEngine::waitTimer()
{
    if((SDL_GetTicks() - m_startTime) < 1000.0/FAMICOM_FPS)
        SDL_Delay(int(1000.0/FAMICOM_FPS - (SDL_GetTicks() - m_startTime)));

    m_startTime = SDL_GetTicks();
}
