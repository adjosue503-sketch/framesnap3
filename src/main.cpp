#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>

using namespace geode::prelude;

static bool framePerfect() {
    return Mod::get()->getSettingValue<bool>("frame-perfect");
}

class $modify(FrameSnapLayer, GJBaseGameLayer) {
    void update(float dt) {
        if (framePerfect()) {
            log::debug("FrameSnap: update dt={}", dt);
        }
        GJBaseGameLayer::update(dt);
    }
};
