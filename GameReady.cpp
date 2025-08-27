// gameready.cpp
#include "main.h"
#include "GameReady.h"

#include "Map.h"
#include "gamebg.h"
#include "player.h"   // ← 确保包含它，使用 Player_EnsureAllocated
#include "BuildGlobals.h"
#include "BuildUI.h"
#include "BuildSystem.h"
#include "controller.h"
#include "game.h" 

// === 追加：战斗格子位置（九宫格中心的右边 +3 步距）===
static Object_2D::Vec2 CalcBattleTileCenter()
{
    Object_2D::Vec2* Box = GetBoxSzie();
    Object_2D::Vec2* MPos = GetMapPos();
    const float halfW = (MAP_WIDTH - 1) * 0.5f;
    const float halfH = (MAP_HEIGHT - 1) * 0.5f;
    const float stepX = Box->x * GetTileSpacingX();
    const float stepY = Box->y * GetTileSpacingY();

    Object_2D::Vec2 center;
    center.x = (1 - halfW) * stepX + MPos->x;
    center.y = (1 - halfH) * stepY + MPos->y;

    Object_2D::Vec2 p;
    p.x = center.x + stepX * 3.0f;
    p.y = center.y;
    return p;
}

static bool IsPointInRect(const Object_2D::Vec2& p, const Object_2D::Vec2& c, const Object_2D::Vec2& wh)
{
    return (p.x >= c.x - wh.x * 0.5f && p.x <= c.x + wh.x * 0.5f &&
        p.y >= c.y - wh.y * 0.5f && p.y <= c.y + wh.y * 0.5f);
}

// —— 空格/左右 的触发沿缓存
static bool s_prevSpace = false;
static bool s_prevLeft = false;
static bool s_prevRight = false;

void InitializeGameReady(void)
{
    InitializeGamebg();
    InitializePlayer();   // ← 先建人
    InitializeMap();

    InitBuildUI();
    EnsureBuildInitialized();
}

void UpdateGameReady(void)
{
    // ★★★ 关键：帧帧确保 player 存在，避免任何顺序问题导致的空指针
    Player_EnsureAllocated();

    UpdatePlayer();
    UpdateMap();
    UpdateGamebg();

    // === 站位判定 ===
    gBuild.playerStanding.fill(false);
    Object_2D* p = GetPlayer();
    if (!p) {
        // 极限情况下（外部重新释放了 player），先不做菜单逻辑，等待下一帧重建
        return;
    }
    int idx = GetTileIndexFromWorld(p->Transform.Position);
    if (idx >= 0) gBuild.playerStanding[idx] = true;

    // === 键盘触发沿 ===
    extern Keyboard kbd;
    extern Mouse mouse;

    const bool spaceNow = kbd.KeyIsPressed(VK_SPACE);
    const bool leftNow = kbd.KeyIsPressed(VK_LEFT);
    const bool rightNow = kbd.KeyIsPressed(VK_RIGHT);

    const bool spaceTrig = spaceNow && !s_prevSpace;
    const bool leftTrig = leftNow && !s_prevLeft;
    const bool rightTrig = rightNow && !s_prevRight;

    s_prevSpace = spaceNow;
    s_prevLeft = leftNow;
    s_prevRight = rightNow;

    bool justOpened = false;

    // === 战斗格子特殊菜单 ===
    if (!gBuild.menuOpen && spaceTrig)
    {
        Object_2D::Vec2 my = p->Transform.Position;

        Object_2D::Vec2 battleC = CalcBattleTileCenter();
        Object_2D::Vec2 box = *GetBoxSzie(); // 用格子同尺寸
        if (IsPointInRect(my, battleC, box))
        {
            RTS9::OpenBattleConfirm(gBuild);
            justOpened = true;
        }
    }

    // === 普通：站在九宫格上打开该地块菜单 ===
    if (!gBuild.menuOpen && idx >= 0 && spaceTrig && !justOpened) {
        RTS9::OpenMenu(gBuild, idx);
        justOpened = true;
    }

    // === 菜单输入（只用触发沿） ===
    RTS9::InputState in;
    in.leftClickTileIndex = -1;
    in.leftArrowPressed = leftTrig;
    in.rightArrowPressed = rightTrig;
    in.spacePressed = (gBuild.menuOpen && spaceTrig && !justOpened);
    in.closePressed = mouse.RightIsPressed() || kbd.KeyIsPressed(VK_ESCAPE);

    RTS9::HandleInput(gBuild, in);
}

void DrawGameReady(void)
{
    DrawGamebg();
    DrawMap();
    DrawBuildMenu(gBuild);
    DrawBuildWorld(gBuild);
    

    if (!Player_IsInitialized()) {
        // 极端情况下保证一下
        InitializePlayer();
    }
    DrawPlayer();
}

void FinalizeGameReady(void)
{
    FinalizePlayer();
    FinalizeGamebg();
    FinalizeMap();
    UninitBuildUI();
}
