#include <PuzzleEngine/core/Application.h>
#include <PuzzleEngine/core/Input.h>
#include <PuzzleEngine/core/InputContext.h>

#include <spdlog/spdlog.h>

enum UIAction : SYN::ActionID { OpenSettings, CloseSettings };
enum GameAction : SYN::ActionID { Heal, Damage, ShowHealth };

class UIContext : public SYN::InputContext {
  public:
    void onActionReceive(SYN::ActionBinding actionBinding) override {
        if (actionBinding.m_State == SYN::InputState::Up)
            return;
        switch (actionBinding.m_Action) {
        case UIAction::OpenSettings: {
            spdlog::info("Opening settings!");
            break;
        }
        case UIAction::CloseSettings: {
            spdlog::info("Close settings!");
            break;
        }
        }
    }
};

class GameContext : public SYN::InputContext {
  public:
    GameContext() {
        m_Health = 100;
        m_HealPoints = 10;
        m_DamageReceived = 5;
    }
    GameContext(int32_t health, int32_t healPoints, int32_t damageReceived) {
        m_Health = health;
        m_HealPoints = healPoints;
        m_DamageReceived = damageReceived;
    }
    void onActionReceive(SYN::ActionBinding actionBinding) override {
        if (actionBinding.m_State == SYN::InputState::Up)
            return;
        switch (actionBinding.m_Action) {
        case GameAction::Heal: {
            spdlog::info("Healing for {} points!", m_HealPoints);
            m_Health += m_HealPoints;
            break;
        }
        case GameAction::Damage: {
            spdlog::info("Taking {} damage!", m_DamageReceived);
            m_Health -= m_DamageReceived;
            break;
        }
        case GameAction::ShowHealth: {
            spdlog::info("You have {} health!", m_Health);
            break;
        }
        }
    }

  private:
    int32_t m_Health;
    int32_t m_HealPoints;
    int32_t m_DamageReceived;
};

int main() {
    // todo move outside engine
    std::unique_ptr<SYN::Application> app =
        std::make_unique<SYN::Application>();

    app->init();

    app->GetInput()->assignAction(SYN::InputKey::Q, GameAction::Heal);
    app->GetInput()->assignAction(SYN::InputKey::E, GameAction::Damage);
    app->GetInput()->assignAction(SYN::InputKey::R, GameAction::ShowHealth);
    std::optional<SYN::InputContextHandle> gameContext =
        app->GetInput()->addInputContext<GameContext>(0);
    if (!gameContext) {
        spdlog::error("Unable to create game context!");
    } else {
        SYN::InputContext *ctx =
            app->GetInput()->getInputContext(gameContext.value()).value();
        app->GetInput()->disableContext(gameContext.value());
        app->GetInput()->enableContext(gameContext.value());
        ctx->enableAction(GameAction::Heal);
        ctx->enableAction(GameAction::Damage);
        ctx->enableAction(GameAction::ShowHealth);
    }

    app->run();
    app->shutdown();
}
