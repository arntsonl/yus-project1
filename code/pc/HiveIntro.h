#ifndef HIVEINTRO_H
#define HIVEINTRO_H

#include "famiengine/famiengine.h"
enum IntroStates { IDLE, DONE };

class HiveIntro
{
    public:
        HiveIntro(FamiEngine*);
        virtual ~HiveIntro();
        int update(int); // update our timers...
    private:
        unsigned int m_internalTimer;
        // some cool logic
        FamiEngine * m_fEngine;
};

#endif // HIVEINTRO_H
