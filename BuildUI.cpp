// BuildUI.cpp —— 贴图版（间距因子 / 升级用下一等级贴图 / 无选择框&背景 / 无灰色兜底）
// 关键更新：
// - 详情卡片补全 Build_* → zheshi*.tga（招募屋/圣女不再回“返回”）
// - 花销面板仅在【建造1级 / 升级 / 专职 / 拆卸】显示，跟随主角；支持“+”号贴图
// - 升级费用：1→2=30 金币，2→3=50 金币，3→4=200 金币 + 20 单位（示例）
// - 拆卸返还：Lv1=+1单位，Lv2=+20 金币，Lv3=+60 金币
// - 专职默认花 1 单位；建造1级固定花 1 单位 + 各建筑金币表

#include "main.h"
#include "texture.h"
#include "sprite.h"
#include "player.h"
#include "Map.h"
#include "BuildGlobals.h"
#include "BuildSystem.h"

#include <unordered_map>
#include <string>
#include <cmath>
#include <vector>
#include <cstdlib>
#include <cctype>

//// ---- BuildUI forward declares ----
//namespace RTS9 { struct GameState; }      // 前置声明（避免强依赖头里完整定义）
//void BuildUI_UpdateOnlyCountdown();       // 更新时间轴（无参数）
//void BuildUI_DrawTopMost(const RTS9::GameState& gs);  // 顶层绘制

// ====== 前置声明 ======
static void BuildUI__BeginCountdown_CB();
void BuildUI_DrawBattleCountdownOverlay(bool doUpdate);
static bool BuildUI_IsBattleCountdownActive();

// —— 在使用前做前置声明（修复编译错误）
struct MenuItemSnap { std::string iconName; RTS9::MenuAction action; };
static std::string ChooseSpecializeUnitIcon(RTS9::BuildingType ty, int level);
static void MakeMenuSnapshot(const RTS9::GameState& gs, std::vector<MenuItemSnap>& dst);




// ====== 小工具 & 动画曲线 ======
static inline float saturate(float x) { return x < 0.f ? 0.f : (x > 1.f ? 1.f : x); }
static inline float UI_dt() { return 1.0f / 60.0f; }
static inline float frand(float a, float b) { return a + (b - a) * (std::rand() / (float)RAND_MAX); }
static inline std::string lower(std::string s) { for (char& c : s) c = (char)std::tolower((unsigned char)c); return s; }

// === 角度↔弧度 & 扇形布局参数（可按需微调） ===
static inline float Deg2Rad(float d) { return d * 3.1415926f / 180.0f; }

// 菜娘推荐：右下微倾 + 稍微拉开间距
static constexpr float MENU_RADIUS = 270.0f;   // 原 220.f → 稍微远一点，图标更疏
static constexpr float MENU_ARC_DEG = 145.0f;   // 原约 120°(2π/3) → 稍微更开一点
static constexpr float MENU_TILT_DEG = 35.0f;    // 整体向“右下”旋转 15°


// ★ 屏幕中心（倒计时固定居中）
static inline Object_2D::Vec2 UI_ScreenCenter() {
    // 若坐标系原点在左上角，请改成 { WindowWidth()*0.5f, WindowHeight()*0.5f }
    return { 0.f, 0.f };
}

// ★ 左侧“花销面板”相对主角的锚点（世界坐标系）
static inline Object_2D::Vec2 UI_CostPanelAnchorFromPlayer(const Object_2D::Vec2& playerCenter) {
    // 可按你的镜头/缩放调整 520 偏移
    return { playerCenter.x - 520.f, playerCenter.y };
}

// 两段式“弹出”
static float PopScale01(float p, float minS = 0.2f, float overS = 1.20f) {
    p = saturate(p);
    if (p < 0.7f) { float t = p / 0.7f; return minS + (overS - minS) * t; }
    float t = (p - 0.7f) / 0.3f; return overS + (1.0f - overS) * t;
}
// 关门：先微放大再缩没
static float PopScale01_Close(float p, float overS = 1.12f, float minS = 0.0f) {
    p = saturate(p);
    if (p < 0.3f) { float t = p / 0.3f; return 1.0f + (overS - 1.0f) * t; }
    float t = (p - 0.3f) / 0.7f; return overS + (minS - overS) * t;
}
// “爆裂→回正”脉冲
static float PulseBang(float p, float peak = 1.6f) {
    p = saturate(p);
    if (p < 0.25f) { float t = p / 0.25f; return 1.0f + (peak - 1.0f) * t; }
    float t = (p - 0.25f) / 0.75f; return peak + (1.0f - peak) * t;
}

// ====== 建筑动画状态 ======
enum class TileAnimKind { None, SpawnIn, UpgradeOut, UpgradeIn, Demolish };
struct TileAnim {
    TileAnimKind kind = TileAnimKind::None;
    float t = 0.f, dur = 0.f;
    RTS9::BuildingType prevType = RTS9::BuildingType::None;
    int prevLevel = 0;
};

// —— 状态缓存
static TileAnim g_tileAnims[RTS9::GRID_SIZE];
static RTS9::BuildingType g_prevType[RTS9::GRID_SIZE];
static int g_prevLevel[RTS9::GRID_SIZE];
static bool g_prevInited = false;

// —— 外部重置接口：模式切换时调用，秒清菜单状态
static bool g_resetMenuStateFlag = false;
void BuildUI_ResetMenuState() { g_resetMenuStateFlag = true; }

// —— “确认后先关菜单再跳转”支持
typedef void (*BuildUI_Callback)();
static BuildUI_Callback s_onClosedCB = nullptr;
void BuildUI_OnConfirmPressed(BuildUI_Callback afterClose) {
    s_onClosedCB = afterClose;
    g_resetMenuStateFlag = false; // 使用正常关闭动画
    gBuild.menuOpen = false;      // 走关闭流程
}

// —— 点击“专职”时触发额外爆裂动效
static float s_specializePulseT = -1.f;
void BuildUI_TriggerSpecializePulse() { s_specializePulseT = 0.f; }

// —— “进入基建后自动关闭一次菜单（带动画）”
static bool g_autoCloseOnce = false;
void BuildUI_RequestAutoCloseOnce() { g_autoCloseOnce = true; } // ★ 新增 API

// —— 倒计时→进入战斗 回调
static BuildUI_Callback s_enterBattleCB = nullptr;
void BuildUI_SetEnterBattleCallback(BuildUI_Callback fn) { s_enterBattleCB = fn; } // ★ 新增 API

// —— “开始战斗”按钮按下入口（外部调用）
void BuildUI_OnStartBattlePressed() { // ★ 新增 API
    // 先关菜单，等完全收回后，开始倒计时
    BuildUI_OnConfirmPressed(&BuildUI__BeginCountdown_CB);
}

// —— 快照
static void SnapshotPrevFrom(const RTS9::GameState& gs) {
    for (int i = 0; i < RTS9::GRID_SIZE; i++) {
        g_prevType[i] = gs.grid[i].b.type;
        g_prevLevel[i] = gs.grid[i].b.level;
        g_tileAnims[i] = TileAnim{};
    }
    g_prevInited = true;
}

// —— 兜底纹理（技术用途，不参与 UI）
static unsigned int GetFallbackTex() {
    static unsigned int cached = (unsigned int)(-1);
    if (cached != (unsigned int)(-1)) return cached;
    cached = 0;
    int* mapTex = GetMapTexturesID();
    if (mapTex && mapTex[0] != 0) { cached = (unsigned int)mapTex[0]; return cached; }
    Object_2D* p = GetPlayer();
    if (p && p->texNo != 0) { cached = p->texNo; return cached; }
    return cached;
}

