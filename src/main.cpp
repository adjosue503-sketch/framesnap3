#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <algorithm>
#include <chrono>
#include <deque>

using namespace geode::prelude;
using Clock = std::chrono::steady_clock;
using FSec = std::chrono::duration<float>;

struct QueuedClick {
    bool down;
    int button;
    bool isPlayer1;
    Clock::time_point time;
};

static std::deque<QueuedClick> g_queue;
static bool g_applying = false;

static bool framePerfect() {
    return Mod::get()->getSettingValue<bool>("frame-perfect");
}

class $modify(FrameSnapLayer, GJBaseGameLayer) {
    void handleButton(bool down, int button, bool isPlayer1) {
        if (!framePerfect() || g_applying) {
            return GJBaseGameLayer::handleButton(down, button, isPlayer1);
        }
        g_queue.push_back({down, button, isPlayer1, Clock::now()});
    }

    void processCommands(float dt) {
        if (!framePerfect()) {
            g_queue.clear();
            return GJBaseGameLayer::processCommands(dt);
        }
        if (g_queue.empty()) {
            return GJBaseGameLayer::processCommands(dt);
        }

        auto frameStart = Clock::now() - std::chrono::duration_cast<Clock::duration>(FSec(dt));
        float consumed = 0.f;

        while (!g_queue.empty()) {
            auto click = g_queue.front();
            g_queue.pop_front();

            float offset = FSec(click.time - frameStart).count();
            offset = std::clamp(offset, consumed, std::max(consumed, dt));

            float run = offset - consumed;
            if (run > 0.f) {
                GJBaseGameLayer::processCommands(run);
                consumed += run;
            }

            g_applying = true;
            GJBaseGameLayer::handleButton(click.down, click.button, click.isPlayer1);
            g_applying = false;
        }

        if (dt - consumed > 0.f) {
            GJBaseGameLayer::processCommands(dt - consumed);
        }
    }
};
