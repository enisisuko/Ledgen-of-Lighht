// game.cpp  —— HUD图标等比绘制 & youdie 覆盖层 +（完善视效：光晕/拖影/抖屏/飘字/1秒复活 + 震击伤害 + 永远移动的法师）

#include "main.h"
#include "controller.h"
#include "game.h"

#include "GameReady.h"
#include "GameBattle.h"
#include "GameResult.h"
#include "Map.h"

// —— 这些必须在用到前包含（LoadTexture/Object_2D/Direct3D尺寸/RTS9::GameState 等）——
#include "BuildSystem.h"
#include "BuildGlobals.h"     // gBuild / RTS9::GameState / BuildingType...
#include "texture.h"          // LoadTexture
#include "sprite.h"           // Object_2D / BoxVertexsMake / DrawObject_2D
#include "direct3d.h"         // Direct3D_GetBackBufferWidth/Height

#include <string>
#include <unordered_map>
#include <algorithm>
#include <vector>
#include <random>
#include <cmath>

// 供 MenuBG 调用数字贴图函数（在 namespace MenuBG 之前）
static void HUD_DrawNum(const std::string& s, float cx, float cy, float h, float alpha = 1.0f);

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

// =========================================================
// 开始界面：背景战斗（MenuBG）+ 两按钮（键盘上下切换、呼吸摇摆高亮）
// =========================================================
static bool  gShowStartMenu = true;   // 启动直接显示开始菜单
static int   gStartSel = 0;           // 0=开始游戏  1=无限资源
static float gBtnT = 0.f;             // 按钮动画时间
static bool  gPrevLB = false;         // 鼠标左键沿边

static bool KeyJustPressed(int vk) {
    static bool prev[256] = { false };
    bool now = kbd.KeyIsPressed(vk);
    bool j = (now && !prev[vk]);
    prev[vk] = now;
    return j;
}

// —— 鼠标工具 —— 
static void GetMouseGamePos(float& gx, float& gy) {
    const float W = (float)Direct3D_GetBackBufferWidth();
    const float H = (float)Direct3D_GetBackBufferHeight();
    POINT p; GetCursorPos(&p); ScreenToClient(*GethWnd(), &p);
    gx = (float)p.x - W * 0.5f;
    gy = (float)p.y - H * 0.5f;
}
static bool MouseJustPressedL() {
    SHORT s = GetAsyncKeyState(VK_LBUTTON);
    bool down = (s & 0x8000) != 0;
    bool just = (down && !gPrevLB);
    gPrevLB = down;
    return just;
}

// —— 纯色矩形 —— 
struct BtnRect { float cx, cy, w, h; };
static void DrawRectSolid(const BtnRect& r, float R, float G, float B, float A) {
    Object_2D o(0, 0, r.w, r.h);
    o.Transform.Position = { r.cx, r.cy };
    o.Color = { R,G,B,A };
    o.texNo = 0;
    BoxVertexsMake(&o);
    DrawObject_2D(&o);
}
static bool PtInRectCenter(float x, float y, const BtnRect& r) {
    return (std::fabs(x - r.cx) <= r.w * 0.5f) && (std::fabs(y - r.cy) <= r.h * 0.5f);
}

//static void DrawSpriteRot(const char* tex, V2 p, float w, float h, float angleRad,
//    float a = 1, float r = 1, float g = 1, float b = 1) {
//    Object_2D o(0, 0, w, h);
//    o.Transform.Position = { p.x + gCam.x, p.y + gCam.y };
//    // 关键：以 Z 轴为旋转轴（屏幕法线方向）
//    o.Transform.Rotation = { 0.0f, 0.0f, angleRad };
//    o.Color = { r,g,b,a };
//    o.texNo = Tex(tex);
//    BoxVertexsMake(&o);
//    DrawObject_2D(&o);
//}


// —— 标签贴图（有就画，没有就忽略）——
static void TryDrawLabelCentered(const char* fname, const BtnRect& r, float scaleH = 0.55f) {
    if (!fname || !*fname) return;
    static std::unordered_map<std::string, unsigned> cache;
    auto it = cache.find(fname);
    unsigned tex = 0;
    if (it != cache.end()) tex = it->second;
    else {
        std::string path = std::string("rom:/") + fname;
        tex = LoadTexture(path.c_str()); cache[fname] = tex;
    }
    if (!tex) return;
    Object_2D o(0, 0, r.w * 0.78f, r.h * scaleH);
    o.Transform.Position = { r.cx, r.cy };
    o.Color = { 1,1,1,1 }; o.texNo = tex;
    BoxVertexsMake(&o); DrawObject_2D(&o);
}

// —— 两个按钮布局 —— 
static void StartMenuLayout(BtnRect& bStart, BtnRect& bInf) {
    const float W = (float)Direct3D_GetBackBufferWidth();
    const float H = (float)Direct3D_GetBackBufferHeight();
    float bw = (std::min)(W, H) * 0.77f;
    float bh = H * 0.45f;
    float gap = H * 0.02f;
    float cy0 = -H * 0.25f;
    bStart = { 0.0f, cy0,            bw, bh };
    bInf = { 0.0f, cy0 + bh + gap, bw, bh };
}

// =========================================================
// 背景战斗（加强版视效）：脚下阴影/光晕/拖影/抖屏/飘字/1秒复活
// + 规则：BOSS 冲击波额外对我方造成 0.5*BOSS 攻击伤害；法师3永远移动
// =========================================================
// =========================================================
// 背景战斗（加强版视效）：脚下阴影/光晕/拖影/抖屏/飘字/1秒复活
// + 规则：BOSS 冲击波额外对我方造成 0.5*BOSS 攻击伤害；法师3永远移动
// =========================================================
namespace MenuBG {
    // —— 常量 —— 
    static constexpr float VS = 1.3f;        // 角色基准缩放
    static constexpr float BOSS_SCALE = 5.0f;
    static constexpr float DT = 1.0f / 60.0f;

    // —— 光晕材质（不存在则回退）——
    static constexpr const char* TEX_BEAM = "light.tga";
    static constexpr const char* TEX_HALO_SOFT = "halo_soft.tga";
    static constexpr const char* TEX_HALO_MAGE = "halo_mage.tga";
    static constexpr const char* TEX_HALO_SHIELD = "halo_shield.tga";
    static constexpr const char* TEX_HALO_BOSS = "halo_boss.tga";

    // —— 类型提前：避免“先用后声明” —— ★ FIX
    struct V2 { float x = 0.f, y = 0.f; };

    enum class Team { Ally, Enemy };
    enum class Kind { Shield, Mage, Boss };

    // —— 前置声明：避免“未声明标识符” —— ★ FIX
    static float frand(float a, float b);
    static float dist(const V2& a, const V2& b);

    // —— 内联工具 —— 
    static inline float clamp01(float x) { return x < 0.f ? 0.f : (x > 1.f ? 1.f : x); }
    static inline float len(const V2& v) { return std::sqrt(v.x * v.x + v.y * v.y); }
    static inline V2    nrm(const V2& v) {
        float L = len(v); if (L <= 1e-5f) return { 0,0 }; return { v.x / L, v.y / L };
    }
    static inline float frand01() { return frand(0.f, 1.f); }  // ★ 先声明 frand 再调用

    // —— 光晕参数（带弹跳跟随）——
    static constexpr float HALO_MAGE_GROWTH = 0.70f; // 蓄满放大
    static constexpr float HALO_SHIELD_SCALE = 1.15f;
    static constexpr float HALO_BOSS_SCALE = 1.25f;
    static constexpr float HALO_BOUNCE_SCALE = 0.06f; // 弹跳额外等比
    static constexpr float HALO_BOUNCE_Y = 1.0f;  // yoff 跟随强度