// —— 带兜底的图标缓存（用于真正绘制）
static std::unordered_map<std::string, unsigned int> g_texCache;
static unsigned int LoadIconCached(const std::string& fname) {
    if (fname.empty()) return GetFallbackTex();
    auto it = g_texCache.find(fname);
    if (it != g_texCache.end()) return it->second;
    std::string path = "rom:/" + fname;
    unsigned int id = LoadTexture(path.c_str());
    if (id == 0) id = GetFallbackTex();
    g_texCache[fname] = id;
    return id;
}

// —— 不带兜底的“存在性探测”（只用于判断是否存在）
static std::unordered_map<std::string, unsigned int> g_texProbe;
static bool IconFileExists(const std::string& fname) {
    if (fname.empty()) return false;
    auto it = g_texProbe.find(fname);
    if (it != g_texProbe.end()) return it->second != 0;
    std::string path = "rom:/" + fname;
    unsigned int id = LoadTexture(path.c_str());
    g_texProbe[fname] = id;
    if (id != 0) g_texCache[fname] = id; // 共享缓存，避免重复加载
    return id != 0;
}

// === 新增：安全的“下一等级预览贴图”选择 ===
// === 新版：安全的“下一等级预览贴图”选择（只用 Building& 重载） ===
static std::string PreviewSpriteForNextLevel(RTS9::BuildingType ty, int curLevel) {
    // 上限卡死：金币屋/招募屋=3，其它=4（由 BuildSystem.h 提供）
    int cap = RTS9::MaxLevelForBuilding(ty);
    int nxt = curLevel + 1;
    if (nxt > cap) {
        // 已到顶：不给 4 级预览，直接回退到通用“升级牌”
        return std::string("zheshishengji.tga");
    }

    // 先尝试严格的下一等级
    {
        RTS9::Building b; b.type = ty; b.level = nxt;
        std::string fn = RTS9::SpriteForBuilding(b);
        if (IconFileExists(fn)) return fn;
    }

    // 回退：用当前等级（但不超过 cap），避免空贴图
    int backLv = curLevel <= cap ? curLevel : cap;
    if (backLv > 0) {
        RTS9::Building bb; bb.type = ty; bb.level = backLv;
        std::string back = RTS9::SpriteForBuilding(bb);
        if (IconFileExists(back)) return back;
    }

    // 最终兜底
    return std::string("zheshishengji.tga");
}



// === 数字绘制（支持 '0'-'9'、'/'、'+'） ===
#ifndef UI_NUM_HELPERS_DEFINED
#define UI_NUM_HELPERS_DEFINED
static unsigned int LoadDigitCached_UI(char ch) {
    static std::unordered_map<char, unsigned int> cache;
    auto it = cache.find(ch); if (it != cache.end()) return it->second;
    std::string path;
    if (ch == '/') path = "rom:/number/pie.tga";          // “/”
    else if (ch == '+') path = "rom:/number/plus.tga";    // “+”
    else if (ch >= '0' && ch <= '9') path = "rom:/number/" + std::string(1, ch) + ".tga";
    else { unsigned int fb = GetFallbackTex(); cache[ch] = fb; return fb; }
    unsigned int id = LoadTexture(path.c_str());
    if (id == 0) id = GetFallbackTex();
    cache[ch] = id;
    return id;
}
static float NumWidth_UI(const std::string& s, float h) {
    if (s.empty()) return 0.f;
    const float w = h * 0.62f, pad = h * 0.06f;
    return (w + pad) * (float)s.size() - pad;
}
static void DrawNum_UI(const std::string& s, const Object_2D::Vec2& center, float h, float alpha = 1.0f) {
    if (s.empty()) return;
    const float w = h * 0.62f, pad = h * 0.06f, totalW = NumWidth_UI(s, h);
    float x = center.x - totalW * 0.5f;
    for (char ch : s) {
        unsigned int tex = LoadDigitCached_UI(ch);
        if (tex) {
            Object_2D o(0, 0, w, h);
            o.Transform.Position = { x + w * 0.5f,center.y };
            o.Color = { 1,1,1,alpha };
            o.texNo = tex; BoxVertexsMake(&o); DrawObject_2D(&o);
        }
        x += (w + pad);
    }
}
#endif

static void DrawNum_UI_Color(const std::string& s, const Object_2D::Vec2& center, float h, float r, float g, float b, float a) {
    if (s.empty()) return;
    const float w = h * 0.62f, pad = h * 0.06f, totalW = NumWidth_UI(s, h);
    float x = center.x - totalW * 0.5f;
    for (char ch : s) {
        unsigned int tex = LoadDigitCached_UI(ch);
        if (tex) {
            Object_2D o(0, 0, w, h);
            o.Transform.Position = { x + w * 0.5f, center.y };
            o.Color = { r,g,b,a };
            o.texNo = tex; BoxVertexsMake(&o); DrawObject_2D(&o);
        }
        x += (w + pad);
    }
}

// —— 行列小行（图标 + “数字/数字”或“+数字/数字”）
#ifndef UI_ICON_ROW_HELPER_DEFINED
#define UI_ICON_ROW_HELPER_DEFINED
static void DrawIconNumRow_UI(const char* iconName, const std::string& text, const Object_2D::Vec2& leftBase, float iconH) {
    unsigned int itex = LoadIconCached(iconName);
    float iw = iconH;
    if (itex) {
        Object_2D ico(0, 0, iw, iconH);
        ico.Transform.Position = { leftBase.x + iw * 0.5f,leftBase.y };
        ico.Color = { 1,1,1,1 };
        ico.texNo = itex; BoxVertexsMake(&ico); DrawObject_2D(&ico);
    }
    const float gap = iconH * 0.35f, numH = iconH * 0.58f;
    float wNum = NumWidth_UI(text, numH);
    float cx = leftBase.x + iw + gap + wNum * 0.5f;
    DrawNum_UI(text, { cx,leftBase.y }, numH, 1.0f);
}
#endif

static void DrawIconNumRow_UI_Anim(const char* iconName, const std::string& text, const Object_2D::Vec2& leftBase, float iconH, float scale, float alpha) {
    unsigned int itex = LoadIconCached(iconName);
    float iw = iconH * scale, ih = iconH * scale;
    if (itex) {
        Object_2D ico(0, 0, iw, ih);
        ico.Transform.Position = { leftBase.x + iw * 0.5f,leftBase.y };
        ico.Color = { 1,1,1,alpha };
        ico.texNo = itex; BoxVertexsMake(&ico); DrawObject_2D(&ico);
    }
    const float gap = iconH * 0.35f * scale, numH = iconH * 0.58f * scale;
    float wNum = NumWidth_UI(text, numH);
    float cx = leftBase.x + iw + gap + wNum * 0.5f;
    DrawNum_UI(text, { cx,leftBase.y }, numH, alpha);
}

// —— 显示用：建造成本（用于“建造1级建筑”的金币）
#ifndef UI_BUILD_COST_TABLE_DEFINED
#define UI_BUILD_COST_TABLE_DEFINED
static void GetBuildCoinCostUI(RTS9::BuildingType ty, int& coinCost /*out*/) {
    switch (ty) {
    case RTS9::BuildingType::Gold:        coinCost = 0; break;
    case RTS9::BuildingType::Recruit:     coinCost = 0; break;
    case RTS9::BuildingType::MageHouse:   coinCost = 0; break;
    case RTS9::BuildingType::ShieldHouse: coinCost = 0; break;
    case RTS9::BuildingType::SaintHouse:  coinCost = 0; break;
    default: coinCost = 0; break;
    }
}
#endif

