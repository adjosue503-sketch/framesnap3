#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>

using namespace geode::prelude;

class $modify(FrameSnapLayer, GJBaseGameLayer) {
    void handleButton(bool down, int button, bool isPlayer1) {
        if (Mod::get()->getSettingValue<bool>("frame-perfect")) {
            log::debug("FrameSnap: click down={} button={}", down, button);
        }
        GJBaseGameLayer::handleButton(down, button, isPlayer1);
    }
};
