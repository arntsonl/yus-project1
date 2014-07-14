#include "famiengine/famiengine.h"

unsigned char mapData[] = {
15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,5,3,3,3,6,15,15,15,15,15,15,
15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,4,15,15,15,4,15,15,15,15,15,15,
15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,4,15,15,15,4,15,15,15,15,15,15,
15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,4,15,15,15,4,15,15,15,15,15,15,
15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,4,15,15,15,4,15,15,15,15,15,15,
15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,4,15,15,15,4,15,15,15,15,15,15,
7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,4,7,7,7,4,7,7,7,7,7,7,
7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,4,7,7,7,4,7,7,7,7,7,7,
7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,4,7,7,7,4,7,7,7,7,7,7,
7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,4,7,7,7,4,7,7,7,7,7,7,
7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,4,7,7,7,4,7,7,7,7,7,7,
7,7,7,7,7,7,7,7,7,7,5,3,3,3,3,3,3,3,3,3,3,13,7,7,7,4,7,7,7,7,7,7,
7,7,7,7,7,7,7,7,7,7,12,3,3,3,6,7,7,7,7,7,7,7,7,7,7,4,7,7,7,7,7,7,
7,7,7,7,7,7,7,7,7,7,7,7,7,7,12,3,3,3,3,3,3,3,3,3,3,13,7,7,7,7,7,7,
7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,10,10,
7,7,7,7,7,7,10,11,10,10,10,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,10,10,10,
7,7,7,7,10,10,10,10,11,10,10,10,10,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,10,11,11,10,
10,10,10,10,10,11,10,10,10,11,10,10,10,11,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,
14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,
14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,
14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,
14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14
};

int main(int argc, char * argv[])
{
    FamiEngine * fEngine = new FamiEngine();
    fEngine->init("Forgotten Star", 640, 480, false);

    // Init our Famicom Data
    SDL_Surface * newBg = LoadIMG2RGBA("\\data\\exampletiles.png");
    fEngine->loadBG(newBg, 64, 0);   // loads our example tiles into BGVRAM
    SDL_FreeSurface(newBg);

    SDL_Surface * newSpr = LoadIMG2RGBA("\\data\\examplesprites.png");
    fEngine->loadSPR(newSpr, 64, 0); // loads our example sprites into SPRVRAM
    SDL_FreeSurface(newSpr);

    int i;
    for(i = 0; i < 0x3C0; i++)
    {
        if ( mapData[i] > 0 )
            fEngine->setBGData(i, mapData[i], FC_NAMETABLE1);
    }

    for(i = 0; i < 0x3C0; i++)
    {
        fEngine->setBGData(i, 5, FC_NAMETABLE2);
    }

    // Moves everyone off screen
    for(i = 0; i < 64; i++)
    {
        fEngine->setPositionOAM(i, 0, 0xFE); // move them off screen
    }

    fEngine->setBg(FAMI_BGRED);

    // Lets make a player sprite
    fEngine->setOAM(0, 0, 25, 25, false, false, false);
    fEngine->setOAM(1, 8, 25, 33, false, false, false);
    fEngine->setOAM(2, 1, 50, 25, false, false, false);

    int scrollingX = 0;
    int scrollingY = 0;
    int scrollingX_NT = 0;
    int scrollingY_NT = 0;

    // Call init type things here, like loading sprites,background,register,etc.
    while( fEngine->update() == FAMI_IDLE )
    {
        if ( fEngine->getJoy1() & J_LEFT )
        {
            if ( scrollingX - 1 < 0 )
            {
                scrollingX = 7;
                if ( scrollingX_NT - 1 < 0 )
                {
                    fEngine->setScrollXNT(32);
                    scrollingX_NT = 31;
                }
                else
                {
                    scrollingX_NT--;
                }
                fEngine->setScrollXNT(scrollingX_NT);
            }
            else
            {
                scrollingX--;
            }
            fEngine->setScrollX(scrollingX);
        }
        else if ( fEngine->getJoy1() & J_RIGHT )
        {
            if ( scrollingX + 1 > 7 )
            {
                scrollingX = 0;
                if ( scrollingX_NT + 1 > 31 )
                {
                    fEngine->setScrollXNT(32);
                    scrollingX_NT = 0;
                }
                else
                {
                    scrollingX_NT++;
                }
                fEngine->setScrollXNT(scrollingX_NT);
            }
            else
            {
                scrollingX++;
            }
            fEngine->setScrollX(scrollingX);
        }
        if ( fEngine->getJoy1() & J_UP )
        {
            if ( scrollingY - 1 < 0 )
            {
                scrollingY = 7;
                if ( scrollingY_NT - 1 < 0 )
                {
                    fEngine->setScrollYNT(30);
                    scrollingY_NT = 29;
                }
                else
                {
                    scrollingY_NT--;
                }
                fEngine->setScrollYNT(scrollingY_NT);
            }
            else
            {
                scrollingY--;
            }
            fEngine->setScrollY(scrollingY);
        }
        else if ( fEngine->getJoy1() & J_DOWN )
        {
            if ( scrollingY + 1 > 7 )
            {
                scrollingY = 0;
                if ( scrollingY_NT + 1 > 29 )
                {
                    fEngine->setScrollYNT(30);
                    scrollingY_NT = 0;
                }
                else
                {
                    scrollingY_NT++;
                }
                fEngine->setScrollYNT(scrollingY_NT);
            }
            else
            {
                scrollingY++;
            }
            fEngine->setScrollY(scrollingY);
        }
    }

    delete fEngine; // delete it, calls deconstructor

    return 0;
}