// ===================== 顶部“详情菜单”系统 =====================
// 资源名：zheshi*.tga（由用户提供）
static const char* ZHESHI_MAGE = "zheshimofashi.tga";
static const char* ZHESHI_SHIELD = "zheshidunbing.tga";
static const char* ZHESHI_SAINT = "zheshishengnv.tga";
static const char* ZHESHI_GOLD = "zheshijinbi.tga";
static const char* ZHESHI_RECRUIT = "zheshizhaomu.tga";
static const char* ZHESHI_DEMOLISH = "zheshichaichu.tga";
static const char* ZHESHI_BACK = "zheshifanhui.tga";
static const char* ZHESHI_UPGRADE = "zheshishengji.tga";
static const char* ZHESHI_SPEC = "zheshizhuanzhi.tga";
static const char* ZHESHI_BATTLE = "zheshikaishi.tga";

// 状态
static bool         s_detailVisible = false;
static bool         s_detailClosing = false;
static float        s_detailOpenT = 0.f;
static float        s_detailCloseT = 0.f;
static float        s_detailSwitchT = 2.f; // >dur 表示无切换
static std::string  s_detailCur, s_detailPrev;

// —— 判定：当前是否“建造1级建筑”（用于显示 1单位 花费）
static inline bool IsBuildLevel1Key(const std::string& key) {
    return key == ZHESHI_MAGE || key == ZHESHI_SHIELD || key == ZHESHI_SAINT || key == ZHESHI_GOLD || key == ZHESHI_RECRUIT;
}

// —— 从菜单项推断 zheshi*.tga 名（要用到上面的 MenuItemSnap）
static std::string DetermineDetailKey(const MenuItemSnap& snap, const RTS9::GameState& gs, int menuTile) {
    using A = RTS9::MenuAction;

    // 1) 动作 -> 详情卡片（固定名/或下一等级预览）
    switch (snap.action) {
    case A::Build_Mage:    return ZHESHI_MAGE;      // zheshimofashi.tga
    case A::Build_Shield:  return ZHESHI_SHIELD;    // zheshidunbing.tga
    case A::Build_Saint:   return ZHESHI_SAINT;     // zheshishengnv.tga
    case A::Build_Gold:    return ZHESHI_GOLD;      // zheshijinbi.tga
    case A::Build_Recruit: return ZHESHI_RECRUIT;   // zheshizhaomu.tga
    case A::Upgrade: {
        if (menuTile >= 0 && menuTile < RTS9::GRID_SIZE) {
            const auto& b = gs.grid[menuTile].b;
            return PreviewSpriteForNextLevel(b.type, b.level); // ★ 使用下一等级贴图
        }
        return ZHESHI_UPGRADE;
    }
    case A::Demolish:    return ZHESHI_DEMOLISH;  // zheshichaichu.tga
    case A::StartBattle: return ZHESHI_BATTLE;    // zheshikaishi.tga
    case A::Cancel:      return ZHESHI_BACK;      // zheshifanhui.tga
    case A::Train:
        return ZHESHI_SPEC;                       // zheshizhuanzhi.tga
    default:
        break;
    }

    // 2) 无动作命中时，用格子上建筑类型兜底
    if (menuTile >= 0 && menuTile < RTS9::GRID_SIZE) {
        auto ty = gs.grid[menuTile].b.type;
        switch (ty) {
        case RTS9::BuildingType::MageHouse:   return ZHESHI_MAGE;
        case RTS9::BuildingType::ShieldHouse: return ZHESHI_SHIELD;
        case RTS9::BuildingType::SaintHouse:  return ZHESHI_SAINT;
        case RTS9::BuildingType::Gold:        return ZHESHI_GOLD;
        case RTS9::BuildingType::Recruit:     return ZHESHI_RECRUIT;
        default: break;
        }
    }

    // 3) 最终兜底
    return ZHESHI_BACK;
}


// ===================== 花销面板（仅在：建造1级 / 升级 / 专职 / 拆卸） =====================
static const char* TEX_DEMOLISH = "chaichu.tga";
static const char* TEX_CANCEL = "cancel.tga";
static const char* TEX_ZHUANZHI = "zhuanzhi.tga";
static const char* TEX_TRAIN = "danwei.tga";
static const char* TEX_COIN_A = "qian.tga";
static const char* TEX_COIN_B = "coin.tga";
static const char* ChooseCoinIcon() {
    if (IconFileExists(TEX_COIN_A)) return TEX_COIN_A;
    if (IconFileExists(TEX_COIN_B)) return TEX_COIN_B;
    return TEX_COIN_A;
}

static bool IsActionShowCost(RTS9::MenuAction a) {
    using A = RTS9::MenuAction;
    switch (a) {
    case A::Build_Gold: case A::Build_Recruit: case A::Build_Mage:
    case A::Build_Shield: case A::Build_Saint:
    case A::Upgrade:
    case A::Train:
    case A::Demolish:
        return true;
    default:
        return false;
    }
}

static void ComputeCostForAction(const RTS9::GameState& gs, RTS9::MenuAction a, int menuTile,
    int& unitDelta, int& unitAfter,
    int& coinDelta, int& coinAfter)
{
    const int unitsHave = gs.res.units;
    const int coinsHave = gs.res.coins;
    unitDelta = 0; coinDelta = 0;

    using A = RTS9::MenuAction;

    if (a == A::Build_Gold || a == A::Build_Recruit || a == A::Build_Mage ||
        a == A::Build_Shield || a == A::Build_Saint)
    {
        // 1级建造：只花 1 单位（保持你目前规则）
        unitDelta = -1;
        coinDelta = 0;
    }
    else if (a == A::Upgrade)
    {
        int lvl = 0;
        if (menuTile >= 0 && menuTile < RTS9::GRID_SIZE) lvl = gs.grid[menuTile].b.level;
        if (lvl == 1) { coinDelta = -30; unitDelta = 0; }
        else if (lvl == 2) { coinDelta = -50; unitDelta = 0; }
        else if (lvl == 3) { coinDelta = -200; unitDelta = -20; } // ☆ 3->4
    }
    else if (a == A::Train)
    {
        unitDelta = -1;
        coinDelta = 0;
    }
    else if (a == A::Demolish)
    {
        int lvl = 0;
        if (menuTile >= 0 && menuTile < RTS9::GRID_SIZE) lvl = gs.grid[menuTile].b.level;
        if (lvl <= 1) { unitDelta = +1; coinDelta = 0; }
        else if (lvl == 2) { unitDelta = 0; coinDelta = +20; }
        else if (lvl == 3) { unitDelta = 0; coinDelta = +60; }
        else { unitDelta = 0; coinDelta = 0; } // Lv4 及以上未定义返还 => 0
    }

    unitAfter = unitsHave + unitDelta; if (unitAfter < 0) unitAfter = 0;
    coinAfter = coinsHave + coinDelta; if (coinAfter < 0) coinAfter = 0;
}


// —— 绘制花销面板（跟随主角，花费显示“X/剩余”，返还显示“+X/返后”）
static void DrawCostPanel(const RTS9::GameState& gs, RTS9::MenuAction a, const Object_2D::Vec2& playerCenter)
{
    if (!IsActionShowCost(a)) return;

    // 动效：轻呼吸 + 小摇摆
    static float t = 0.f; t += UI_dt();
    const float wobbleX = 6.0f * std::sinf(t * 1.8f);
    const float scale = 1.0f + 0.04f * std::sinf(t * 2.2f);

    Object_2D::Vec2 anchor = UI_CostPanelAnchorFromPlayer(playerCenter);
    anchor.x += wobbleX;

    // 计算 “Δ/After”
    int dUnit = 0, aUnit = 0, dCoin = 0, aCoin = 0;
    ComputeCostForAction(gs, a, gs.menuTileIndex, dUnit, aUnit, dCoin, aCoin);

    // 两行：单位 / 金币
    const float rowH = 80.f * scale;
    const float gapY = 46.f * scale;
    Object_2D::Vec2 left1{ anchor.x + 140.f * scale, anchor.y - gapY };
    Object_2D::Vec2 left2{ anchor.x + 140.f * scale, anchor.y + gapY };

    auto fmtDelta = [](int d) -> std::string {
        if (d >= 0) return std::string("+") + std::to_string(d); // 返还
        return std::to_string(-d); // 花费：以正数显示
        };

    std::string unitStr = fmtDelta(dUnit) + "/" + std::to_string(aUnit);
    std::string coinStr = fmtDelta(dCoin) + "/" + std::to_string(aCoin);

    DrawIconNumRow_UI_Anim("danwei.tga", unitStr, left1, rowH, /*scale*/1.0f, /*alpha*/1.0f);
    DrawIconNumRow_UI_Anim(ChooseCoinIcon(), coinStr, left2, rowH, /*scale*/1.0f, /*alpha*/1.0f);
}

