#ifndef HIVEGAME_H
#define HIVEGAME_H

#include "famiengine/famiengine.h"

// States
#include "HiveIntro.h"

enum HiveGameState { INTRO, MENU, OPTIONS, PLAY };

class HiveGame
{
    public:
        HiveGame();
        virtual ~HiveGame();
        void run();
    private:
        FamiEngine * m_fEngine;
        HiveIntro * m_hiveIntro;
        int m_state;
};

#endif // HIVEGAME_H