    // —— 单位/子弹/特效/飘字 —— 
    struct Unit {
        Team team = Team::Ally; Kind kind = Kind::Shield;
        V2 pos{}, last{};
        float maxhp = 100, hp = 100, atk = 10, range = 120, cd = 1.2f, t = 0;
        float speed = 80; int level = 3;
        const char* tex = "xb.tga";
        bool  alive = true;

        // 动画/移动视觉
        bool  moving = false;
        float animPhase = 0.0f;

        // 受击闪白
        float hitFlashT = 0.0f; // 秒

        // 拖影（Boss 冲击或击退效果）
        float ghostT = 0.0f;    // 剩余时间
        float ghostDur = 0.7f;  // 持续时长
        V2    ghostDir{};       // 被外推方向

        // 复活计时
        float reviveT = 0.0f;

        // ===== Mage Brain (human-like) =====
        int   brainOrbitSign = 1;     // +1/-1 顺/逆时针
        float brainDesiredR = 0.f;    // 偏好半径（围绕射程附近波动）
        float brainNextThink = 0.f;   // 下次重估策略时间
        float brainReactLag = 0.f;    // 反应延迟（切换策略时的“犹豫”）
        float brainJitterAmp = 0.0f;  // 抖动幅度（让轨迹不死）
        float brainJitterPh = 0.0f;   // 抖动相位
        float brainDodgeCD = 0.0f;    // 闪避冷却（避免频繁横跳）
    };

    struct Bullet {
        V2 pos{}; float spd = 260; float dmg = 10;
        int target = -1; Team team = Team::Ally;
        const char* tex = "fireball.tga"; bool alive = true;
        V2 heading{ 1.0f, 0.0f }; // 新增：当前飞行朝向（单位向量，默认向右）
    };

    struct FX { V2 pos{}; float t = 0, dur = 0.20f; const char* tex = "attack_effect.tga"; bool alive = true; };

    struct FloatNum {
        V2 pos{}, vel{}; std::string text;
        float t = 0.0f, dur = 0.9f; float r = 1, g = 1, b = 1; bool  alive = true;
    };

    // —— 画面/状态 ——  ★ 放在函数之前，避免使用时未声明
    static std::vector<Unit>      U;
    static std::vector<Bullet>    B;
    static std::vector<FX>        FXs;
    static std::vector<FloatNum>  FNs;

    static float W = 0, H = 0, L = 0, T = 0, Wf = 0, Hf = 0;

    // —— 随机 —— 
    static std::mt19937& RNG() { static std::mt19937 g{ 13579u }; return g; }
    static float frand(float a, float b) { std::uniform_real_distribution<float>d(a, b); return d(RNG()); }

    // —— 贴图缓存 —— 
    static unsigned Tex(const char* name) {
        static std::unordered_map<std::string, unsigned> cache;
        auto it = cache.find(name);
        if (it != cache.end()) return it->second;
        std::string p = "rom:/"; p += name;
        unsigned t = LoadTexture(p.c_str()); cache[name] = t; return t;
    }

    // ==== 画面抖动（仅作用于战场，不影响菜单 UI）====
    static V2 gCam{ 0,0 };
    static float gShakeT = 0.f, gShakeDur = 0.f, gShakeAmp = 0.f;
    static float frandS(float a, float b) { std::uniform_real_distribution<float> d(a, b); return d(RNG()); }
    static void AddScreenShake(float amp, float dur) {
        gShakeAmp = std::max(gShakeAmp, amp);
        gShakeDur = std::max(gShakeDur, dur);
        gShakeT = std::max(gShakeT, dur);
    }
    static V2 ComputeCamShakeOffset(float dt) {
        if (gShakeT <= 0.f || gShakeDur <= 0.f || gShakeAmp <= 0.f) return { 0,0 };
        float p = std::max(0.f, std::min(1.f, gShakeT / gShakeDur));
        float a = gShakeAmp * p * p;        // 二次衰减
        gShakeT -= dt; if (gShakeT < 0) gShakeT = 0;
        return { frandS(-1.f,1.f) * a, frandS(-1.f,1.f) * a };
    }

    // —— 绘制小工具 —— 
    static void DrawSprite(const char* tex, V2 p, float w, float h, float a = 1, float r = 1, float g = 1, float b = 1) {
        Object_2D o(0, 0, w, h);
        o.Transform.Position = { p.x + gCam.x, p.y + gCam.y };
        o.Color = { r,g,b,a }; o.texNo = Tex(tex);
        BoxVertexsMake(&o); DrawObject_2D(&o);
    }
    static void DrawSpriteRot(const char* tex, V2 p, float w, float h, float angleRad,
        float a = 1.f, float r = 1.f, float g = 1.f, float b = 1.f) {
        Object_2D o(0, 0, w, h);
        o.Transform.Position = { p.x + gCam.x, p.y + gCam.y };
        o.Transform.Rotation = angleRad;     // ← 关键：Rotation 是单个 float
        o.Color = { r,g,b,a };
        o.texNo = Tex(tex);
        BoxVertexsMake(&o);
        DrawObject_2D(&o);
    }
    static void DrawSpriteCenterTexId(unsigned texId, V2 p, float w, float h, float a = 1, float r = 1, float g = 1, float b = 1) {
        if (!texId) return;
        Object_2D o(0, 0, w, h);
        o.Transform.Position = { p.x + gCam.x, p.y + gCam.y };
        o.Color = { r,g,b,a }; o.texNo = texId;
        BoxVertexsMake(&o); DrawObject_2D(&o);
    }

    static void Clamp(V2& p, float rad = 20) {
        p.x = (std::max)(L + rad, (std::min)(L + Wf - rad, p.x));
        p.y = (std::max)(T + rad, (std::min)(T + Hf - rad, p.y));
    }
    static float dist(const V2& a, const V2& b) { float dx = a.x - b.x, dy = a.y - b.y; return std::sqrt(dx * dx + dy * dy); }

    // 友军分离：避免堆叠（更像真人会给队友让位） —— ★ 放到 U 定义之后
    static V2 SteerSeparation(int meIdx, float radius, float strength) {
        V2 acc{ 0,0 };
        const V2& me = U[meIdx].pos;
        for (int j = 0; j < (int)U.size(); ++j) {
            if (j == meIdx || !U[j].alive || U[j].team != U[meIdx].team) continue;
            float d = dist(me, U[j].pos);
            if (d < radius && d > 1e-3f) {
                float w = (radius - d) / radius; // 越近越强
                V2 away = nrm(V2{ me.x - U[j].pos.x, me.y - U[j].pos.y });
                acc.x += away.x * w;
                acc.y += away.y * w;
            }
        }
        acc = nrm(acc);
        acc.x *= strength; acc.y *= strength;
        return acc;
    }

    // 不时重估“偏好半径/方向/抖动”（像人在临场微调）
    static void ReconsiderMageBrain(Unit& u) {
        if (u.brainDesiredR <= 0.f) u.brainDesiredR = u.range * frand(0.82f, 0.94f);
        if (u.brainNextThink <= 0.f) {
            // 1) 轻微上下浮动目标半径
            u.brainDesiredR = u.range * frand(0.82f, 0.96f);
            // 2) 小概率换个环绕方向
            if (frand01() < 0.18f) u.brainOrbitSign = -u.brainOrbitSign;
            // 3) 抖动参数
            u.brainJitterAmp = frand(0.25f, 0.55f);     // 0~1
            u.brainJitterPh = frand(0.f, 6.28318f);
            // 4) 反应延迟 + 下次思考时间
            u.brainReactLag = frand(0.05f, 0.16f);
            u.brainNextThink = frand(0.8f, 1.6f);
        }
    }