// ===================== 建筑渲染（世界图标） =====================
static Object_2D::Vec2 TileCenterFromIndex(int idx) {
    Object_2D::Vec2* Box = GetBoxSzie();
    Object_2D::Vec2* MPos = GetMapPos();
    const float halfW = (MAP_WIDTH - 1) * 0.5f, halfH = (MAP_HEIGHT - 1) * 0.5f;
    const float stepX = Box->x * GetTileSpacingX();
    const float stepY = Box->y * GetTileSpacingY();
    int y = idx / MAP_WIDTH, x = idx % MAP_WIDTH;
    return { (x - halfW) * stepX + MPos->x, (y - halfH) * stepY + MPos->y };
}
static float TileIconSizeForLevel(int level) {
    Object_2D::Vec2* Box = GetBoxSzie();
    const float tileMin = (Box->x < Box->y ? Box->x : Box->y);
    float scale = 0.88f + ((level < 1 ? 1 : level) - 1) * 0.12f;
    if (scale > 1.0f) scale = 1.0f;
    return tileMin * scale;
}
static const char* DefaultIconForAction(RTS9::MenuAction a) {
    using A = RTS9::MenuAction;
    switch (a) {
    case A::Demolish:   return TEX_DEMOLISH;
    case A::Cancel:     return TEX_CANCEL;
    case A::Train:      return TEX_TRAIN;
    case A::StartBattle:return "battle.tga";
    default:            return "";
    }
}

void InitBuildUI() { g_texCache.clear(); g_texProbe.clear(); SnapshotPrevFrom(gBuild); }
void UninitBuildUI() { g_texCache.clear(); g_texProbe.clear(); g_prevInited = false; }

void DrawBuildWorld(const RTS9::GameState& gs) {
    if (!g_prevInited) SnapshotPrevFrom(gs);
    const float dt = UI_dt();

    for (int i = 0; i < RTS9::GRID_SIZE; i++) {
        const auto& curB = gs.grid[i].b;
        TileAnim& A = g_tileAnims[i];
        RTS9::BuildingType& PT = g_prevType[i];
        int& PL = g_prevLevel[i];

        if (A.kind == TileAnimKind::None) {
            bool prevHas = (PL > 0 && PT != RTS9::BuildingType::None);
            bool curHas = (curB.level > 0 && curB.type != RTS9::BuildingType::None);
            if (!prevHas && curHas) { A.kind = TileAnimKind::SpawnIn; A.t = 0.f; A.dur = 0.28f; }
            else if (prevHas && !curHas) { A.kind = TileAnimKind::Demolish; A.t = 0.f; A.dur = 0.30f; A.prevType = PT; A.prevLevel = PL; }
            else if (prevHas && curHas && (curB.type != PT || curB.level > PL)) { A.kind = TileAnimKind::UpgradeOut; A.t = 0.f; A.dur = 0.26f; A.prevType = PT; A.prevLevel = PL; }
        }
        PT = curB.type; PL = curB.level;

        Object_2D::Vec2 center = TileCenterFromIndex(i);
        auto DrawOne = [&](RTS9::BuildingType ty, int lvl, float scale, float alpha) {
            if (lvl <= 0 || ty == RTS9::BuildingType::None) return;
            std::string fn = RTS9::SpriteForBuilding(ty, lvl);
            unsigned int tex = LoadIconCached(fn); if (!tex) return;
            float base = TileIconSizeForLevel(lvl), sz = base * scale;
            Object_2D icon(0, 0, sz, sz);
            icon.Transform.Position = center; icon.texNo = tex; icon.Color = { 1,1,1,alpha };
            BoxVertexsMake(&icon); DrawObject_2D(&icon);
            };

        if (A.kind == TileAnimKind::None) {
            if (curB.level > 0 && curB.type != RTS9::BuildingType::None) DrawOne(curB.type, curB.level, 1.0f, 1.0f);
        }
        else {
            A.t += dt; float p = saturate(A.t / A.dur);
            if (A.kind == TileAnimKind::SpawnIn) {
                DrawOne(curB.type, curB.level, PopScale01(p, 0.18f, 1.28f), p);
                if (A.t >= A.dur) A = TileAnim{};
            }
            else if (A.kind == TileAnimKind::Demolish) {
                float s = (p < 0.5f) ? (1.0f + 0.28f * (p / 0.5f)) : (1.28f * (1.0f - (p - 0.5f) / 0.5f));
                float a = 1.0f - p; DrawOne(A.prevType, A.prevLevel, s, a);
                if (A.t >= A.dur) A = TileAnim{};
            }
            else if (A.kind == TileAnimKind::UpgradeOut) {
                float s = (p < 0.5f) ? (1.0f + 0.28f * (p / 0.5f)) : (1.28f * (1.0f - (p - 0.5f) / 0.5f));
                float a = 1.0f - p; DrawOne(A.prevType, A.prevLevel, s, a);
                if (A.t >= A.dur) { A.kind = TileAnimKind::UpgradeIn; A.t = 0.f; A.dur = 0.28f; }
            }
            else if (A.kind == TileAnimKind::UpgradeIn) {
                DrawOne(curB.type, curB.level, PopScale01(p, 0.18f, 1.28f), p);
                if (A.t >= A.dur) A = TileAnim{};
            }
        }
    }

    // === 战斗入口图标（轻微呼吸，无背景）
    {
        Object_2D::Vec2* Box = GetBoxSzie(); Object_2D::Vec2* MPos = GetMapPos();
        const float halfW = (MAP_WIDTH - 1) * 0.5f;
        const float stepX = Box->x * GetTileSpacingX();
        Object_2D::Vec2 center{ (1 - halfW) * stepX + MPos->x, MPos->y };
        Object_2D::Vec2 battleC{ center.x + stepX * 3.0f, center.y };
        unsigned int tex = LoadIconCached("battle.tga");
        if (tex) {
            float base = TileIconSizeForLevel(1);
            static float tBreath = 0.f; tBreath += UI_dt();
            float s = 1.0f + 0.04f * std::sinf(tBreath * 2.6f);
            Object_2D icon(0, 0, base * s, base * s);
            icon.Transform.Position = battleC; icon.texNo = tex; icon.Color = { 1,1,1,0.95f };
            BoxVertexsMake(&icon); DrawObject_2D(&icon);
        }
    }
}


// ===================== 321 倒计时系统（修正版：固定 UI 白贴图 & 禁止 FX 回退到地块） =====================
static unsigned int TryTex(const char* name) { return LoadIconCached(name); }

// 固定用于 UI 的纯白贴图（优先尝试 ui_white.tga；没有就退回系统白贴图）
static unsigned int TEX_WHITE() {
    static unsigned int id = TryTex("ui_white.tga");
    return id ? id : GetFallbackTex();
}

