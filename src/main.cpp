#include <Geode/Geode.hpp>
#include <Geode/ui/GeodeUI.hpp>
#include <Geode/loader/SettingV3.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <optional>
#include <cstdlib>

using namespace geode::prelude;

namespace {
    constexpr float kStep = 1.f / 60.f;

    struct PendingClick { int framesLeft; bool down; int button; bool p1; };

    bool g_simulating = false;   // 미리보기 시뮬레이션 중인지
    bool g_simDied = false;      // 시뮬레이션에서 죽었는지
    bool g_injecting = false;    // 모드가 직접 입력을 넣는 중인지
    std::optional<PendingClick> g_pending;

    int strength() {
        return (int)Mod::get()->getSettingValue<int64_t>("strength");
    }
    bool isEnabled() {
        return Mod::get()->getSettingValue<bool>("enabled") && strength() > 0;
    }

    // 입력을 delay 프레임 뒤에 적용했을 때 lookahead 프레임 안에 죽는지 확인
    bool simulateDies(PlayLayer* pl, bool down, int button, bool p1, int delay, int lookahead) {
        g_simulating = true;
        g_simDied = false;

        auto cp = pl->createCheckpoint();
        cp->retain();

        for (int f = 0; f < lookahead && !g_simDied; ++f) {
            if (f == delay) pl->handleButton(down, button, p1);
            pl->GJBaseGameLayer::update(kStep);
        }
        if (delay >= lookahead && !g_simDied) pl->handleButton(down, button, p1);

        pl->loadFromCheckpoint(cp);
        cp->release();

        g_simulating = false;
        return g_simDied;
    }
}

class $modify(SimPlayLayer, PlayLayer) {
    void destroyPlayer(PlayerObject* player, GameObject* object) {
        if (g_simulating) {
            g_simDied = true;   // 시뮬레이션 중의 죽음은 기록만 하고 실제 처리는 건너뜀
            return;
        }
        PlayLayer::destroyPlayer(player, object);
    }

    void resetLevel() {
        g_pending.reset();
        PlayLayer::resetLevel();
    }
};

class $modify(AssistLayer, GJBaseGameLayer) {
    void handleButton(bool down, int button, bool isPlayer1) {
        auto pl = PlayLayer::get();
        if (!pl || g_simulating || g_injecting || !isEnabled()) {
            return GJBaseGameLayer::handleButton(down, button, isPlayer1);
        }

        // 대기 중인 입력과 반대되는 입력이 들어오면 대기 입력을 먼저 처리
        if (g_pending && g_pending->down != down) {
            auto p = *g_pending;
            g_pending.reset();
            g_injecting = true;
            GJBaseGameLayer::handleButton(p.down, p.button, p.p1);
            g_injecting = false;
        }

        int s = strength();
        // 강도에 따라 개입 확률 결정
        if ((std::rand() % 100) >= s) {
            return GJBaseGameLayer::handleButton(down, button, isPlayer1);
        }

        int lookahead = 2 + s * 28 / 100;   // 2~30 프레임
        int maxDelay  = 1 + s * 9 / 100;    // 1~10 프레임

        if (!simulateDies(pl, down, button, isPlayer1, 0, lookahead)) {
            return GJBaseGameLayer::handleButton(down, button, isPlayer1);
        }
        for (int d = 1; d <= maxDelay; ++d) {
            if (!simulateDies(pl, down, button, isPlayer1, d, lookahead)) {
                g_pending = PendingClick{d, down, button, isPlayer1};
                return;   // 원래 입력은 취소
            }
        }
        // 구제할 방법이 없으면 원래 입력 그대로
        GJBaseGameLayer::handleButton(down, button, isPlayer1);
    }

    void processCommands(float dt, bool isHalfTick, bool isLastTick) {
        GJBaseGameLayer::processCommands(dt, isHalfTick, isLastTick);
        // 한 프레임에 여러 번 호출되므로 마지막 틱에서만 프레임 카운트를 줄임
        if (g_simulating || !g_pending || !isLastTick) return;
        if (--g_pending->framesLeft <= 0) {
            auto p = *g_pending;
            g_pending.reset();
            g_injecting = true;
            GJBaseGameLayer::handleButton(p.down, p.button, p.p1);
            g_injecting = false;
        }
    }
};

$on_mod(Loaded) {
    static auto handle = listenForKeybindSettingPresses(
        "menu-key",
        [](Keybind const&, bool down, bool repeat, double) {
            if (down && !repeat) openSettingsPopup(Mod::get(), false);
        }
    );
}