    // —— 飘字 —— 
    static void AddFloatNumber(const V2& p, int amount, bool isHeal = false) {
        FloatNum f;
        f.pos = { p.x, p.y - 16.0f * VS };
        f.vel = { frand(-12.0f, 12.0f), -80.0f * VS };
        f.text = (isHeal ? "+" : "-") + std::to_string(std::max(0, amount));
        f.dur = 0.95f;
        if (isHeal) { f.r = 0.15f; f.g = 0.95f; f.b = 0.2f; }
        else { f.r = 0.95f; f.g = 0.2f;  f.b = 0.2f; }
        FNs.push_back(f);
    }

    static V2 SpawnPoint(Team t, Kind k) {
        if (t == Team::Ally) {
            if (k == Kind::Mage)   return { L + frand(Wf * 0.12f, Wf * 0.24f), T + frand(Hf * 0.18f, Hf * 0.82f) };
            else                   return { L + frand(Wf * 0.26f, Wf * 0.42f), T + frand(Hf * 0.18f, Hf * 0.82f) };
        }
        else {
            if (k == Kind::Boss)   return { L + Wf * 0.88f, 0 };
            else                   return { L + frand(Wf * 0.78f, Wf * 0.92f), T + frand(Hf * 0.18f, Hf * 0.82f) };
        }
    }

    static float gTime = 0.f;   // 用于游走/环绕

    static void Init() {
        RECT rc; GetClientRect(*GethWnd(), &rc);
        W = float(rc.right - rc.left); H = float(rc.bottom - rc.top);
        Wf = W; Hf = H; L = -Wf * 0.5f; T = -Hf * 0.5f;

        U.clear(); B.clear(); FXs.clear(); FNs.clear();

        gShakeT = gShakeDur = gShakeAmp = 0.f; gCam = { 0,0 };
        gTime = 0.f;

        // 10 法（正常血量：30）
        for (int i = 0; i < 10; i++) {
            Unit m; m.team = Team::Ally; m.kind = Kind::Mage; m.maxhp = m.hp = 30; m.atk = 25; m.range = 400; m.cd = 2.0f; m.speed = 200; m.tex = "magicman3.tga";
            m.pos = { L + frand(Wf * 0.15f, Wf * 0.30f), T + frand(Hf * 0.15f, Hf * 0.85f) };
            U.push_back(m);
        }
        // 10 盾（正常血量：100）
        for (int i = 0; i < 10; i++) {
            Unit s; s.team = Team::Ally; s.kind = Kind::Shield; s.maxhp = s.hp = 100; s.atk = 10; s.range = 135; s.cd = 1.0f; s.speed = 250; s.tex = "sheldman3.tga";
            s.pos = { L + frand(Wf * 0.25f, Wf * 0.45f), T + frand(Hf * 0.15f, Hf * 0.85f) };
            U.push_back(s);
        }
        // 1 Boss（正常血量：5000）
        Unit boss; boss.team = Team::Enemy; boss.kind = Kind::Boss; boss.maxhp = boss.hp = 5000; boss.atk = 24; boss.range = 190; boss.cd = 1.2f; boss.speed = 60; boss.tex = "boss.tga";
        boss.pos = { L + Wf * 0.80f, 0 };
        U.push_back(boss);
    }

    static int NearestEnemy(int i) {
        const auto& me = U[i]; int best = -1; float bd = 1e9f;
        for (int j = 0; j < (int)U.size(); ++j) {
            if (i == j || !U[j].alive) continue;
            if (U[j].team == me.team) continue;
            float d = dist(me.pos, U[j].pos);
            if (d < bd) { bd = d; best = j; }
        }
        return best;
    }

