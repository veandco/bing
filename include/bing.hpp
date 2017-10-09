#pragma once

#include "bing/speech.hpp"
#include <glib.h>

namespace Bing {
    void        init();
    void        run();
    void        stop();
    void        quit();
    gpointer    threadCallback(gpointer data);        
};