// FX 贴图：缺失时不再回退到“兜底贴图”（否则可能把地块混进来）
static unsigned int TEX_RING() { static unsigned int id = TryTex("fx_ring.tga");  return id; }
static unsigned int TEX_FLARE() { static unsigned int id = TryTex("fx_flare.tga"); return id; }
static unsigned int TEX_SPARK() { static unsigned int id = TryTex("fx_spark.tga"); return id; }

struct FXParticle { Object_2D::Vec2 pos, vel; float life = 1.f, age = 0.f, size = 20.f, rot = 0.f, rotVel = 0.f; float r = 1.f, g = 1.f, b = 1.f, a = 1.f; };
static std::vector<FXParticle> s_fxParticles;

struct FXRing { Object_2D::Vec2 pos; float t = 0.f, dur = 0.8f; float r0 = 80.f, r1 = 720.f; float a0 = 0.8f; };
static std::vector<FXRing> s_fxRings;

struct DigitGhost { float h = 300.f, alpha = 0.5f, offsetX = 0.f, offsetY = 0.f; float r = 1.f, g = 1.f, b = 1.f; };
static std::vector<DigitGhost> s_digitTrail;

static bool  s_countdownActive = false;
static float s_countdownT = 0.f;     // [0,3]
static int   s_currentNum = 3;       // 3/2/1
static int   s_prevSlot = 4;         // 用于边界检测
static float s_jitterT = 0.f;        // 镜头微抖

static bool BuildUI_IsBattleCountdownActive() { return s_countdownActive; }

static void SpawnBurstFor(int num, const Object_2D::Vec2& center) {
    for (int k = 0; k < 2; k++) {
        FXRing ring; ring.pos = center; ring.t = 0.f; ring.dur = 0.70f + 0.15f * k;
        ring.r0 = 90.f + 20.f * k; ring.r1 = 820.f + 60.f * k; ring.a0 = 0.65f - 0.12f * k;
        s_fxRings.push_back(ring);
    }
    int n = 120 + num * 30;
    for (int i = 0; i < n; i++) {
        float ang = frand(0.f, 6.28318f);
        float spd = frand(220.f, 720.f);
        FXParticle p;
        p.pos = center; p.vel = { std::cos(ang) * spd, std::sin(ang) * spd };
        p.life = frand(0.6f, 1.0f);
        p.size = frand(12.f, 26.f) * (1.0f + 0.3f * num / 3.f);
        p.rot = frand(0.f, 6.28f); p.rotVel = frand(-6.f, 6.f);
        if (num == 3) { p.r = 1.f; p.g = frand(0.3f, 0.6f); p.b = 0.2f; }
        else if (num == 2) { p.r = 1.f; p.g = 0.9f; p.b = frand(0.2f, 0.5f); }
        else { p.r = 0.2f; p.g = 0.9f; p.b = 1.0f; }
        s_fxParticles.push_back(p);
    }
    s_digitTrail.clear();
    for (int i = 0; i < 8; i++) {
        DigitGhost g;
        g.h = 340.f + i * 10.f;
        g.alpha = 0.20f * (1.0f - i / 8.f);
        g.offsetX = frand(-12.f, 12.f);
        g.offsetY = frand(-8.f, 8.f);
        if (num == 3) { g.r = 1.f; g.g = 0.45f; g.b = 0.25f; }
        else if (num == 2) { g.r = 1.f; g.g = 0.95f; g.b = 0.4f; }
        else { g.r = 0.3f; g.g = 0.95f; g.b = 1.0f; }
        s_digitTrail.push_back(g);
    }
    s_jitterT = 0.30f;
}

static void UpdateFX(const Object_2D::Vec2& center) {
    float dt = UI_dt();
    for (auto& p : s_fxParticles) {
        p.age += dt; if (p.age > p.life) continue;
        p.vel.x *= (1.f - 0.8f * dt);
        p.vel.y = p.vel.y * (1.f - 0.8f * dt) + 420.f * dt;
        p.pos.x += p.vel.x * dt; p.pos.y += p.vel.y * dt;
        p.rot += p.rotVel * dt;
    }
    size_t w = 0; for (size_t i = 0; i < s_fxParticles.size(); ++i) {
        if (s_fxParticles[i].age <= s_fxParticles[i].life) s_fxParticles[w++] = s_fxParticles[i];
    }
    s_fxParticles.resize(w);

    for (auto& r : s_fxRings) { r.t += dt; }
    w = 0; for (size_t i = 0; i < s_fxRings.size(); ++i) {
        if (s_fxRings[i].t <= s_fxRings[i].dur) s_fxRings[w++] = s_fxRings[i];
    }
    s_fxRings.resize(w);

    for (auto& g : s_digitTrail) g.alpha *= (1.f - 0.9f * dt);
    if (s_jitterT > 0.f) { s_jitterT -= dt; if (s_jitterT < 0.f) s_jitterT = 0.f; }
}

static void DrawFX_Layers(const Object_2D::Vec2& center) {
    // 背景暗角：强制使用 UI 白贴图，避免“地块”作为兜底贴图混进来
    {
        float t = s_countdownT;
        float a = 0.18f + 0.05f * std::sinf(t * 6.0f);
        unsigned int tex = TEX_WHITE();
        Object_2D quad(0, 0, 4000.f, 2200.f);
        quad.Transform.Position = center;
        quad.Color = { 0.f,0.f,0.f,a };
        quad.texNo = tex; BoxVertexsMake(&quad); DrawObject_2D(&quad);
    }

    // 扩散光环：贴图缺失就跳过
    for (const auto& r : s_fxRings) {
        float p = saturate(r.t / r.dur);
        float rad = r.r0 + (r.r1 - r.r0) * p;
        float a = r.a0 * (1.f - p);
        unsigned int tex = TEX_RING();
        if (!tex) continue;
        Object_2D o(0, 0, rad * 2.f, rad * 2.f);
        o.Transform.Position = r.pos;
        o.Color = { 1.f,1.f,1.f,a };
        o.texNo = tex; BoxVertexsMake(&o); DrawObject_2D(&o);
    }

    // 粒子：贴图缺失就跳过
    {
        unsigned int tex = TEX_SPARK();
        if (tex) {
            for (const auto& p : s_fxParticles) {
                float k = 1.f - saturate(p.age / p.life);
                float a = p.a * k;
                Object_2D o(0, 0, p.size * (0.6f + 0.8f * k), p.size * (0.6f + 0.8f * k));
                o.Transform.Position = p.pos;
                o.Color = { p.r, p.g, p.b, a };
                o.texNo = tex; BoxVertexsMake(&o); DrawObject_2D(&o);
            }
        }
    }

    // 光晕和十字辉：flare 缺失就跳过；白条强制用 UI 白贴图
    {
        unsigned int flare = TEX_FLARE();
        if (flare) {
            for (int i = 0; i < 2; i++) {
                float s = (i == 0) ? 900.f : 600.f;
                float a = (i == 0) ? 0.25f : 0.35f;
                Object_2D o(0, 0, s, s);
                o.Transform.Position = UI_ScreenCenter();
                o.Color = { 1.f,1.f,1.f,a };
                o.texNo = flare; BoxVertexsMake(&o); DrawObject_2D(&o);
            }
        }
        unsigned int white = TEX_WHITE();
        Object_2D v(0, 0, 60.f, 1600.f);
        v.Transform.Position = UI_ScreenCenter(); v.Color = { 1.f,1.f,1.f,0.18f };
        v.texNo = white; BoxVertexsMake(&v); DrawObject_2D(&v);
        Object_2D h(0, 0, 1600.f, 60.f);
        h.Transform.Position = UI_ScreenCenter(); h.Color = { 1.f,1.f,1.f,0.18f };
        h.texNo = white; BoxVertexsMake(&h); DrawObject_2D(&h);
    }
}