    static void Update(float dt = DT) {
        // 抖动推进 & 时间推进
        gCam = ComputeCamShakeOffset(dt);
        gTime += dt;

        // 复活推进
        for (auto& u : U) {
            if (!u.alive) {
                u.reviveT -= dt;
                if (u.reviveT <= 0.0f) {
                    u.alive = true;
                    u.hp = u.maxhp;
                    u.pos = SpawnPoint(u.team, u.kind);
                    u.last = u.pos;
                    u.hitFlashT = 0.0f; u.ghostT = 0.0f;
                }
            }
        }

        // AI & 战斗
        for (int i = 0; i < (int)U.size(); ++i) {
            auto& u = U[i];                           // ★ FIX：先拿到 u
            if (!u.alive) continue;

            // ★ FIX：这些更新要放在声明 u 之后
            u.brainNextThink = (std::max)(0.f, u.brainNextThink - dt);
            u.brainReactLag = (std::max)(0.f, u.brainReactLag - dt);
            u.brainDodgeCD = (std::max)(0.f, u.brainDodgeCD - dt);

            int ti = NearestEnemy(i);

            if (ti != -1) {
                auto& t = U[ti];
                float d = dist(u.pos, t.pos);

                if (u.kind == Kind::Mage) {
                    // ===== 真人风格：优先索敌 + 射程带内环绕走位 + 抖动 + 让位 + 闪避 =====
                    ReconsiderMageBrain(u);               // 可能更新期望半径/环绕方向/抖动等

                    // 朝向与切向
                    V2 toT{ t.pos.x - u.pos.x, t.pos.y - u.pos.y };
                    float Ld = (std::max)(0.001f, std::sqrt(toT.x * toT.x + toT.y * toT.y));
                    toT.x /= Ld; toT.y /= Ld;
                    V2 tang{ -toT.y * (float)u.brainOrbitSign, toT.x * (float)u.brainOrbitSign };

                    // 带：过远/过近/舒适圈（以 brainDesiredR 为中心）
                    float desired = (u.brainDesiredR > 0.f ? u.brainDesiredR : u.range * 0.9f);
                    float nearBand = desired * 0.83f;
                    float farBand = (std::max)(desired, u.range * 0.96f);

                    // 反应延迟：刚切换策略时稍钝一点（更像人）
                    float reactK = (u.brainReactLag > 0.f) ? 0.45f : 1.0f;

                    // 速度分配
                    float approachSpd = u.speed * 1.00f * reactK;
                    float retreatSpd = u.speed * 0.70f * reactK;
                    float orbitSpd = u.speed * 0.78f;

                    V2 vel{ 0,0 };

                    if (d > farBand) {
                        // 压近
                        vel.x += toT.x * approachSpd;
                        vel.y += toT.y * approachSpd;
                    }
                    else if (d < nearBand) {
                        // 稍微后撤
                        vel.x -= toT.x * retreatSpd;
                        vel.y -= toT.y * retreatSpd;
                    }
                    else {
                        // 射程内：切向环绕 + 抖动
                        vel.x += tang.x * orbitSpd;
                        vel.y += tang.y * orbitSpd;

                        // 抖动：一丢丢径向 + 一丢丢切向（轨迹不死）
                        float w = 1.7f + 0.3f * std::sin(gTime * 1.3f + i * 0.47f);
                        float j = std::sin(gTime * w + u.brainJitterPh) * u.brainJitterAmp; // -amp~+amp
                        vel.x += (toT.x * 0.20f + tang.x * 0.35f) * (u.speed * 0.55f) * j;
                        vel.y += (toT.y * 0.20f + tang.y * 0.35f) * (u.speed * 0.55f) * j;

                        // “探身”到侧面（peek）
                        V2 peek = SteerSeparation(i, /*radius*/64.f, /*strength*/ u.speed * 0.35f);
                        vel.x += peek.x;
                        vel.y += peek.y;
                    }

                    // 面向 Boss 的“读招”闪避
                    if (U[ti].kind == Kind::Boss && d < U[ti].range * 1.25f && u.brainDodgeCD <= 0.f && U[ti].t < 0.25f) {
                        vel.x += tang.x * u.speed * 1.35f;
                        vel.y += tang.y * u.speed * 1.35f;
                        u.brainDodgeCD = 0.9f;                 // 冷却 0.9s 再闪
                        u.brainReactLag = frand(0.05f, 0.12f);  // 闪后“硬直”
                    }

                    // 轻微分离，避免抱团
                    V2 sep = SteerSeparation(i, /*radius*/56.f, /*strength*/ u.speed * 0.25f);
                    vel.x += sep.x; vel.y += sep.y;

                    // 应用速度
                    u.pos.x += vel.x * dt;
                    u.pos.y += vel.y * dt;
                }
                else {
                    // 盾/Boss 贴脸
                    if (d > u.range * 0.9f) {
                        V2 v{ t.pos.x - u.pos.x, t.pos.y - u.pos.y }; float Ld = (std::max)(0.001f, std::sqrt(v.x * v.x + v.y * v.y)); v.x /= Ld; v.y /= Ld;
                        u.pos.x += v.x * u.speed * dt; u.pos.y += v.y * u.speed * dt;
                    }
                }

                // 出手
                u.t -= dt;
                if (u.t <= 0 && d <= u.range) {
                    if (u.kind == Kind::Mage) {

                        // 假设你已经有发射起点 sx, sy，和目标/方向向量 dx, dy，以及伤害 dmg
                        // 计算朝向 & 起点（略偏出角色身体）
                        V2 dir{ t.pos.x - u.pos.x, t.pos.y - u.pos.y };
                        float L = (std::max)(0.001f, std::sqrt(dir.x * dir.x + dir.y * dir.y));
                        dir.x /= L; dir.y /= L;

                        float sx = u.pos.x + dir.x * 24.0f * VS;
                        float sy = u.pos.y + dir.y * 24.0f * VS;

                        // 用 MenuBG 的本地子弹系统发射
                        Bullet b;
                        b.pos = { sx, sy };
                        b.spd = 260.0f;
                        b.dmg = u.atk;      // 伤害同单位攻击
                        b.target = ti;         // 追踪最近敌人
                        b.team = u.team;
                        b.tex = "fireball.tga";
                        b.alive = true;
                        B.push_back(b);


                        // 如果要强制发巨型法球（不走计数器触发），用：
                        // Battle_SpawnAllyFireballGiant(sx, sy, dx, dy);


                    }
                    else {
                        // 近战命中
                        t.hp -= u.atk; t.hitFlashT = 0.18f; AddFloatNumber(t.pos, (int)u.atk, /*isHeal*/false);
                        FXs.push_back(FX{ t.pos, 0, 0.20f, "attack_effect.tga", true });
                        if (t.hp <= 0) { t.alive = false; t.reviveT = 1.0f; }  // ★ 死亡后 1 秒复活

                        // Boss 近战：冲击波 + 抖动 + 拖影 + ★对我方造成半攻伤害
                        if (u.kind == Kind::Boss) {
                            float radius = 290.0f * VS;
                            float maxPush = 140.0f * VS;
                            int shockDmg = (int)std::round(u.atk * 0.5f); // ★ 半攻伤害
                            for (auto& a : U) {
                                if (!a.alive) continue;
                                if (&a == &u) continue;
                                float dd = dist(a.pos, u.pos);
                                if (dd > radius) continue;

                                V2 dir{ a.pos.x - u.pos.x, a.pos.y - u.pos.y };
                                float Ld = (std::max)(0.001f, std::sqrt(dir.x * dir.x + dir.y * dir.y)); dir.x /= Ld; dir.y /= Ld;
                                float fall = 1.0f - (dd / radius);
                                float pixels = maxPush * (0.45f + 0.55f * fall);

                                // 外推
                                a.pos.x += dir.x * pixels; a.pos.y += dir.y * pixels; Clamp(a.pos, 18);
                                a.ghostT = a.ghostDur; a.ghostDir = dir;

                                // ★ 对敌对阵营（我方）造成半攻伤害
                                if (a.team != u.team) {
                                    a.hp -= shockDmg;
                                    a.hitFlashT = 0.18f;
                                    AddFloatNumber(a.pos, shockDmg, /*heal*/false);
                                    if (a.hp <= 0) { a.alive = false; a.reviveT = 1.0f; }
                                }
                            }
                            AddScreenShake(14.0f, 0.35f);
                            FXs.push_back(FX{ u.pos, 0, 0.35f, "attack_effecten.tga", true });
                        }
                    }
                    u.t = u.cd;
                }
            }
            else {
                // —— 没有目标：法师也保持移动（小幅游走），其他单位原地略微游走 —— 
                if (u.kind == Kind::Mage) {
                    float ang = gTime * 1.6f + i * 0.7f;
                    V2 dir{ std::cos(ang), std::sin(ang) };
                    u.pos.x += dir.x * u.speed * 0.6f * dt;
                    u.pos.y += dir.y * u.speed * 0.6f * dt;
                }
                else {
                    float ang = gTime * 0.6f + i * 0.5f;
                    V2 dir{ std::cos(ang), std::sin(ang) };
                    u.pos.x += dir.x * u.speed * 0.15f * dt;
                    u.pos.y += dir.y * u.speed * 0.15f * dt;
                }
            }

            // 动画/状态
            float dx = u.pos.x - u.last.x, dy = u.pos.y - u.last.y;
            u.moving = (std::sqrt(dx * dx + dy * dy) > 0.5f);
            if (u.moving) {
                u.animPhase += dt * 8.0f;
                if (u.animPhase > 1000.0f) u.animPhase = std::fmod(u.animPhase, 6.2831853f);
            }
            else {
                // 法师永远移动：如果检测到“停”，给一点抖动式漂移
                if (u.kind == Kind::Mage) {
                    float ang = gTime * 2.2f + i * 0.9f;
                    u.pos.x += std::cos(ang) * 1.5f;
                    u.pos.y += std::sin(ang) * 1.5f;
                    u.moving = true;
                }
                else {
                    u.animPhase *= (1.0f - std::min(1.0f, 4.0f * dt));
                }
            }
            u.last = u.pos;

            // 计时
            if (u.hitFlashT > 0.0f) u.hitFlashT -= dt;
            if (u.ghostT > 0.0f)    u.ghostT = (std::max)(0.0f, u.ghostT - dt);

            Clamp(u.pos, 18);
        }

        // 子弹
        for (auto& b : B) {
            if (!b.alive) continue;
            if (b.target < 0 || b.target >= (int)U.size() || !U[b.target].alive) { b.alive = false; continue; }
            auto& t = U[b.target];
            V2 d{ t.pos.x - b.pos.x, t.pos.y - b.pos.y }; float Ld = (std::max)(0.001f, std::sqrt(d.x * d.x + d.y * d.y)); d.x /= Ld; d.y /= Ld;
            b.heading = d; // 记录这帧的朝向（以便绘制时旋转）
            b.pos.x += d.x * b.spd * dt; b.pos.y += d.y * b.spd * dt;
            if (Ld < 12) {
                t.hp -= b.dmg; t.hitFlashT = 0.18f; AddFloatNumber(t.pos, (int)b.dmg, /*isHeal*/false);
                FXs.push_back(FX{ t.pos, 0, 0.20f, "attack_effect.tga", true });
                if (t.hp <= 0) { t.alive = false; t.reviveT = 1.0f; }
                b.alive = false;
            }
        }
        B.erase(std::remove_if(B.begin(), B.end(), [](const Bullet& x) { return !x.alive; }), B.end());

        // FX
        for (auto& e : FXs) { if (!e.alive) continue; e.t += dt; if (e.t >= e.dur) e.alive = false; }

        // 飘字
        for (auto& f : FNs) {
            if (!f.alive) continue;
            f.t += dt; f.pos.x += f.vel.x * dt; f.pos.y += f.vel.y * dt;
            if (f.t >= f.dur) f.alive = false;
        }
        FNs.erase(std::remove_if(FNs.begin(), FNs.end(), [](const FloatNum& x) { return !x.alive; }), FNs.end());
    }

