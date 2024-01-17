#ifndef game_h__
#define game_h__

struct Game;

struct GameView
{
    Game *game;
    void gui();
    void toolbar();

    enum class GameState
    {
        started,
        paused,
        stopped
    } gameState = GameState::stopped;
};

#endif // game_h__
