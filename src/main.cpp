#include "core/Session.h"

namespace {

constexpr int defaultSeed = 42;

}

#ifdef __EMSCRIPTEN__
#include <chrono>
#include <emscripten.h>

static Session *g_session = nullptr;

static void emscriptenMainLoop() { g_session->tick(); }
#endif

int main() {
    Session session(defaultSeed);
    session.init();

#ifdef __EMSCRIPTEN__
    g_session = &session;
    // 0 = use requestAnimationFrame for smooth vsync-driven callbacks
    emscripten_set_main_loop(emscriptenMainLoop, 0, 1);
#else
    session.run();
#endif

    return 0;
}
