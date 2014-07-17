#include "HiveGame.h"

HiveGame::HiveGame()
{
    m_fEngine = new FamiEngine();
    m_fEngine->init("Hive Mind - Team YUS 2014");

    // Initialize all of our states
    m_hiveIntro = new HiveIntro(m_fEngine);
    m_state = INTRO;
}

HiveGame::~HiveGame()
{
    delete m_hiveIntro;
    delete m_fEngine; // delete it, calls deconstructor
}

void HiveGame::run()
{
    // Call init type things here, like loading sprites,background,register,etc.
    while( m_fEngine->update() == FAMI_IDLE )
    {
        // Check to see which state we are in....
        switch( m_state ){
        case INTRO:
            if ( m_hiveIntro->update(m_fEngine->getJoy1()) == DONE )
            {
                m_state = MENU;
            }
            break;
        case MENU:
            if ( m_fEngine->getJoy1() & J_UP )
            {

            }
            else if ( m_fEngine->getJoy1() & J_DOWN )
            {

            }
            if ( m_fEngine->getJoy1() & J_START || m_fEngine->getJoy1() & J_A || m_fEngine->getJoy1() & J_B )
            {
                // Go to options or play
            }
            break;
        case OPTIONS:
            break;
        case PLAY:
            break;
        };
        m_fEngine->waitForVsync(); // Wait for VSYNC (NES interrupt)
    }
}