static void DrawDigitLayered(int num, const Object_2D::Vec2& baseCenter) {
    Object_2D::Vec2 center = baseCenter;
    if (s_jitterT > 0.f) {
        center.x += frand(-6.f, 6.f) * (s_jitterT / 0.30f);
        center.y += frand(-4.f, 4.f) * (s_jitterT / 0.30f);
    }
    float t = s_countdownT - (3 - num);
    t = t < 0.f ? 0.f : (t > 1.f ? 1.f : t);
    float H = 520.f * PulseBang(saturate(t / 1.0f), 1.45f);
    for (size_t i = 0; i < s_digitTrail.size(); ++i) {
        const auto& g = s_digitTrail[i];
        DrawNum_UI_Color(std::to_string(num), { center.x + g.offsetX, center.y + g.offsetY }, g.h, g.r, g.g, g.b, g.alpha);
    }
    float off = 10.f * (1.f - t);
    DrawNum_UI_Color(std::to_string(num), { center.x - off, center.y }, H, 1.f, 0.2f, 0.2f, 0.40f);
    DrawNum_UI_Color(std::to_string(num), { center.x + off, center.y }, H, 0.2f, 1.f, 0.2f, 0.40f);
    DrawNum_UI_Color(std::to_string(num), { center.x, center.y - off }, H, 0.2f, 0.6f, 1.f, 0.40f);
    DrawNum_UI_Color(std::to_string(num), center, H, 1.f, 1.f, 1.f, 1.0f);
}

static void Countdown_Update(const Object_2D::Vec2& screenCenter) {
    if (!s_countdownActive) return;
    s_countdownT += UI_dt(); if (s_countdownT > 3.0f) s_countdownT = 3.0f;
    int slot = 3 - (int)std::floor(s_countdownT);
    if (slot < 1) slot = 1;
    if (slot != s_prevSlot) {
        s_currentNum = slot;
        SpawnBurstFor(s_currentNum, screenCenter);
        s_prevSlot = slot;
    }
    UpdateFX(screenCenter);
    if (s_countdownT >= 3.0f) {
        s_countdownActive = false;
        s_fxParticles.clear();
        s_fxRings.clear();
        s_digitTrail.clear();
        if (s_enterBattleCB) s_enterBattleCB();
    }
}

static void Countdown_Draw(const Object_2D::Vec2& screenCenter) {
    if (!s_countdownActive) return;
    DrawFX_Layers(screenCenter);
    DrawDigitLayered(s_currentNum, screenCenter);
}

void BuildUI_DrawBattleCountdownOverlay(bool doUpdate) {
    if (!s_countdownActive) return;
    Object_2D::Vec2 center = UI_ScreenCenter();
    if (doUpdate) Countdown_Update(center);
    Countdown_Draw(center);
}

// === 仅更新时间轴，不做任何绘制（供 Update 调用） ===
void BuildUI_UpdateOnlyCountdown() {
    if (!BuildUI_IsBattleCountdownActive()) return;
    Object_2D::Vec2 center = UI_ScreenCenter();

    s_countdownT += UI_dt();
    if (s_countdownT > 3.0f) s_countdownT = 3.0f;

    int slot = 3 - (int)std::floor(s_countdownT);
    if (slot < 1) slot = 1;

    if (slot != s_prevSlot) {
        s_currentNum = slot;
        SpawnBurstFor(s_currentNum, center);
        s_prevSlot = slot;
    }

    UpdateFX(center);

    if (s_countdownT >= 3.0f) {
        s_countdownActive = false;
        s_fxParticles.clear();
        s_fxRings.clear();
        s_digitTrail.clear();
        if (s_enterBattleCB) s_enterBattleCB();
    }
}

static void BuildUI__BeginCountdown_CB() {
    s_countdownActive = true;
    s_countdownT = 0.f;
    s_prevSlot = 4;
    s_currentNum = 3;
    s_fxParticles.clear();
    s_fxRings.clear();
    s_digitTrail.clear();

    gBuild.menuOpen = false;
    gBuild.currentMenu.clear();

    SpawnBurstFor(3, UI_ScreenCenter());
}


// ===================== 扇形菜单快照（无背景；严格顺序关闭） =====================
static void MakeMenuSnapshot(const RTS9::GameState& gs, std::vector<MenuItemSnap>& dst) {
    dst.clear();
    using A = RTS9::MenuAction;

    int menuTile = (gs.menuTileIndex >= 0 && gs.menuTileIndex < RTS9::GRID_SIZE) ? gs.menuTileIndex : -1;
    if (menuTile < 0) { for (int i = 0; i < RTS9::GRID_SIZE; i++) { if (gs.playerStanding[i]) { menuTile = i; break; } } }
    const bool tileOK = (menuTile >= 0 && menuTile < RTS9::GRID_SIZE);
    const auto* tile = tileOK ? &gs.grid[menuTile] : nullptr;
    const bool isMageOrShield = (tile && (tile->b.type == RTS9::BuildingType::MageHouse ||
        tile->b.type == RTS9::BuildingType::ShieldHouse));

    int trainTotal = 0; for (const auto& e : gs.currentMenu) if (e.action == A::Train) ++trainTotal;
    int trainSeen = 0;

    for (size_t i = 0; i < gs.currentMenu.size(); ++i) {
        const auto& e = gs.currentMenu[i];
        std::string iconName = e.icon;

        if (iconName.empty() && e.action == A::Upgrade && tile) {
            iconName = PreviewSpriteForNextLevel(tile->b.type, tile->b.level);
        }

        if (e.action == A::Train) {
            int nextTrainIndex = trainSeen + 1;
            bool shouldSpecialize = isMageOrShield && ((trainTotal >= 2) ? (nextTrainIndex == 2) : (nextTrainIndex == 1));
            if (iconName.empty() && shouldSpecialize && tile) {
                iconName = ChooseSpecializeUnitIcon(tile->b.type, tile->b.level);
            }
            if (iconName.empty()) iconName = "danwei.tga";
        }

        if (iconName.empty()) {
            const char* fb = DefaultIconForAction(e.action);
            if (fb && *fb) iconName = fb;
        }

        if (e.action == A::Train) ++trainSeen;
        dst.push_back(MenuItemSnap{ iconName, e.action });
    }
}


