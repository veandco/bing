#pragma once

#include "bing/speech.hpp"

namespace Bing {
    void        init();
    void        run();
    void        stop();
    void        quit();
    gpointer    threadCallback(gpointer data);        
};
