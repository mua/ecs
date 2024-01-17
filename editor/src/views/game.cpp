#include "game.h"
#include "types.h"

#include "imgui.h"

#include "editor.h"
#include "IconsFontAwesome.h"
#include "utils/uiutils.h"

void GameView::gui()
{
    if (!game->textureHandle)
    {
        return;
    }
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    bool isOpen = true;
    if (ImGui::Begin("Game", &isOpen))
    {
        ImVec2 vMin = ImGui::GetWindowContentRegionMin();
        ImVec2 vMax = ImGui::GetWindowContentRegionMax();

        vMin.x += ImGui::GetWindowPos().x;
        vMin.y += ImGui::GetWindowPos().y;
        vMax.x += ImGui::GetWindowPos().x;
        vMax.y += ImGui::GetWindowPos().y;
        auto region = ImGui::GetWindowContentRegionMax();
        ImVec2 size = ImVec2(region.x, glm::max<float>(1, vMax.y - vMin.y));

        auto handle = game->textureHandle();
        ImGui::Image((void *)(intptr_t)handle, ImVec2(size.x, size.y - 3), ImVec2(0, 1), ImVec2(1, 0));
        game->resizeGame(size.x, size.y);
    }
    ImGui::End();
    ImGui::PopStyleVar();
}

void GameView::toolbar()
{        
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(3.0f, 5.0f));

    int selected = 0;
    const char* playIcon = ICON_FA_PLAY;
    switch (gameState)
    {
    case GameState::started:
        selected = 0;
        playIcon = ICON_FA_PAUSE;
        break;
    case GameState::paused:
        selected = 0;
        break;
    case GameState::stopped:
        selected = -1;
        break;
    }
    if (UIUtils::toggleButtonSet({playIcon, ICON_FA_STOP}, &selected))
    {
        if (selected == 0 && gameState == GameState::started)
        {
            gameState = GameState::paused;            
            game->pauseGame();
        }
        else if (selected == 0)
        {
            gameState = GameState::started;
            game->startGame();            
        }
        else if (selected == 1)
        {
            gameState = GameState::stopped;
            game->stopGame();
        }
    }
    ImGui::PopStyleVar(2);
}