    // —— 阴影 & 场地 —— 
    static unsigned TexShadow() {
        unsigned t = Tex(TEX_HALO_SOFT);
        if (!t) t = Tex(TEX_BEAM);
        return t;
    }
    static void DrawShadow(V2 p, float w, float h, float alpha = 0.35f) {
        unsigned tex = TexShadow(); if (!tex) return;
        DrawSpriteCenterTexId(tex, { p.x, p.y + 6.0f }, w, h, alpha, 0, 0, 0);
    }

    static void DrawBG() {
        const float Wb = (float)Direct3D_GetBackBufferWidth();
        const float Hb = (float)Direct3D_GetBackBufferHeight();

        unsigned tb = Tex("bg_01.tga");
        if (!tb) tb = Tex("bg_01.tga");
        if (tb) {
            Object_2D o(0, 0, Wb, Hb);
            o.Transform.Position = { 0,0 };
            o.texNo = tb; o.Color = { 1,1,1,1 };
            BoxVertexsMake(&o); DrawObject_2D(&o);
        }
        else {
            Object_2D o(0, 0, Wb, Hb);
            o.Transform.Position = { 0,0 }; o.texNo = 0; o.Color = { 0,0,0,1 };
            BoxVertexsMake(&o); DrawObject_2D(&o);
        }

        // 绿色战场（跟抖动）
        unsigned tField = Tex("bg_02.tga");
        V2 center{ L + Wf * 0.5f, T + Hf * 0.5f };
        if (tField) {
            Object_2D o(0, 0, Wf, Hf);
            o.Transform.Position = { center.x + gCam.x, center.y + gCam.y };
            o.texNo = tField; o.Color = { 1,1,1,1 };
            BoxVertexsMake(&o); DrawObject_2D(&o);
        }
        else {
            DrawRectSolid({ 0 + gCam.x, 0 + gCam.y, Wf, Hf }, 0.08f, 0.12f, 0.10f, 1.0f);
            const float thick = 6.0f;
            DrawRectSolid({ 0 + gCam.x, -Hf * 0.5f + thick * 0.5f + gCam.y, Wf, thick }, 0, 0, 0, 1);
            DrawRectSolid({ 0 + gCam.x,  Hf * 0.5f - thick * 0.5f + gCam.y, Wf, thick }, 0, 0, 0, 1);
            DrawRectSolid({ -Wf * 0.5f + thick * 0.5f + gCam.x, 0 + gCam.y, thick, Hf }, 0, 0, 0, 1);
            DrawRectSolid({ Wf * 0.5f - thick * 0.5f + gCam.x, 0 + gCam.y, thick, Hf }, 0, 0, 0, 1);
        }
    }

    static void DrawUnits() {
        const int   GHOSTS = 4;
        const float GHOST_STEP = 18.0f;

        // 先画单位（含阴影、光晕、受击闪光、拖影）
        for (size_t i = 0; i < U.size(); ++i) {
            auto& u = U[i]; if (!u.alive) continue;

            float base = 48.0f * VS;
            if (u.kind == Kind::Boss) base *= BOSS_SCALE;

            // 跳动动画参数
            float scaleX = 1.0f, scaleY = 1.0f, yoff = 0.0f;
            if (u.moving) {
                float k = 0.5f * (1.0f + std::sin(u.animPhase));
                scaleY = 1.0f + 0.12f * k;
                scaleX = 1.0f - 0.08f * k;
                yoff = (6.0f * VS) * k;
            }
            float hopK = (u.moving ? 0.5f * (1.0f + std::sin(u.animPhase)) : 0.0f);
            float haloScaleBounce = 1.0f + HALO_BOUNCE_SCALE * hopK;
            float haloY = u.pos.y - yoff * HALO_BOUNCE_Y;

            // 阴影
            DrawShadow({ u.pos.x, u.pos.y - yoff }, base * 1.05f, base * 0.45f, 0.32f);

            // 光晕（本体之前绘制）
            unsigned texMage = Tex(TEX_HALO_MAGE);   if (!texMage)   texMage = Tex(TEX_HALO_SOFT);
            unsigned texShield = Tex(TEX_HALO_SHIELD); if (!texShield) texShield = Tex(TEX_HALO_SOFT);
            unsigned texBoss = Tex(TEX_HALO_BOSS);   if (!texBoss)   texBoss = Tex(TEX_HALO_SOFT);

            if (u.kind == Kind::Mage && u.cd > 0.0f) {
                float charge = 1.0f - std::max(0.f, std::min(1.f, u.t / u.cd)); // 0~1
                float cs = 1.0f + HALO_MAGE_GROWTH * charge;
                DrawSpriteCenterTexId(texMage, { u.pos.x, haloY },
                    base * cs * haloScaleBounce, base * cs * haloScaleBounce,
                    0.7f * charge, 0.6f, 0.8f, 1.0f);
            }
            if (u.kind == Kind::Shield) {
                // 距离最近敌人<范围则显示护航光圈
                int ti = NearestEnemy((int)i);
                if (ti != -1) {
                    float d = dist(u.pos, U[ti].pos);
                    if (d < u.range * 1.4f) {
                        float pulse = 0.3f + 0.2f * std::sin(gBtnT * 10.0f + (float)i);
                        DrawSpriteCenterTexId(texShield, { u.pos.x, haloY },
                            base * HALO_SHIELD_SCALE * haloScaleBounce, base * HALO_SHIELD_SCALE * haloScaleBounce,
                            0.25f + pulse, 1.0f, 1.0f, 1.0f);
                    }
                }
            }
            if (u.kind == Kind::Boss) {
                float pulse = 0.5f + 0.5f * std::sin(gBtnT * 3.0f);
                DrawSpriteCenterTexId(texBoss, { u.pos.x, haloY },
                    base * HALO_BOSS_SCALE * haloScaleBounce, base * HALO_BOSS_SCALE * haloScaleBounce,
                    0.27f + 0.17f * pulse, 1.0f, 0.4f, 0.4f);
            }

            // 拖影（被外推时）
            if (u.ghostT > 0.f && u.ghostDur > 0.f) {
                const int   GHOSTS = 4;
                const float GHOST_STEP = 18.0f * VS;
                float p = std::max(0.f, std::min(1.f, u.ghostT / u.ghostDur));
                for (int gi = 1; gi <= GHOSTS; ++gi) {
                    float fall = 1.0f - (float)gi / (float)(GHOSTS + 1);
                    float ga = 0.28f * p * fall;
                    V2 gp{
                        u.pos.x - u.ghostDir.x * GHOST_STEP * gi * (0.6f + 0.4f * p),
                        u.pos.y - u.ghostDir.y * GHOST_STEP * gi * (0.6f + 0.4f * p)
                    };
                    DrawSprite(u.tex, gp, base * scaleX, base * scaleY, ga, 1, 1, 1);
                }
            }

            // 受击闪白叠层
            if (u.hitFlashT > 0.0f) {
                float k = std::max(0.f, std::min(1.f, u.hitFlashT / 0.18f));
                DrawSprite(u.tex, { u.pos.x, u.pos.y - yoff },
                    base * scaleX * (1.0f + 0.06f * k),
                    base * scaleY * (1.0f + 0.06f * k),
                    0.75f * k, 1.7f, 1.6f, 1.6f);
            }

            // 本体
            DrawSprite(u.tex, { u.pos.x, u.pos.y - yoff }, base * scaleX, base * scaleY, 1, 1, 1, 1);
        }

        // 子弹
        for (auto& b : B) if (b.alive) {
            // 只旋转 fireball 系（以向右为前）
            if (b.tex && std::strcmp(b.tex, "fireball.tga") == 0) {
                float ang = std::atan2(b.heading.y, b.heading.x); // 右为 0°
                DrawSpriteRot(b.tex, b.pos, 28.0f * VS, 28.0f * VS, ang, 1);
            }
            else {
                DrawSprite(b.tex, b.pos, 28.0f * VS, 28.0f * VS, 1);
            }
        }

        // 近战FX
        for (auto& e : FXs) if (e.alive) {
            float t = e.t / e.dur; float a = (t < 0.5f) ? (t / 0.5f) : (1.0f - (t - 0.5f) / 0.5f);
            a = std::max(0.f, std::min(1.f, a));
            DrawSprite(e.tex, e.pos, 42.0f * VS, 42.0f * VS, 0.85f * a);
        }

        // 飘字（数字贴图）
        for (const auto& f : FNs) {
            if (!f.alive) continue;
            float t = std::max(0.f, std::min(1.f, f.t / f.dur));
            float alpha = (t < 0.4f) ? (t / 0.4f) : (1.0f - (t - 0.4f) / 0.6f);
            float hNum = ((float)Direct3D_GetBackBufferWidth()) * 0.018f;
            HUD_DrawNum(f.text, f.pos.x + gCam.x, f.pos.y + gCam.y, hNum, alpha);
        }
    }
} // namespace MenuBG


