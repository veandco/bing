#include "bing.hpp"

namespace Bing {
    static GMainLoop * mMainLoop;
    static GThread *   mThread;

    void init()
    {
        mMainLoop = g_main_loop_new(NULL, true);
    }

    void quit()
    {
        stop();

        if (mMainLoop)
            g_main_loop_unref(mMainLoop);

        if (mThread)
            g_thread_unref(mThread);
    }

    void run()
    {
        if (!mMainLoop)
            return;

        g_main_loop_run(mMainLoop);
    }

    void stop()
    {
        if (mMainLoop) {
            if (g_main_loop_is_running(mMainLoop))
                g_main_loop_quit(mMainLoop);
        }
    }

    gpointer threadCallback(gpointer data)
    {
        Bing::run();
    }
}
