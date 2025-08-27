// Map.cpp
#include "main.h"
#include "Map.h"
#include "texture.h"
#include "game.h"

building map[MAP_WIDTH * MAP_HEIGHT];
int MapTexturesID[MAP_WIDTH * MAP_HEIGHT];

Object_2D::Vec2 BoxSzie;
Object_2D::Vec2 MapPos;

Object_2D objmap(0.0f, 0.0f, 0.0f, 0.0f);
float ColorA = 1.0f;

// ===== 可调参数：格子尺寸 & 间距 =====
static float kTileSize = 300.0f;   // ← 想更大就改这个
static float gTileSpacingX = 0.82f; // ← 想“更挨着”就把它调小些(0.80~0.95)
static float gTileSpacingY = 0.88f; //    1.00 表示边对边，无缝；<1 会略重叠

// 对外查询/设置
float GetTileSpacingX() { return gTileSpacingX; }
float GetTileSpacingY() { return gTileSpacingY; }
void  SetTileSpacing(float sx, float sy) { gTileSpacingX = sx; gTileSpacingY = sy; }

// —— 把世界坐标反查为格子索引（与 Draw/BuildUI 保持同公式）
int GetTileIndexFromWorld(const Object_2D::Vec2& w)
{
    const float halfW = (MAP_WIDTH - 1) * 0.5f;
    const float halfH = (MAP_HEIGHT - 1) * 0.5f;

    const float stepX = BoxSzie.x * gTileSpacingX;
    const float stepY = BoxSzie.y * gTileSpacingY;

    float fx = (w.x - MapPos.x) / stepX + halfW;
    float fy = (w.y - MapPos.y) / stepY + halfH;

    int x = (int)std::roundf(fx);
    int y = (int)std::roundf(fy);

    if (x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT) return -1;

    return y * MAP_WIDTH + x;
}

void GameInitializeMap(void)
{
    objmap.Color.A = 0.5f;

    MapTexturesID[0] = LoadTexture("rom:/block_01.tga"); // 地面/空地
    MapTexturesID[1] = LoadTexture("rom:/CC-0001.tga");  // 示例：备用

    BoxSzie = { kTileSize, kTileSize };   // 统一格子尺寸
    MapPos = { -350.0f, 00.0f };           // 统一偏移（以前是在 Draw 里设置的）

    for (int i = 0; i < MAP_WIDTH * MAP_HEIGHT; ++i) {
        map[i].SetType(BuildingType::Empty);
    }
}



void InitializeMap(void)
{
    for (int i = 0; i < MAP_WIDTH * MAP_HEIGHT; ++i) {
        if (map[i].GetType() != BuildingType::Empty) {
            Startfunction(&map[i]);
        }
    }
}

void UpdateMap(void)
{
}

void DrawMap(void)
{
    const float halfW = (MAP_WIDTH - 1) * 0.5f;
    const float halfH = (MAP_HEIGHT - 1) * 0.5f;

    const float stepX = BoxSzie.x * gTileSpacingX;
    const float stepY = BoxSzie.y * gTileSpacingY;

    for (int i = 0; i < MAP_WIDTH * MAP_HEIGHT; ++i)
    {
        objmap.Color.A = ColorA;
        objmap.Size = BoxSzie;

        const int y = i / MAP_WIDTH;
        const int x = i % MAP_WIDTH;

        objmap.Transform.Position.x = (x - halfW) * stepX + MapPos.x;
        objmap.Transform.Position.y = (y - halfH) * stepY + MapPos.y;

        objmap.texNo = MapTexturesID[static_cast<int>(map[i].GetType())];

        BoxVertexsMake(&objmap);
        DrawObject_2D(&objmap);

        // —— 已去掉“每格再叠一个示例icon”的演示代码
    }
}

void FinalizeMap(void)
{
}

Object_2D::Vec2* GetMapPos(void) { return &MapPos; }
building* GetMap(void) { return map; }
int* GetMapTexturesID(void) { return MapTexturesID; }
Object_2D::Vec2* GetBoxSzie(void) { return &BoxSzie; }
Object_2D* GetObject(void) { return &objmap; }
float* GetColorA(void) { return &ColorA; }

void Startfunction(building* buil)
{
    switch (buil->GetType())
    {
    case BuildingType::Empty:     StartEmpty(buil->GetLv());     break;
    case BuildingType::Shrine:    StartShrine(buil->GetLv());    break;
    case BuildingType::Market:    StartMarket(buil->GetLv());    break;
    case BuildingType::Guild:     StartGuild(buil->GetLv());     break;
    case BuildingType::Range:     StartRange(buil->GetLv());     break;
    case BuildingType::Barracks:  StartBarracks(buil->GetLv());  break;
    case BuildingType::Sanctuary: StartSanctuary(buil->GetLv()); break;
    default: break;
    }
}

// 空地
void StartEmpty(int) {}
// 圣殿
void StartShrine(int) {}
// 集市
void StartMarket(int Lv) { if (Lv > 0) { ADDMoney(Lv * 10 + 5); } }
// 工会
void StartGuild(int Lv) { if (Lv > 0) { ADDPlayers(Lv); } }
// 靶场
void StartRange(int) {}
// 兵营
void StartBarracks(int) {}
// 圣所
void StartSanctuary(int) {}




int GetBuildingLevel(BuildingType type)
{
    int best = 0;
    for (int i = 0; i < MAP_WIDTH * MAP_HEIGHT; ++i) {
        if (map[i].GetType() == type) {
            best = (std::max)(best, map[i].GetLv());  // 注意括号写法，避开 Windows 宏
        }
    }
    return best;
}