// —— 开始菜单：绘制 —— 
extern void Polygon_Draw(void); // 2D管线重绑

static void DrawStartMenu() {
    if (!gShowStartMenu) return;

    Polygon_Draw();         // 确保2D管线
    MenuBG::DrawBG();       // 先画背景战斗场地
    MenuBG::DrawUnits();    // 再画单位

    const float W = (float)Direct3D_GetBackBufferWidth();
    const float H = (float)Direct3D_GetBackBufferHeight();

    // 半透明罩一层，让按钮更清晰
    DrawRectSolid({ 0,0,W,H }, 0.0f, 0.0f, 0.0f, 0.25f);

    // 标题条
    DrawRectSolid({ 0,-H * 0.28f,W * 0.60f,H * 0.08f }, 0.18f, 0.23f, 0.36f, 0.95f);
    DrawRectSolid({ 0,-H * 0.28f,W * 0.60f - 10.f,H * 0.08f - 10.f }, 0.30f, 0.36f, 0.58f, 0.95f);

    BtnRect bStart, bInf; StartMenuLayout(bStart, bInf);

    // 呼吸 + 摇摆
    gBtnT += 1.0f / 60.0f;
    auto pulse = [](float t) { return 0.5f + 0.5f * std::sin(t * 3.4f); };
    auto sway = [](float t) { return std::sin(t * 5.5f) * 10.0f; }; // 左右摇摆偏移像素

    auto drawBtn = [&](BtnRect r, bool selected, float baseHueR, float baseHueG, float baseHueB, const char* cn, const char* en) {
        float k = pulse(gBtnT) * (selected ? 0.32f : 0.18f);
        float glow = selected ? 0.85f : 0.55f;
        float scale = selected ? (1.0f + 0.06f * std::sin(gBtnT * 3.2f)) : 1.0f;
        float xoff = selected ? sway(gBtnT) : 0.0f;

        DrawRectSolid({ r.cx + xoff, r.cy, r.w * scale, r.h * scale }, baseHueR + k, baseHueG + k, baseHueB + k, 0.95f);
        DrawRectSolid({ r.cx + xoff, r.cy, r.w * scale - 10, r.h * scale - 10 }, 0.92f, 0.96f, 0.99f, selected ? 0.98f : 0.92f);
        if (selected) DrawRectSolid({ r.cx + xoff, r.cy, r.w * scale + 14, r.h * scale + 14 }, glow, glow, glow, 0.12f);

        TryDrawLabelCentered(cn, { r.cx + xoff, r.cy, r.w * scale, r.h * scale }, 0.55f);
        TryDrawLabelCentered(en, { r.cx + xoff, r.cy, r.w * scale, r.h * scale }, 0.55f);
        };

    drawBtn(bStart, gStartSel == 0, 0.10f, 0.35f, 0.45f, "start_cn.tga", "start.tga");
    drawBtn(bInf, gStartSel == 1, 0.20f, 0.32f, 0.10f, "infinite_cn.tga", "infinite.tga");
}

// —— 开始菜单：输入/逻辑 —— 
static bool UpdateStartMenu() {
    if (!gShowStartMenu) return false;

    // 背景战斗推进（始终跑）
    MenuBG::Update(1.0f / 60.0f);

    // 键盘上下切换
    if (KeyJustPressed(VK_UP) || KeyJustPressed('W')) gStartSel = (gStartSel + 1) % 2;
    if (KeyJustPressed(VK_DOWN) || KeyJustPressed('S')) gStartSel = (gStartSel + 1) % 2;

    // 鼠标悬停同步选中项
    float mx, my; GetMouseGamePos(mx, my);
    BtnRect bStart, bInf; StartMenuLayout(bStart, bInf);
    if (PtInRectCenter(mx, my, bStart)) gStartSel = 0;
    if (PtInRectCenter(mx, my, bInf))   gStartSel = 1;

    auto doChoose = [&](int sel) {
        if (sel == 0) {
            gShowStartMenu = false;
            SetcurrentScene(GameScene::Ready);
        }
        else {
            gBuild.res.coins += 1000;
            gBuild.res.units += 100;
            gShowStartMenu = false;
            SetcurrentScene(GameScene::Ready);
        }
        };

    // 回车/空格选择
    if (KeyJustPressed(VK_RETURN) || KeyJustPressed(VK_SPACE)) { doChoose(gStartSel); return true; }
    // 鼠标点击
    if (MouseJustPressedL()) {
        if (PtInRectCenter(mx, my, bStart)) { doChoose(0); return true; }
        if (PtInRectCenter(mx, my, bInf)) { doChoose(1); return true; }
    }
    return true;
}

// ---- BuildUI forward declares (for game.cpp) ----
namespace RTS9 { struct GameState; }
void BuildUI_UpdateOnlyCountdown();
void BuildUI_DrawTopMost(const RTS9::GameState& gs);

// Battle 场景过场透明度（用于 youdie 贴图）
extern float Battle_GetFailFadeAlpha();

// =========================================================
// 数字贴图（0-9 与 '/')
// =========================================================
static unsigned int HUD_LoadDigit(char ch) {
    static std::unordered_map<char, unsigned int> cache;
    auto it = cache.find(ch);
    if (it != cache.end()) return it->second;

    std::string path;
    if (ch == '/') path = "rom:/number/pie.tga";
    else if (ch >= '0' && ch <= '9') path = "rom:/number/" + std::string(1, ch) + ".tga";
    else return 0;

    unsigned int id = LoadTexture(path.c_str());
    cache[ch] = id;
    return id;
}
// ---- 定义处不写默认参数（避免编译器“重复默认参数”错误）----
static void HUD_DrawNum(const std::string& s, float cx, float cy, float h, float alpha /*no default*/)
{
    if (s.empty()) return;
    const float w = h * 0.57f;
    const float pad = h * 0.06f;
    const float totalW = (w + pad) * (float)s.size() - pad;
    float x = cx - totalW * 0.5f;

    for (char ch : s) {
        unsigned int tex = HUD_LoadDigit(ch);
        if (tex) {
            Object_2D o(0, 0, w, h);
            o.Transform.Position = { x + w * 0.5f, cy };
            o.Color = { 1,1,1,alpha };
            o.texNo = tex;
            BoxVertexsMake(&o);
            DrawObject_2D(&o);
        }
        x += (w + pad);
    }
}