// ===================== 扇形菜单绘制（含详情HUD & 花销面板） =====================
void DrawBuildMenu(const RTS9::GameState& gs) {
    if (BuildUI_IsBattleCountdownActive()) return;

    static bool  s_prevOpen = false;  static float s_openT = 0.f;
    static bool  s_closing = false;   static float s_closeT = 0.f;
    static std::vector<MenuItemSnap> s_lastMenu;
    static Object_2D::Vec2 s_closeCenter{ 0,0 };
    static bool  s_switching = false; static float s_switchT = 0.f;
    static float s_animClock = 0.f;
    static float s_closedIdleT = 0.f;
    static int   s_closeIndex = 0;

    // 外部强制重置
    if (g_resetMenuStateFlag) {
        s_prevOpen = false; s_openT = 0.f;
        s_closing = false;  s_closeT = 0.f;
        s_switching = false; s_switchT = 0.f;
        s_lastMenu.clear();
        s_closedIdleT = 0.f;
        s_closeIndex = 0;
        s_onClosedCB = nullptr;
        s_specializePulseT = -1.f;
        g_resetMenuStateFlag = false;
        g_autoCloseOnce = false;

        // 同步重置详情HUD
        s_detailVisible = false; s_detailClosing = false;
        s_detailOpenT = 0.f; s_detailCloseT = 0.f; s_detailSwitchT = 2.f;
        s_detailCur.clear(); s_detailPrev.clear();
    }

    if (s_specializePulseT >= 0.f) { s_specializePulseT += UI_dt(); if (s_specializePulseT > 0.30f) s_specializePulseT = -1.f; }

    s_animClock += UI_dt();
    bool openingNow = (gs.menuOpen && !gs.currentMenu.empty());
    Object_2D* p = GetPlayer();
    Object_2D::Vec2 center = p ? p->Transform.Position : Object_2D::Vec2{ 0,0 };

    // 自动关闭一次
    if (g_autoCloseOnce && !s_closing) {
        if (s_lastMenu.empty() && !gs.currentMenu.empty()) {
            MakeMenuSnapshot(gs, s_lastMenu);
        }
        if (!s_lastMenu.empty()) {
            s_prevOpen = true;
            s_closing = true; s_closeT = 0.f;
            s_switching = false; s_closeIndex = 0;
            s_closeCenter = center;
            gBuild.menuOpen = false;
        }
        else {
            g_autoCloseOnce = false;
        }
    }

    // 开/关状态切换
    if (openingNow) {
        s_closedIdleT = 0.f;
        if (!s_prevOpen) {
            s_openT = 0.f; s_closing = false; s_switching = false; s_closeIndex = 0;
        }
        else {
            bool changed = (s_lastMenu.size() != gs.currentMenu.size());
            if (!changed) {
                for (size_t i = 0; i < gs.currentMenu.size(); ++i) {
                    if (s_lastMenu[i].action != gs.currentMenu[i].action) { changed = true; break; }
                }
            }
            if (changed && !s_switching) {
                s_switching = true; s_switchT = 0.f; s_closeCenter = center; s_closeIndex = 0;
            }
        }
        s_prevOpen = true; s_openT += UI_dt();
    }
    else {
        if (s_prevOpen) {
            s_prevOpen = false; s_closing = true; s_switching = false; s_closeT = 0.f; s_closeCenter = center; s_closeIndex = 0;
        }
        if (!s_closing && !s_switching) {
            s_closedIdleT += UI_dt();
            if (s_closedIdleT > 0.25f) { s_lastMenu.clear(); s_closedIdleT = 0.25f; }
        }
    }

    // 严格顺序收回（切换/关闭共用）
    auto DrawCloseSequence = [&](float& tClock, float dur, bool isSwitching)->bool {
        const int M = (int)s_lastMenu.size();
        if (M <= 0) return true;
        const float radius = MENU_RADIUS;
        const float arc = Deg2Rad(MENU_ARC_DEG);
        const float start = -arc * 0.5f + Deg2Rad(MENU_TILT_DEG); // 整体右下微倾

        float p_ = saturate(tClock / dur);

        int i = M - 1 - s_closeIndex;
        if (i < 0) return true;

        float t = (M == 1) ? 0.f : (float)i / (M - 1);
        float ang = start + arc * t;
        Object_2D::Vec2 pos{ s_closeCenter.x + radius * std::cos(ang),
                             s_closeCenter.y + radius * std::sin(ang) };

        float sc = PopScale01_Close(p_, isSwitching ? 1.10f : 1.12f, 0.0f);
        float alpha = 1.0f - p_;
        float baseS = 150.f;
        float sz = baseS * sc;

        std::string iconName = s_lastMenu[i].iconName;
        if (iconName.empty()) {
            const char* fb = DefaultIconForAction(s_lastMenu[i].action);
            if (fb && *fb) iconName = fb;
        }
        unsigned int tex = LoadIconCached(iconName);
        if (tex) {
            Object_2D it(0, 0, sz, sz);
            it.Transform.Position = pos; it.texNo = tex; it.Color = { 1,1,1,alpha };
            BoxVertexsMake(&it); DrawObject_2D(&it);
        }

        if (p_ >= 1.0f) { tClock = 0.f; ++s_closeIndex; }
        return (s_closeIndex >= M);
        };

    if (openingNow && s_switching) {
        s_switchT += UI_dt();
        bool finished = DrawCloseSequence(s_switchT, /*dur*/0.03f, /*switch*/true);
        if (!finished) return;
        s_switching = false; s_openT = 0.f;
    }

    if (!openingNow && s_closing) {
        s_closeT += UI_dt();
        if (s_detailVisible && !s_detailClosing) { s_detailClosing = true; s_detailCloseT = 0.f; }

        bool finished = DrawCloseSequence(s_closeT, /*dur*/0.07f, /*switch*/false);
        if (!finished) {
            BuildUI_DrawBattleCountdownOverlay(false);
            if (s_detailClosing) {
                s_detailCloseT += UI_dt();
                if (s_detailCloseT >= 0.18f) { s_detailClosing = false; s_detailVisible = false; s_detailCur.clear(); s_detailPrev.clear(); }
            }
            return;
        }
        s_closing = false;
        s_lastMenu.clear();
        if (g_autoCloseOnce) g_autoCloseOnce = false;
        if (s_onClosedCB) { BuildUI_Callback cb = s_onClosedCB; s_onClosedCB = nullptr; cb(); }
        s_detailClosing = false; s_detailVisible = false; s_detailCur.clear(); s_detailPrev.clear();
        return;
    }

    // —— 开启态绘制（扇形）
    const int   N = (int)gs.currentMenu.size();
    const float radius = MENU_RADIUS;
    const float arc = Deg2Rad(MENU_ARC_DEG);
    const float start = -arc * 0.5f + Deg2Rad(MENU_TILT_DEG);


    int menuTile = (gs.menuTileIndex >= 0 && gs.menuTileIndex < RTS9::GRID_SIZE) ? gs.menuTileIndex : -1;
    if (menuTile < 0) { for (int i = 0; i < RTS9::GRID_SIZE; i++) { if (gs.playerStanding[i]) { menuTile = i; break; } } }
    const bool tileOK = (menuTile >= 0 && menuTile < RTS9::GRID_SIZE);
    const auto* tile = tileOK ? &gs.grid[menuTile] : nullptr;
    const bool isMageOrShield = (tile && (tile->b.type == RTS9::BuildingType::MageHouse ||
        tile->b.type == RTS9::BuildingType::ShieldHouse));

    using A = RTS9::MenuAction;
    int trainTotal = 0; for (const auto& e : gs.currentMenu) if (e.action == A::Train) ++trainTotal;
    int trainSeen = 0;

    std::vector<MenuItemSnap> curSnap; curSnap.reserve(N);

    int selectedIdx = (N > 0 ? gs.selectedIdx : -1);
    if (selectedIdx < 0 || selectedIdx >= N) selectedIdx = 0;

    for (int i = 0; i < N; i++) {
        float t = (N == 1) ? 0.f : (float)i / (N - 1);
        float ang = start + arc * t;
        Object_2D::Vec2 pos{ center.x + radius * std::cos(ang),
                             center.y + radius * std::sin(ang) };

        const auto& e = gs.currentMenu[i];
        bool sel = (i == selectedIdx);

        std::string iconName = e.icon;
        if (iconName.empty() && e.action == A::Upgrade && tile) {
            iconName = PreviewSpriteForNextLevel(tile->b.type, tile->b.level); // ★ 下一等级预览
        }
        if (e.action == A::Train) {
            int nextTrainIndex = trainSeen + 1;
            bool shouldSpecialize = isMageOrShield && ((trainTotal >= 2) ? (nextTrainIndex == 2) : (nextTrainIndex == 1));
            if (iconName.empty() && shouldSpecialize && tile)
                iconName = ChooseSpecializeUnitIcon(tile->b.type, tile->b.level);
            if (iconName.empty()) iconName = TEX_TRAIN;
        }
        if (iconName.empty()) {
            const char* fb = DefaultIconForAction(e.action);
            if (fb && *fb) iconName = fb;
        }
        if (e.action == A::Train) ++trainSeen;

        curSnap.push_back(MenuItemSnap{ iconName, e.action });

        const float delayPer = 0.06f, inDur = 0.22f;
        float pIn = saturate((s_openT - i * delayPer) / inDur);
        float pop = PopScale01(pIn, 0.65f, 1.28f);

        float springAmpAll = 0.015f, springFreq = 8.0f;
        float spring = 1.0f + springAmpAll * std::sinf((s_animClock + i * 0.15f) * springFreq) * std::exp(-3.0f * (1.0f - pIn));
        float alpha = pIn;

        float baseS = sel ? 180.f : 150.f;
        float sz = baseS * pop * spring;

        if (alpha > 0.01f) {
            if (sel) {
                float wobble = 22.0f * std::sinf(s_animClock * 2.8f + i * 0.6f);
                float pulse = 1.5f + 0.27f * std::sinf(s_animClock * 3.0f + i * 1.1f);
                pos.x += wobble; sz *= pulse;
            }
            else {
                float wobble = 6.0f * std::sinf(s_animClock * 1.6f + i * 0.5f);
                float pulse = 1.0f + 0.03f * std::sinf(s_animClock * 1.8f + i * 0.9f);
                pos.x += wobble; sz *= pulse;
            }
        }

        unsigned int tex = LoadIconCached(iconName);
        if (tex) {
            Object_2D it(0, 0, sz, sz);
            it.Transform.Position = pos; it.texNo = tex; it.Color = { 1,1,1, alpha * (sel ? 1.0f : 0.95f) };
            BoxVertexsMake(&it); DrawObject_2D(&it);
        }
    }

    // === 顶部“详情菜单” ===
    MenuItemSnap selSnap = curSnap.empty() ? MenuItemSnap{} : curSnap[selectedIdx];
    if (menuTile < 0) { for (int i = 0; i < RTS9::GRID_SIZE; i++) { if (gs.playerStanding[i]) { menuTile = i; break; } } }
    std::string curKey = DetermineDetailKey(selSnap, gs, menuTile);

    const float headYOffset = 260.f; // 若方向相反改为 -260.f
    Object_2D::Vec2 headCenter{ center.x, center.y - headYOffset };

    if (openingNow) {
        if (!s_detailVisible) {
            s_detailVisible = true; s_detailClosing = false;
            s_detailOpenT = 0.f; s_detailCloseT = 0.f;
            s_detailCur = curKey; s_detailPrev.clear(); s_detailSwitchT = 2.f;
        }
        else if (curKey != s_detailCur) {
            s_detailPrev = s_detailCur; s_detailCur = curKey; s_detailSwitchT = 0.f;
        }
        s_detailOpenT += UI_dt();
        if (s_detailSwitchT <= 1.0f) s_detailSwitchT += UI_dt();
    }
    else {
        if (s_detailVisible && !s_detailClosing) { s_detailClosing = true; s_detailCloseT = 0.f; }
        if (s_detailClosing) {
            s_detailCloseT += UI_dt();
            if (s_detailCloseT >= 0.18f) { s_detailClosing = false; s_detailVisible = false; s_detailCur.clear(); s_detailPrev.clear(); }
        }
    }

    if (s_detailVisible || s_detailClosing) {
        // —— 头顶详情卡片绘制（含开合/切换动画）
        auto DrawDetailHUD = [&](const Object_2D::Vec2& headCenter_, const std::string& key, bool showCost1Unit) {
            const float openDur = 0.22f;
            const float closeDur = 0.18f;
            const float switchDur = 0.22f;

            float inP = saturate(s_detailOpenT / openDur);
            float outP = saturate(s_detailCloseT / closeDur);
            bool closing = s_detailClosing;

            float baseScale = closing ? PopScale01_Close(outP, 1.08f, 0.0f)
                : PopScale01(inP, 0.60f, 1.10f);
            float baseAlpha = closing ? (1.0f - outP) : inP;

            auto drawOne = [&](const std::string& fname, float aMul, float sMul) {
                unsigned int tex = LoadIconCached(fname);
                if (!tex) return;
                float W = 340.f * sMul * baseScale;
                float H = 240.f * sMul * baseScale;
                Object_2D o(0, 0, W, H);
                o.Transform.Position = headCenter_;
                o.Color = { 1.f, 1.f, 1.f, baseAlpha * aMul };
                o.texNo = tex; BoxVertexsMake(&o); DrawObject_2D(&o);
                };

            // 切换过渡：旧淡出→新淡入
            if (!s_detailPrev.empty() && s_detailSwitchT <= 1.0f) {
                float p = saturate(s_detailSwitchT / switchDur);
                drawOne(s_detailPrev, 1.0f - p, 1.0f - 0.08f * p);
                drawOne(s_detailCur, p, 0.92f + 0.08f * p);
            }
            else {
                drawOne(s_detailCur, 1.0f, 1.0f);
            }

            // 建造1级时在牌面角落提示“1单位”
            if (showCost1Unit) {
                unsigned int unitTex = LoadIconCached("danwei.tga");
                float ih = 60.f * baseScale, iw = ih;
                Object_2D ico(0, 0, iw, ih);
                ico.Transform.Position = { headCenter_.x + 200.f * baseScale, headCenter_.y + 60.f * baseScale };
                ico.Color = { 1,1,1, baseAlpha };
                ico.texNo = unitTex; BoxVertexsMake(&ico); DrawObject_2D(&ico);

                DrawNum_UI("1", { ico.Transform.Position.x + iw * 0.5f + 36.f * baseScale,
                                  ico.Transform.Position.y }, ih * 0.8f, baseAlpha);
            }
            };

        // 直接禁用详情卡片角落的“1单位”徽章
        DrawDetailHUD(headCenter, s_detailCur, /*showCost1Unit=*/false);

    }

    // ★ 花销面板：仅在（建造1级/升级/专职/拆卸）显示，且跟随主角
    if (!curSnap.empty()) {
        DrawCostPanel(gs, selSnap.action, center);
    }

    // —— 保存快照
    s_lastMenu = curSnap;
}


