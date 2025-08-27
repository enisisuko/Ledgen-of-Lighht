#pragma once

// Map.h
#define MAP_WIDTH 3
#define MAP_HEIGHT 3


enum class BuildingType
{
    Empty,      // 空地
    Shrine,     // 圣殿
    Market,     // 集市
    Guild,      // 工会
    Range,      // 靶场
    Barracks,   // 兵营
    Sanctuary   // 圣所
};


class building
{

private:
    BuildingType type = BuildingType::Empty;
    int PlayerTextureId = 0;//テクスチャID
    int Lv = 0;
public:
    BuildingType GetType(void) { return type; }
    void SetType(BuildingType tp) { type = tp; }

    int GetLv(void) { return Lv; }
    void UpLv(void) { if (Lv < 3) { Lv++; } }
    void DownLv(void) { if (Lv > 0) { Lv--; } }

};

void GameInitializeMap(void);
void InitializeMap(void);
void UpdateMap(void);
void DrawMap(void);
void FinalizeMap(void);


Object_2D::Vec2* GetMapPos(void);

building* GetMap(void);

int* GetMapTexturesID(void);

Object_2D::Vec2* GetBoxSzie(void);

Object_2D* GetObject(void);

float* GetColorA(void);



void StartEmpty(int Lv);      // 空地
void StartShrine(int Lv);     // 圣殿
void StartMarket(int Lv);     // 集市
void StartGuild(int Lv);     // 工会
void StartRange(int Lv);      // 靶场
void StartBarracks(int Lv);  // 兵营
void StartSanctuary(int Lv);  // 圣所






void Startfunction(building* buil);

// Map.h 追加
Object_2D::Vec2 GetTileCenter(int idx);
int GetTileIndexFromWorld(const Object_2D::Vec2& worldPos);
int GetBuildingLevel(BuildingType type);


// Map.h 里新增
float GetTileSpacingX();
float GetTileSpacingY();
void  SetTileSpacing(float sx, float sy);
int   GetTileIndexFromWorld(const Object_2D::Vec2& world);  // 站位反查格子
