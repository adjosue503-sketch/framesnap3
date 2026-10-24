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
static float g_avgDt = 1.f / 60.f;

static bool framePerfect() {
    return Mod::get()->getSettingValue<bool>("frame-perfect");
}

static float baseWindow() {
    return Mod::get()->getSettingValue<int64_t>("window-ms") / 1000.f;
}

class $modify(FrameSnapLayer, GJBaseGameLayer) {
    void handleButton(bool down, int button, bool isPlayer1) {
        if (!framePerfect() || g_applying || !PlayLayer::get()) {
            return GJBaseGameLayer::handleButton(down, button, isPlayer1);
        }
        g_queue.push_back({down, button, isPlayer1, Clock::now()});
    }

    void update(float dt) {
        if (!framePerfect() || !PlayLayer::get()) {
            g_queue.clear();
            return GJBaseGameLayer::update(dt);
        }

        // promedio del tiempo de frame: se adapta a cada nivel
        g_avgDt = g_avgDt * 0.95f + dt * 0.05f;

        if (g_queue.empty() || dt <= 0.f) {
            return GJBaseGameLayer::update(dt);
        }

        float window = std::max(baseWindow(), g_avgDt * 2.f);
        auto frameStart = Clock::now() - std::chrono::duration_cast<Clock::duration>(FSec(dt));
        float consumed = 0.f;

        while (!g_queue.empty()) {
            auto click = g_queue.front();
            g_queue.pop_front();

            float offset = FSec(click.time - frameStart).count();
            if (offset < -window) offset = 0.f;
            offset = std::clamp(offset, consumed, std::max(consumed, dt));

            float run = offset - consumed;
            if (run > 0.f) {
                GJBaseGameLayer::update(run);
                consumed += run;
            }

            g_applying = true;
            GJBaseGameLayer::handleButton(click.down, click.button, click.isPlayer1);
            g_applying = false;
        }

        if (dt - consumed > 0.f) {
            GJBaseGameLayer::update(dt - consumed);
        }
    }
};