// 顶层绘制入口（菜单 + 倒计时只绘制）——放在所有世界/普通HUD之后再画
extern void Polygon_Draw(void); // 2D管线绑定（sprite.cpp）

void BuildUI_DrawTopMost(const RTS9::GameState& gs) {
    Polygon_Draw();                 // 防止被3D深度影响
    DrawBuildMenu(gs);              // 菜单/详情/花销面板（顶层）
    BuildUI_DrawBattleCountdownOverlay(false); // 倒计时【只画不更时钟】
}


// ★★ 依据建筑类型与等级挑“专职/训练”图标；不存在就回退到通用图标（返回 std::string）
static std::string ChooseSpecializeUnitIcon(RTS9::BuildingType ty, int level) {
    if (level < 1) level = 1; // 保底
    // 1) 魔法师屋：magicman<lv>.tga
    if (ty == RTS9::BuildingType::MageHouse) {
        std::string f = "magicman" + std::to_string(level) + ".tga";
        if (IconFileExists(f)) return f;
        if (IconFileExists("mofashi.tga")) return "mofashi.tga";
    }
    // 2) 盾兵屋：优先 shieldman<lv>.tga，再兼容 sheldman<lv>.tga
    if (ty == RTS9::BuildingType::ShieldHouse) {
        std::string f1 = "sheldman" + std::to_string(level) + ".tga";
        if (IconFileExists(f1)) return f1;
        std::string f2 = "sheldman" + std::to_string(level) + ".tga"; // 兼容旧命名
        if (IconFileExists(f2)) return f2;
        if (IconFileExists("dunbing.tga")) return "dunbing.tga";
    }
    // 3) 通用回退链
    if (IconFileExists("zheshizhuanzhi.tga")) return "zheshizhuanzhi.tga";
    if (IconFileExists("zhuanzhi.tga"))        return "zhuanzhi.tga";   // "zhuanzhi.tga"
    return "danwei.tga"; // 最终兜底
}