static float HUD_NumWidth(const std::string& s, float h) {
    if (s.empty()) return 0.0f;
    const float w = h * 0.5f;
    const float pad = h * 0.06f;
    return (w + pad) * (float)s.size() - pad;
}

// =========================================================
// 图标：等比绘制（通过表配置宽高比 = 宽/高）
// =========================================================
struct IconInfo { unsigned int id = 0; float aspect = 1.0f; };
static float HUD_GetAspect(const std::string& name) {
    static const std::unordered_map<std::string, float> ar = {
        {"wave.tga",      1.7f},{"qian.tga",      1.7f},{"danwei.tga",    1.7f},
        {"mofa.tga",      1.7f},{"dunbing.tga",   1.7f},{"naqian.tga",    1.7f},
        {"nadanwei.tga",  1.7f},
    };
    auto it = ar.find(name);
    return (it != ar.end() ? it->second : 1.0f);
}
static IconInfo HUD_LoadIconInfo(const char* fname) {
    static std::unordered_map<std::string, IconInfo> cache;
    auto it = cache.find(fname);
    if (it != cache.end()) return it->second;

    IconInfo info;
    std::string path = std::string("rom:/") + fname;
    info.id = LoadTexture(path.c_str());
    info.aspect = HUD_GetAspect(fname);
    cache.emplace(fname, info);
    return info;
}

// 单行提示：等比图标 + 文本（数字贴图），用于菜单价格面板
static void HUD_DrawLabelSimple(const char* icon, const std::string& text,
    float leftX, float centerY,
    float iconH, float numH, float gap)
{
    IconInfo ii = HUD_LoadIconInfo(icon);
    float iw = iconH * (ii.aspect > 0.0f ? ii.aspect : 1.0f);
    float cx_icon = leftX + iw * 0.5f;

    if (ii.id) {
        Object_2D o(0, 0, iw, iconH);
        o.Transform.Position = { cx_icon, centerY };
        o.Color = { 1,1,1,1 };
        o.texNo = ii.id;
        BoxVertexsMake(&o);
        DrawObject_2D(&o);
    }

    float wNum = HUD_NumWidth(text, numH);
    float cx_num = leftX + iw + gap + wNum * 0.5f;
    HUD_DrawNum(text, cx_num, centerY, numH, 1.0f);
}

// 一行：等比图标 + 右侧数字；数字列使用统一左边界对齐
static void HUD_DrawIconRow_AR(const char* icon, int value,
    float leftX, float centerY,
    float iconH, float numH, float numLeftX) {
    IconInfo ii = HUD_LoadIconInfo(icon);
    float iw = iconH * (ii.aspect > 0.0f ? ii.aspect : 1.0f);
    float cx_icon = leftX + iw * 0.5f;

    if (ii.id) {
        Object_2D o(0, 0, iw, iconH);
        o.Transform.Position = { cx_icon, centerY };
        o.Color = { 1,1,1,1 };
        o.texNo = ii.id;
        BoxVertexsMake(&o);
        DrawObject_2D(&o);
    }

    std::string s = std::to_string(value);
    float wNum = HUD_NumWidth(s, numH);
    float cx_num = numLeftX + wNum * 0.5f;
    HUD_DrawNum(s, cx_num, centerY, numH, 1.0f);
}

// ===== 数字逐个变化 + 弹出动画（仅本文件） =====
static inline float HUD_saturate(float x) { return x < 0.f ? 0.f : (x > 1.f ? 1.f : x); }
static inline float HUD_dt() { return 1.0f / 60.0f; }
static float HUD_Pop01(float p, float over = 1.72f) {
    p = HUD_saturate(p);
    if (p < 0.25f) { float t = p / 0.25f; return 1.0f + (over - 1.0f) * t; }
    else { float t = (p - 0.25f) / 0.75f; return over + (1.0f - over) * t; }
}
struct HUDCounterAnim {
    int   shown = 0;  int   target = 0;  float acc = 0.f;
    float stepInterval = 0.02f; float popT = 1.f; float popDur = 0.27f;
    void setTarget(int v) { target = v; }
    void tick(float dt) {
        if (shown != target) {
            acc += dt;
            while (acc >= stepInterval && shown != target) {
                acc -= stepInterval;
                shown += (shown < target) ? 1 : -1;
                popT = 0.f;
            }
        }
        if (popT < popDur) popT += dt;
    }
    float scale() const {
        float p = (popDur > 0.f) ? HUD_saturate(popT / popDur) : 1.f;
        return HUD_Pop01(p, 1.77f);
    }
};
static void HUD_DrawIconRow_AR_Anim(const char* icon, int value,
    float leftX, float centerY,
    float iconH, float numH, float numLeftX, float numScale)
{
    IconInfo ii = HUD_LoadIconInfo(icon);
    float iw = iconH * (ii.aspect > 0.0f ? ii.aspect : 1.0f);
    float cx_icon = leftX + iw * 0.5f;

    if (ii.id) {
        Object_2D o(0, 0, iw, iconH);
        o.Transform.Position = { cx_icon, centerY };
        o.Color = { 1,1,1,1 };
        o.texNo = ii.id;
        BoxVertexsMake(&o);
        DrawObject_2D(&o);
    }

    std::string s = std::to_string(value);
    float scaledH = numH * (numScale > 0.f ? numScale : 1.f);
    float wNum = HUD_NumWidth(s, scaledH);
    float cx_num = numLeftX + wNum * 0.5f;
    HUD_DrawNum(s, cx_num, centerY, scaledH, 1.0f);
}

// =========================================================
// 下一波收益计算（基于当前基建 gBuild）
// =========================================================
static void HUD_ComputeNextWaveIncome(const RTS9::GameState& gs, int& outGold, int& outUnits) {
    outGold = outUnits = 0;
    for (const auto& t : gs.grid) {
        const auto& b = t.b;
        if (b.level <= 0) continue;
        using BT = RTS9::BuildingType;
        if (b.type == BT::Gold)          outGold += RTS9::GoldIncome(b.level);
        else if (b.type == BT::Recruit)  outUnits += RTS9::RecruitUnits(b.level);
    }
    outGold += 20;
    outUnits += 1;
}

