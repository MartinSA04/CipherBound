#pragma once

class Battle;
struct BattlePresentationState;
class Daemon;
class GameUI;
class NPC;
class Pokedex;
class Renderer;

class BattleRenderer {
  public:
    void drawBattleScene(GameUI &ui, Battle &battle, const BattlePresentationState &presentation,
                         int battleAnimFrame, int attackAnimFrame, bool captureAnimDone) const;
    void drawBattleIntroSceneWild(GameUI &ui, Battle &battle,
                                  const BattlePresentationState &presentation) const;
    void drawBattleIntroSceneTrainer(GameUI &ui, Battle &battle,
                                     const BattlePresentationState &presentation) const;
    void drawBattleMenu(GameUI &ui, int menuSelected) const;
    void drawMoveSelectScreen(GameUI &ui, const Daemon &daemon, const Pokedex &pokedex,
                              int moveSelected) const;
    void drawCaptureScene(GameUI &ui, Battle &battle, const BattlePresentationState &presentation,
                          int captureAnimFrame, bool captureAnimDone) const;
    void drawPlayerSwitchScene(GameUI &ui, Battle &battle,
                               const BattlePresentationState &presentation,
                               int battleAnimFrame) const;

  private:
    struct BaseGeometry {
        int x, y, w, h;
    };

    void drawBall(Renderer &renderer, int frame, int x, int y) const;
    void drawBallCentered(Renderer &renderer, int frame, int cx, int cy) const;

    void drawBattleBackground(Renderer &renderer) const;
    BaseGeometry getPlayerBaseGeometry() const;
    BaseGeometry getOpponentBaseGeometry() const;
    void drawPlayerBase(Renderer &renderer) const;
    void drawOpponentBase(Renderer &renderer, int offsetX = 0) const;
    void drawPlayerBackSprite(Renderer &renderer, int x, int y, int dstW, int dstH,
                              int frame) const;
    void drawOpponentTrainer(GameUI &ui, const NPC *opponent, int offsetX = 0) const;
    void drawOpponentInfoBar(GameUI &ui, const Daemon *opponentDaemon,
                             const BattlePresentationState &presentation, int offsetX = 0) const;
    void drawPlayerInfoBar(GameUI &ui, const Daemon *playerDaemon,
                           const BattlePresentationState &presentation, int offsetX = 0) const;
    void drawOpponentDaemon(GameUI &ui, const Daemon *opponentDaemon, int offsetX = 0,
                            int offsetY = 0) const;
    void drawPlayerDaemon(GameUI &ui, const Daemon *playerDaemon, int offsetX = 0,
                          int offsetY = 0) const;
    void drawPlayerBackOnBase(Renderer &renderer, int offsetX = 0, int frame = 0) const;
    void drawPlayerSendOutPhase(GameUI &ui, const Daemon *playerDaemon,
                                const BattlePresentationState &presentation, float t) const;
};
