#ifndef game_h__
#define game_h__

#include <game_export.h>

class Engine;

GAME_EXPORT void initGame(Engine *engine) noexcept;
GAME_EXPORT void destroyGame() noexcept;

GAME_EXPORT void startGame() noexcept;
GAME_EXPORT void pauseGame() noexcept;
GAME_EXPORT void updateGame() noexcept;
GAME_EXPORT void stopGame() noexcept; 

GAME_EXPORT void* textureHandle() noexcept;
GAME_EXPORT void resizeGame(int width, int height) noexcept; 

#endif // game_h__