// =========================================================
// 左侧 HUD 主绘制（保留你的逻辑）
// =========================================================
static void DrawLeftHUD() {
    Polygon_Draw(); // 保证2D管线

    const float W = (float)Direct3D_GetBackBufferWidth();
    const float H = (float)Direct3D_GetBackBufferHeight();

    const float marginL = W * 0.02f;
    const float topY = H * -0.3f;
    const float rowH = (H * 0.70f) / 7.0f;

    const float iconH = rowH * 0.70f;
    const float numH = rowH * 0.58f;
    const float gap = iconH * 0.35f;

    const int waveReady = Battle_GetWaveForHUD_Ready();
    const int waveBattle = Battle_GetWaveForHUD_Battle();
    const bool inBattle = (GetcurrentScene() == GameScene::Battle);
    const int waveShown = inBattle ? waveBattle : waveReady;

    int nextGold = 0, nextUnits = 0;
    HUD_ComputeNextWaveIncome(gBuild, nextGold, nextUnits);

    const int coins = gBuild.res.coins;
    const int units = gBuild.res.units;
    const int mageCount = gBuild.res.mageUnits;
    const int shieldCnt = gBuild.res.shieldUnits;

    struct Row { const char* icon; int val; };
    Row rows[7] = {
        { "wave.tga",     waveShown },
        { "qian.tga",     coins     },
        { "danwei.tga",   units     },
        { "mofa.tga",     mageCount },
        { "dunbing.tga",  shieldCnt },
        { "naqian.tga",   nextGold  },
        { "nadanwei.tga", nextUnits },
    };

    float maxIconW = 0.0f;
    for (int i = 0; i < 7; ++i) {
        IconInfo ii = HUD_LoadIconInfo(rows[i].icon);
        float iw = iconH * (ii.aspect > 0.0f ? ii.aspect : 1.0f);
        if (iw > maxIconW) maxIconW = iw;
    }

    // 菜单里的价格/数量小面板
    if (gBuild.menuOpen && gBuild.menuTileIndex >= 0 && gBuild.menuTileIndex < (int)gBuild.grid.size()) {
        int sel = gBuild.selectedIdx;
        if (sel >= 0 && sel < (int)gBuild.currentMenu.size()) {
            using A = RTS9::MenuAction;
            A act = gBuild.currentMenu[sel].action;

            float panelLeftX = marginL;
            float panelY = topY - rowH * 0.70f;
            float iconH2 = iconH * 0.85f;
            float numH2 = numH;
            float gap2 = iconH2 * 0.35f;

            const auto& tile = gBuild.grid[gBuild.menuTileIndex];
            if (act == A::Upgrade) {
                int lv = tile.b.level;
                int cost = RTS9::UpgradeCost(lv);
                HUD_DrawLabelSimple("qian.tga", std::to_string(cost), panelLeftX, panelY, iconH2, numH2, gap2);
            }
            else if (act == A::Demolish) {
                int lv = tile.b.level;
                int refund = RTS9::RefundOnDemolish(lv);
                HUD_DrawLabelSimple("qian.tga", std::to_string(refund), panelLeftX, panelY, iconH2, numH2, gap2);
            }
            else if (act == A::Train) {
                int spec = 0;
                if (tile.b.type == RTS9::BuildingType::MageHouse)        spec = gBuild.res.mageUnits;
                else if (tile.b.type == RTS9::BuildingType::ShieldHouse) spec = gBuild.res.shieldUnits;
                std::string txt = std::to_string(spec) + "/" + std::to_string(gBuild.res.units);
                HUD_DrawLabelSimple("danwei.tga", txt, panelLeftX, panelY, iconH2, numH2, gap2);
            }
        }
    }

    const float numLeftX = marginL + maxIconW + gap;

    static bool s_hudInit = false;
    static HUDCounterAnim s_cnt[7];
    if (!s_hudInit) {
        for (int i = 0; i < 7; ++i) {
            s_cnt[i].shown = rows[i].val;
            s_cnt[i].target = rows[i].val;
            s_cnt[i].popT = s_cnt[i].popDur;
        }
        s_hudInit = true;
    }

    for (int i = 0; i < 7; ++i) {
        s_cnt[i].setTarget(rows[i].val);
        s_cnt[i].tick(HUD_dt());
    }

    for (int i = 0; i < 7; ++i) {
        float cy = topY + rowH * i;
        HUD_DrawIconRow_AR_Anim(rows[i].icon, s_cnt[i].shown,
            marginL, cy, iconH, numH, numLeftX, s_cnt[i].scale());
    }
}

// =========================================================
// youdie 覆盖层（放在所有内容之后）
// =========================================================
static void DrawTopmostFadeOverlay(float /*screenW*/, float /*screenH*/) {
    Polygon_Draw(); // 重绑2D

    const float W = (float)Direct3D_GetBackBufferWidth();
    const float H = (float)Direct3D_GetBackBufferHeight();

    static unsigned int s_texYouDie = 0;
    if (s_texYouDie == 0) s_texYouDie = LoadTexture("rom:/meidongxi.tga");

    const float k = Battle_GetFailFadeAlpha();
    if (k <= 0.001f || s_texYouDie == 0) return;

    Object_2D o(0, 0, W, H);
    o.texNo = s_texYouDie;
    o.Color = { 1, 1, 1, k };
    BoxVertexsMake(&o);
    DrawObject_2D(&o);
}

// =========================================================
// 场景框架（保持你原始逻辑）
// =========================================================
GameScene currentScene = GameScene::Ready;

int Money = 0;
int Players = 0;
int Magicians = 0; // 魔法师
int warrior = 0;   // 战士
int MagiciansLv = 0;
int warriorLv = 0;

void InitializeGame(void)
{
    InitializeScreenOffset();
    GameInitializeMap();
    EnsureBuildInitialized();

    if (gShowStartMenu) {
        MenuBG::Init();   // 准备背景战斗
    }

    switch (currentScene)
    {
    case GameScene::Ready:  InitializeGameReady();  break;
    case GameScene::Battle: InitializeGameBattle(); break;
    case GameScene::Result: InitializeGameResult(); break;
    }
}

void UpdateGame(void)
{
    if (gShowStartMenu) {  // 开始界面时，只更新菜单+背景战斗
        UpdateStartMenu();
        return;
    }

    switch (currentScene)
    {
    case GameScene::Ready:  UpdateGameReady();  break;
    case GameScene::Battle: UpdateGameBattle(); break;
    case GameScene::Result: UpdateGameResult(); break;
    }

    BuildUI_UpdateOnlyCountdown();
}

void DrawGame(void)
{
    if (gShowStartMenu) {
        DrawStartMenu();
        return;
    }

    switch (currentScene)
    {
    case GameScene::Ready:  DrawGameReady();  break;
    case GameScene::Battle: DrawGameBattle(); break;
    case GameScene::Result: DrawGameResult(); break;
    }

    // 先画左侧HUD（战斗阶段不渲染）
    if (currentScene != GameScene::Battle) {
        DrawLeftHUD();
    }

    BuildUI_DrawTopMost(gBuild);

    // 最后：失败过场 youdie 叠在最上层
    RECT rc; GetClientRect(*GethWnd(), &rc);
    DrawTopmostFadeOverlay(float(rc.right - rc.left), float(rc.bottom - rc.top));
}

void FinalizeGame(void)
{
    switch (currentScene)
    {
    case GameScene::Ready:  FinalizeGameReady();  break;
    case GameScene::Battle: FinalizeGameBattle(); break;
    case GameScene::Result: FinalizeGameResult(); break;
    }
}

GameScene GetcurrentScene(void) { return currentScene; }

void SetcurrentScene(GameScene type)
{
    if (currentScene == type) return;

    switch (currentScene)
    {
    case GameScene::Ready:  FinalizeGameReady();  break;
    case GameScene::Battle: FinalizeGameBattle(); break;
    case GameScene::Result: FinalizeGameResult(); break;
    default: break;
    }

    currentScene = type;

    switch (currentScene)
    {
    case GameScene::Ready:  InitializeGameReady();  break;
    case GameScene::Battle: InitializeGameBattle(); break;
    case GameScene::Result: InitializeGameResult(); break;
    default: break;
    }
}

// 包装到 gBuild 资源
int  GetMoney(void) { return gBuild.res.coins; }
int  GetPlayers(void) { return gBuild.res.units; }
void ADDMoney(int i) { gBuild.res.coins += i; }
void ADDPlayers(int i) { gBuild.res.units += i; }
void SUBMoney(int i) { gBuild.res.coins -= i; }
void SUBPlayers(int i) { gBuild.res.units -= i; }
