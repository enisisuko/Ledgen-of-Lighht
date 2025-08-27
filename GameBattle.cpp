// =========================================================
// gamebattle.cpp —— 战斗场景（AI 目标/移动智能化 + 圣女脉冲奶 + 飘字
// + 新敌人 + 死亡FX + 塔变黑CD + 隐形Alpha可调
// + 【★】击退拖影动画 + 摄像机抖动 + BOSS登场演出 + 视效增强 + AI强化
// + 【★】四级技能（法师巨型法球/盾兵超级震退） + BOSS狂暴横冲(重做目标锁定/三连冲)
// + 【★】防御塔属性随圣女等级升级 + 圣女范围圈旋转
// + 【★】全局VFX升级（子弹尾迹/屏闪/彩虹辉光/单位左右翻转）
// + 【★】BOSS新增：地裂重踏/棱镜横扫/陨星雨
// + 【★】普通兵种：弓手三连射、小兵死亡溅射；敌方血性光环（Bloodlust）
// + 【★】4级法师：链式闪雷/缓时领域/棱镜新星；4级盾兵：守护穹罩/大地裂波/战号加持
// =========================================================
#include "main.h"
#include "texture.h"
#include "sprite.h"
#include "controller.h"
#include "GameBattle.h"
#include "collision.h"
#include "Map.h"
#include "UI.h"
#include "game.h"
#include "direct3d.h"  // Direct3D_GetBackBufferWidth/Height

// 资源/基建
#include "BuildGlobals.h"
#include "BuildSystem.h"

#include <unordered_map>
#include <string>
#include <vector>
#include <cmath>
#include <algorithm>
#include <random>
#include <deque>


// ==== SFX 绑定式封装（可零侵入接入你现有音频系统）====
//using FnSfxLoad = int(*)(const char* path);       // 返回 soundId（>=0）
//using FnSfxPlay = void(*)(int id, float vol, float pitch, bool loop);
//
//static FnSfxLoad gSfxLoad = nullptr;
//static FnSfxPlay gSfxPlay = nullptr;
//
//void Battle_BindSfx(FnSfxLoad loader, FnSfxPlay player) {
//    gSfxLoad = loader; gSfxPlay = player;
//}
//
//static int LoadSfxCached(const std::string& fname) {
//    static std::unordered_map<std::string, int> cache;
//    auto it = cache.find(fname);
//    if (it != cache.end()) return it->second;
//
//    // 资源路径：可按你项目改，比如 "rom:/sfx/" 或直接相对路径
//    std::string path = "rom:/sound/" + fname;
//    int id = (gSfxLoad ? gSfxLoad(path.c_str()) : -1);
//    cache.emplace(fname, id);
//    return id;
//}
//static void PlaySFX(const std::string& fname, float vol = 1.0f, float pitch = 1.0f, bool loop = false) {
//    if (!gSfxPlay) return; // 未绑定则静音
//    int id = LoadSfxCached(fname);
//    if (id >= 0) gSfxPlay(id, vol, pitch, loop);
//}
//static inline float RandPitch(float cents = 20.0f) {
//    // 抖动音高，±20 cents ≈ ±1.2%
//    float r = Battle::frand(-1.0f, 1.0f);
//    return std::pow(2.0f, (r * cents) / 1200.0f);
//}


// ---------------------------------------------------------
// 【需要的新增贴图（可缺省，缺省会优雅回退到 fallback）】
// 1) 冲击与光环：
//    "shock_ring.tga", "glow.tga", "flare.tga", "rainbow_glow.tga", "star_glow.tga"
// 2) 线与束：
//    "laser.tga", "chain_beam.tga"
// 3) 爆裂与碎片：
//    "burst.tga", "spark.tga", "crack.tga"
// 4) 尾迹与轨迹：
//    "trail.tga"
// 5) 领域：
//    "slow_field.tga", "dome.tga", "earth_wave.tga", "vortex.tga"
// 6) 弹体：
//    "meteor.tga", "prismshot.tga"
// 【已有资源回退】：没有则使用 "light.tga" 或 "attack_effect.tga"
// ---------------------------------------------------------

// === 视觉/玩法统一比例 ===
static constexpr float VISUAL_SCALE = 1.3f;
static constexpr float ARCHER_RANGE_BASE = 220.0f;
static constexpr float KNOCKBACK_DIST = (ARCHER_RANGE_BASE * 0.2f) * VISUAL_SCALE;
static constexpr float KNOCKBACK_TIME = 0.75f;
static constexpr float TANK_VISUAL_SCALE = 2.6f;
// 【★】BOSS体型放大 5 倍
static constexpr float BOSS_VISUAL_SCALE = 7.0f;
// 【★】英雄（4级友军）体型放大倍数
static constexpr float HERO_VISUAL_SCALE = 3.0f;

// 【★】击退拖影
static constexpr int   KNOCKBACK_GHOSTS = 4;
static constexpr float KNOCKBACK_GHOST_STEP = 18.0f;

// 【★】BOSS登场横幅（渐显/停留/渐隐）
static constexpr float gBossInDur = 0.6f;
static constexpr float gBossHoldDur = 1.1f;
static constexpr float gBossOutDur = 0.6f;

// 【★】Runner智选感知半径
static constexpr float RUNNER_SENSE_R = 360.0f * VISUAL_SCALE;

// 【★】盾兵超级震退参数
static constexpr float SHIELD_SUPER_KB_DIST = 200.0f * VISUAL_SCALE;
static constexpr float SHIELD_SUPER_KB_DUR = 0.35f;

// 【★】BOSS三连冲（重做）
static constexpr float BOSS_RAMPAGE_SPEED = 700.0f;   // ← 700
static constexpr float BOSS_RAMPAGE_HITDMG = 10.0f;
static constexpr float BOSS_RAMPAGE_DASH_DUR = 0.85f; // 单次冲撞持续时长
static constexpr int   BOSS_RAMPAGE_CHARGES = 3;      // 三连冲

// 【★】法师巨型法球
static constexpr int   MAGE_GIANT_TRIGGER = 3;          // 每 6 次（前 5 次普通，第 6 次巨型）
static constexpr float MAGE_GIANT_AOE = 150.0f * VISUAL_SCALE;
static constexpr float MAGE_GIANT_DMG = 50.0f;

static constexpr float ARROW_SPEED = 760.0f;                         // 敌方弓箭
static constexpr float ARROW_TTL = 3.0f;
static constexpr float ARROW_HIT_R = 14.0f * VISUAL_SCALE;
static constexpr float ARROW_SPREAD_RD = 3.0f * 3.1415926f / 180.0f;     // 最大±3°

static constexpr float FIREBALL_SPEED = 620.0f;                         // 我方法师
static constexpr float FIREBALL_TTL = 3.0f;
static constexpr float FIREBALL_HIT_R = 18.0f * VISUAL_SCALE;
static constexpr float FIREBALL_SPREAD_RD = 2.0f * 3.1415926f / 180.0f;  // 最大±2°

// 【改】新增：面向翻转冷却常量
static constexpr float FACE_TURN_CD = 0.5f;

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

// ====== 小工具：贴图缓存 ======
static unsigned int LoadIconCached_Battle(const std::string& fname)
{
    static std::unordered_map<std::string, unsigned int> cache;
    if (fname.empty()) return 0;
    auto it = cache.find(fname);
    if (it != cache.end()) return it->second;
    std::string path = "rom:/" + fname;
    unsigned int id = LoadTexture(path.c_str());
    cache[fname] = id;
    return id;
}

// ====== 数字贴图缓存（支持 0-9 / + - / / ）======
static unsigned int LoadNumberTex(char ch) {
    static std::unordered_map<char, unsigned int> cache;
    auto it = cache.find(ch);
    if (it != cache.end()) return it->second;

    std::string path;
    if (ch == '/') path = "rom:/number/pie.tga";
    else if (ch == '+') path = "rom:/number/plus.tga";
    else if (ch == '-') path = "rom:/number/minus.tga";
    else if (ch >= '0' && ch <= '9') path = "rom:/number/" + std::string(1, ch) + ".tga";
    else return 0;

    unsigned int id = LoadTexture(path.c_str());
    cache[ch] = id;
    return id;
}

// ====== 画数字串（中心对齐，可上色） ======
static void DrawNumberString_World(const std::string& s, float cx, float cy,
    float heightPx, float alpha = 1.0f,
    float r = 1.0f, float g = 1.0f, float b = 1.0f)
{
    if (s.empty()) return;
    const float w = heightPx * 0.62f;
    const float pad = heightPx * 0.06f;

    const float totalW = (w + pad) * (float)s.size() - pad;
    float x = cx - totalW * 0.5f;
    float y = cy;

    for (char ch : s) {
        unsigned int tex = LoadNumberTex(ch);
        if (tex != 0) {
            Object_2D o(0, 0, w, heightPx);
            o.Transform.Position = { x + w * 0.5f, y };
            o.Color = { r, g, b, alpha };
            o.texNo = tex;
            BoxVertexsMake(&o);
            DrawObject_2D(&o);
        }
        x += (w + pad);
    }
}

// ---- 波次进度持久化（跨场景保存）----
static int gNextWave = 1;
static int gNextDifficulty = 4;

// 失败过场：2s 变暗 -> 清空基建 -> 2s 变亮 -> 回到基建
enum class FailPhase { None, FadingOut, FadingIn };
static FailPhase gFailPhase = FailPhase::None;
static float gFailT = 0.0f;
static constexpr float gFailDur = 2.0f; // 每段 2 秒

// ★ 胜利过场
enum class WinPhase { None, FadingIn, Hold, FadingOut };
static WinPhase gWinPhase = WinPhase::None;
static float gWinT = 0.0f;
static constexpr float gWinIn = 0.9f;
static constexpr float gWinHold = 0.6f;
static constexpr float gWinOut = 0.9f;
// ★ WIN 过场尺寸控制（正方形）
static float gWinScale = 0.88f;
static bool  gWinUseSquare = true;
// ★ YOUDIE 过场尺寸（正方形 & 缩放）
static float gDieScale = 0.90f;
static bool  gDieUseSquare = true;

// 【★】摄像机抖动
namespace Battle { struct Vec2 { float x = 0, y = 0; }; }
using CamVec2 = Battle::Vec2;
static float gShakeT = 0.0f;
static float gShakeDur = 0.0f;
static float gShakeAmp = 0.0f;
static CamVec2 gRenderCam{ 0,0 };
static inline float clamp01(float v) { return v < 0 ? 0 : (v > 1 ? 1 : v); }
static std::mt19937& RNG_shake() { static std::mt19937 g{ 987654u }; return g; }
static float frand_shake(float a, float b) { std::uniform_real_distribution<float> d(a, b); return d(RNG_shake()); }
static void AddScreenShake(float amp, float dur) {
    gShakeAmp = std::max(gShakeAmp, amp);
    gShakeDur = std::max(gShakeDur, dur);
    gShakeT = std::max(gShakeT, dur);
}
static CamVec2 ComputeCamShakeOffset() {
    if (gShakeT <= 0.0f || gShakeDur <= 0.0f || gShakeAmp <= 0.0f) return { 0,0 };
    float p = clamp01(gShakeT / gShakeDur);
    float a = gShakeAmp * p * p;
    return { frand_shake(-1.0f,1.0f) * a, frand_shake(-1.0f,1.0f) * a };
}
static inline float WaveLinear(int wave, float per) {
    return 1.0f + per * (float)std::max(0, wave - 1);
}

// 【★】全局时间（供闪烁/脉动/旋转用）
static float gBattleTime = 0.0f;

// 【★】屏幕白闪（重大事件用）
static float gFlashT = 0.0f;
static float gFlashDur = 0.0f;
static float gFlashA = 0.0f;
static void AddScreenFlash(float alpha, float dur) {
    gFlashA = std::max(gFlashA, alpha);
    gFlashDur = std::max(gFlashDur, dur);
    gFlashT = std::max(gFlashT, dur);
}

// 【★】BOSS登场横幅
enum class BossBanner { None, FadingIn, Hold, FadingOut };
static BossBanner gBossBanner = BossBanner::None;
static float gBossBannerT = 0.0f;
static void TriggerBossBanner() { gBossBanner = BossBanner::FadingIn; gBossBannerT = 0.0f; AddScreenShake(14.0f * VISUAL_SCALE, 0.9f); }

// [INVIS] 可调：隐形者未显形时的透明度
static float gInvisibleAlpha = 0.25f;

namespace Battle {

    enum class Team { Ally, Enemy };
    enum class Kind {
        Saint, Shield, Mage, EnemyRunner, EnemyArcher, EnemyTank, EnemyMinion,
        EnemyInvisible, EnemySummoner, EnemyMeat, EnemySword,
        EnemyBoss
    };

    struct Beam {
        Vec2 a, b; float life = 0.5f; float maxLife = 0.5f; std::string tex = "light.tga"; bool alive = true;
    };

    static inline float dist(const Vec2& a, const Vec2& b) {
        float dx = a.x - b.x, dy = a.y - b.y; return std::sqrt(dx * dx + dy * dy);
    }
    static inline float clamp(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
    static inline void  norm(Vec2& v) { float L = std::sqrt(v.x * v.x + v.y * v.y); if (L > 0) { v.x /= L; v.y /= L; } }
    static std::mt19937& RNG() { static std::mt19937 g{ 123456u }; return g; }
    static float frand(float a, float b) { std::uniform_real_distribution<float> d(a, b); return d(RNG()); }

    struct Unit {
        Team team = Team::Ally;
        Kind kind = Kind::EnemyMinion;
        Vec2 pos{}, vel{};
        float hp = 1, maxhp = 1;
        float atk = 0, range = 0, atkCD = 1, atkT = 0;
        float speed = 60;
        int   level = 1;
        std::string tex;
        bool  alive = true;
        int   attackTargets = 1;

        // 动画/移动视觉
        Vec2  lastPos{};
        float animPhase = 0.0f;
        bool  moving = false;
        bool  facingRight = true; // 【★】面朝方向（自动翻转）
        // 【改】翻转冷却
        float faceLockT = 0.0f;
        float faceFlipT = 0.0f;   // 【新增】翻转挤压动画计时器（秒）


        // Buff（攻速/移速）
        float atkCdMul = 1.0f;
        float atkBuffT = 0.0f;
        float spdMul = 1.0f;
        float spdBuffT = 0.0f;

        // 击退状态
        float kbTimer = 0.0f, kbDur = 0.0f, kbRem = 0.0f;
        Vec2  kbDir{ 0,0 };

        // 冲锋手
        float chargeT = 0, chargeCD = 3, chargeSpd = 500, walkSpd = 120, curSpd = 120;

        // 隐形者
        bool  invisible = false;
        bool  revealed = false;

        // 呼唤者
        float summonT = 0.0f, summonCD = 0.0f;

        // 剑：攻击后自杀
        bool suicideAfterAttack = false;

        // 呼唤者游走
        Vec2 wanderTarget{};

        // 受击/治疗闪光
        float hitFlashT = 0.0f;
        float healFlashT = 0.0f;

        // 集团作战“脑”
        int   lane = -1;
        int   slot = -1;
        float brainNextThink = 0.0f;
        int   orbitSign = 1;
        float desiredR = 0.0f;
        float jitterAmp = 0.0f;
        float jitterPh = 0.0f;
        float dodgeCD = 0.0f;

        // 【★】四级技能/计数
        int   attackCounter = 0;           // 法师用
        float superKnockCD = 5.0f;         // 4级盾兵
        float superKnockT = 5.0f;

        // 【★】4级大招计时
        float heroS1 = 3.0f, heroS1CD = 6.0f;   // Mage: 链雷
        float heroS2 = 6.0f, heroS2CD = 7.0f;  // Mage: 缓时场
        float heroS3 = 9.0f, heroS3CD = 4.0f;   // Mage: 棱镜新星

        float heroB1 = 4.0f, heroB1CD = 4.0f;   // Shield: 守护穹罩
        float heroB2 = 5.5f, heroB2CD = 7.0f;   // Shield: 大地裂波
        float heroB3 = 8.0f, heroB3CD = 12.0f;  // Shield: 战号加持

        // 【★】BOSS狂暴（重做：三连冲）
        // 【新增】本轮三连冲已选过的目标，保证不重复
        int  rageHitIdx[3] = { -1, -1, -1 };
        int  rageHitCount = 0;

        bool  raging = false;
        bool  rageTrig66 = false, rageTrig33 = false;
        int   rageCharges = 0;
        float rageDashT = 0.0f;
        Vec2  rageDir{ 0,0 };

        // 【★】BOSS主动技CD
        float skill1T = 5.0f, skill1CD = 10.0f; // 地裂重踏
        float skill2T = 7.0f, skill2CD = 12.0f; // 棱镜横扫
        float skill3T = 9.0f, skill3CD = 14.0f; // 陨星雨
    };

    struct Bullet {
        Vec2 pos{}, vel{};
        float dmg = 0;
        int   target = -1;
        bool  targetTower = false;
        Team  team = Team::Ally;
        std::string tex;
        bool  alive = true;

        // 【★】范围弹（命中后爆）
        float aoe = 0.0f;   // 半径>0则是范围弹
        bool  big = false;  // 视觉用途
        bool  exploded = false; // 是否已在命中触发过AOE

        float speed = 1000.0f;                     // 飞行速度（px/s）
        float ttl = 3.0f;                       // 存活时间（秒）
        float hitRadius = 28.0f * VISUAL_SCALE;   // 碰撞半径（像素）

        // 【★】尾迹
        std::deque<Vec2> trail; // 记录最近若干帧位置
    };

    struct Effect {
        Vec2 pos{}; float life = 0.2f; std::string tex = "attack_effect.tga"; bool alive = true;
    };

    // 【★】可编排VFX（旋转/成长/自定义颜色）
    struct VFX {
        Vec2 pos{}; float t = 0.0f; float dur = 1.0f; bool alive = true;
        std::string tex = "glow.tga";
        float w = 64.0f * VISUAL_SCALE, h = 64.0f * VISUAL_SCALE;
        float rot = 0.0f, rotSpd = 0.0f;
        float grow = 0.0f;
        float r = 1, g = 1, b = 1, a = 1;
    };

    // 【★】领域：减速 & 穹罩
    struct SlowField { Vec2 pos{}; float r = 180.0f * VISUAL_SCALE; float t = 0.0f; float dur = 4.0f; bool alive = true; };
    struct DomeField { Vec2 pos{}; float r = 150.0f * VISUAL_SCALE; float t = 0.0f; float dur = 3.0f; bool alive = true; };

    struct Tower {
        Vec2 pos{}; float hp = 200, maxhp = 200;
        float atk = 20, cd = 3, t = 0;
        std::string tex = "tower.tga";
    };

    struct BattleState {
        // 场地
        float W = 0, H = 0;
        float L = 0, T = 0, Wf = 0, Hf = 0;

        bool heroMageActive = false;
        bool heroShieldActive = false;
        int  saintDamageOnPulse = 0;

        std::vector<Unit>   units;
        std::vector<Bullet> bullets;
        std::vector<Effect> effects;
        std::vector<struct Beam> beams;
        std::vector<VFX>    vfx;        // 【★】新VFX
        std::vector<SlowField> slows;   // 【★】减速场
        std::vector<DomeField> domes;   // 【★】穹罩

        Tower tower;
        float towerDrawScale = 1.0f;  // 【★】塔体型倍率（随圣女等级）

        // 圣女（脉冲治疗）
        int saint = -1;
        float healCD = 3.0f;
        float healT = 0.0f;
        float healRadius = 140.0f;
        int   healAmount = 1;
        std::string saintTex = "player.tga";
        float saintHP = 50;

        // 波次
        int wave = 1;
        int difficulty = 4;
        bool running = false;

        bool wave1BonusSpawned = false;
        bool bossAnnounced = false;
    };

    static BattleState S;
} // namespace Battle

// 便捷别名
using BVec2 = Battle::Vec2;
using BTeam = Battle::Team;
using BKind = Battle::Kind;
using BUnit = Battle::Unit;
using BBullet = Battle::Bullet;
using BEffect = Battle::Effect;
using BVFX = Battle::VFX;
using BSlow = Battle::SlowField;
using BDome = Battle::DomeField;
using BTower = Battle::Tower;
using BState = Battle::BattleState;
static BState& BS = Battle::S;

static void AddFloatNumber(const BVec2& p, int amount, bool isHeal);


// ==== forward declarations ====
static int MagePickTarget(const BUnit& m);
static std::pair<int, float> NearestAllyOfKind(BKind kind, const BVec2& from);
static int FindThreatNear(const BVec2& protectPos, float radius);
static std::pair<int, float> NearestEnemyVisible(const BVec2& from);
static int FindLowestHpAlly();
static void OnUnitDeath(const BUnit& dead);

// === 限位辅助 ===
static inline float UnitHalfSizeX(const BUnit& u) {
    float base = 48.0f * VISUAL_SCALE * 0.5f;
    if (u.team == BTeam::Enemy && u.kind == BKind::EnemyTank) base *= TANK_VISUAL_SCALE;
    else if (u.team == BTeam::Enemy && u.kind == BKind::EnemyBoss) base *= BOSS_VISUAL_SCALE;
    if (u.team == BTeam::Ally && u.level >= 4) base *= HERO_VISUAL_SCALE;
    return base;
}
static inline float UnitHalfSizeY(const BUnit& u) { return UnitHalfSizeX(u); }
static inline void ClampUnitToField(BUnit& u) {
    const float hx = UnitHalfSizeX(u);
    const float hy = UnitHalfSizeY(u);
    u.pos.x = Battle::clamp(u.pos.x, BS.L + hx, BS.L + BS.Wf - hx);
    u.pos.y = Battle::clamp(u.pos.y, BS.T + hy, BS.T + BS.Hf - hy);
}
static inline void ClampPointForUnit(BVec2& p, const BUnit& u) {
    const float hx = UnitHalfSizeX(u);
    const float hy = UnitHalfSizeY(u);
    p.x = Battle::clamp(p.x, BS.L + hx, BS.L + BS.Wf - hx);
    p.y = Battle::clamp(p.y, BS.T + hy, BS.T + BS.Hf - hy);
}

// 飘字
struct FloatNum {
    BVec2 pos{};
    BVec2 vel{};
    std::string text;
    float t = 0.0f;
    float dur = 1.0f;
    float r = 1, g = 1, b = 1;
    bool  alive = true;
};
static std::vector<FloatNum> gFloatNums;

// ====== 飘字实现（在 FloatNum / gFloatNums 定义之后粘贴） ======
static void AddFloatNumber(const BVec2& p, int amount, bool isHeal)
{
    // 生成一条从单位头顶冒出的伤害/治疗飘字
    FloatNum f;
    // 起点稍微高一点，避免与单位重叠
    f.pos = { p.x, p.y - 16.0f * VISUAL_SCALE };

    // 初速度：左右随机轻微抖动 + 向上漂浮
    f.vel = { Battle::frand(-12.0f, 12.0f), -80.0f * VISUAL_SCALE };

    // 文本内容：治疗为 "+N"，伤害为 "-N"
    f.text = (isHeal ? "+" : "-") + std::to_string(std::max(0, amount));

    // 存活时长
    f.dur = 1.0f;

    // 颜色：治疗为绿色系，伤害为红色系
    if (isHeal) { f.r = 0.15f; f.g = 0.95f; f.b = 0.20f; }
    else { f.r = 0.95f; f.g = 0.20f; f.b = 0.20f; }

    // 入队
    f.alive = true;
    gFloatNums.push_back(f);
}


// 死亡FX
struct DeathFX { BVec2 pos{}; float t = 0.0f; float dur = 1.0f; bool  alive = true; };
static std::vector<DeathFX> gDeaths;
static void AddDeathFx(const BVec2& p) { gDeaths.push_back(DeathFX{ p, 0.0f, 1.0f, true }); }

// utils
static inline float Dt() { return 1.0f / 60.0f; }
static inline BVec2 Rot(const BVec2& v, float ang) {
    float c = std::cos(ang), s = std::sin(ang);
    return { v.x * c - v.y * s, v.x * s + v.y * c };
}

// ====== 纯色矩形 ======
static void DrawSolidQuad(const BVec2& c, float w, float h, float r, float g, float b, float a)
{
    Object_2D o(0, 0, w, h);
    o.Transform.Position = { c.x + gRenderCam.x, c.y + gRenderCam.y };
    o.Color = { r, g, b, a };
    o.texNo = 0;
    BoxVertexsMake(&o);
    DrawObject_2D(&o);
}

// 【★】中心旋转绘制（带摄像机抖动）
static void DrawSpriteCenterRot(const std::string& tex, const BVec2& c, float w, float h,
    float angle, float a = 1.0f, float r = 1, float g = 1, float b = 1)
{
    unsigned int t = LoadIconCached_Battle(tex);
    if (!t) return;

    const float hw = 0.5f * w;
    const float hh = 0.5f * h;

    const float cs = std::cos(angle);
    const float sn = std::sin(angle);

    auto rot = [&](float x, float y) -> BVec2 {
        return {
            c.x + (x * cs - y * sn) + gRenderCam.x,
            c.y + (x * sn + y * cs) + gRenderCam.y
        };
        };

    const BVec2 lt = rot(-hw, -hh);
    const BVec2 rt = rot(hw, -hh);
    const BVec2 lb = rot(-hw, hh);
    const BVec2 rb = rot(hw, hh);

    Object_2D o(0, 0, 0, 0);
    o.texNo = t;
    o.Color = { r, g, b, a };

    o.UV[0] = { 0, 0 };
    o.UV[1] = { 1, 0 };
    o.UV[2] = { 0, 1 };
    o.UV[3] = { 1, 1 };

    o.Vertexs[0] = { lt.x, lt.y };
    o.Vertexs[1] = { rt.x, rt.y };
    o.Vertexs[2] = { lb.x, lb.y };
    o.Vertexs[3] = { rb.x, rb.y };

    DrawObject_2D(&o);
}

// 【★】普通中心绘制（可左右翻转）
static void DrawSpriteCenterFlip(const std::string& tex, const BVec2& p, float w, float h,
    bool flipX, float a = 1.0f, float r = 1, float g = 1, float b = 1)
{
    Object_2D o(0, 0, w, h);
    o.Transform.Position = { p.x + gRenderCam.x, p.y + gRenderCam.y };
    o.Color = { r,g,b,a };
    o.texNo = LoadIconCached_Battle(tex);

    if (!flipX) {
        o.UV[0] = { 0,0 }; o.UV[1] = { 1,0 }; o.UV[2] = { 0,1 }; o.UV[3] = { 1,1 };
    }
    else {
        o.UV[0] = { 1,0 }; o.UV[1] = { 0,0 }; o.UV[2] = { 1,1 }; o.UV[3] = { 0,1 };
    }

    BoxVertexsMake(&o);
    DrawObject_2D(&o);
}

static void DrawSpriteCenter(const std::string& tex, const BVec2& p, float w, float h,
    float a = 1.0f, float r = 1, float g = 1, float b = 1)
{
    DrawSpriteCenterFlip(tex, p, w, h, /*flipX*/false, a, r, g, b);
}

// 【★】光束绘制（含摄像机偏移）
static void DrawBeam(const Battle::Beam& be)
{
    if (!be.alive) return;
    float t = (be.life > 0.f && be.maxLife > 0.f) ? (be.life / be.maxLife) : 0.f;
    float thickness = 8.0f + 20.0f * t;
    float alpha = t;

    Battle::Vec2 a{ be.a.x + gRenderCam.x, be.a.y + gRenderCam.y };
    Battle::Vec2 b{ be.b.x + gRenderCam.x, be.b.y + gRenderCam.y };
    Battle::Vec2 dir{ b.x - a.x, b.y - a.y };
    float L = std::max(0.001f, std::sqrt(dir.x * dir.x + dir.y * dir.y));
    dir.x /= L; dir.y /= L;
    Battle::Vec2 n{ -dir.y, dir.x };
    float hh = thickness * 0.5f;

    Battle::Vec2 lt{ a.x - n.x * hh, a.y - n.y * hh };
    Battle::Vec2 rt{ b.x - n.x * hh, b.y - n.y * hh };
    Battle::Vec2 lb{ a.x + n.x * hh, a.y + n.y * hh };
    Battle::Vec2 rb{ b.x + n.x * hh, b.y + n.y * hh };

    Object_2D o(0, 0, 0, 0);
    o.texNo = LoadIconCached_Battle(be.tex);
    o.Color = { 1,1,1, alpha };
    o.UV[0] = { 0,0 }; o.UV[1] = { 1,0 }; o.UV[2] = { 0,1 }; o.UV[3] = { 1,1 };
    o.Vertexs[0] = { lt.x, lt.y };
    o.Vertexs[1] = { rt.x, rt.y };
    o.Vertexs[2] = { lb.x, lb.y };
    o.Vertexs[3] = { rb.x, rb.y };
    DrawObject_2D(&o);
}

// === 读取九宫格中某建筑的最高等级 ===
static int HighestLevelInGrid(const RTS9::GameState& gs, RTS9::BuildingType ty) {
    int best = 0;
    for (const auto& t : gs.grid) if (t.b.type == ty) best = std::max(best, t.b.level);
    return best;
}

// ====== VFX Helpers ======
static inline void VfxSpawnRing(const BVec2& p, float size, float dur, float r = 1, float g = 1, float b = 1, float a = 0.8f, const char* tex = "shock_ring.tga")
{
    BVFX fx; fx.pos = p; fx.dur = dur; fx.t = 0; fx.tex = LoadIconCached_Battle(tex) ? tex : "light.tga";
    fx.w = fx.h = size; fx.grow = 0.4f; fx.rotSpd = 0.0f; fx.r = r; fx.g = g; fx.b = b; fx.a = a; fx.alive = true;
    BS.vfx.push_back(fx);
}
static inline void VfxSpawnBurst(const BVec2& p, float size, float dur, const char* tex = "burst.tga")
{
    BVFX fx; fx.pos = p; fx.dur = dur; fx.t = 0; fx.tex = LoadIconCached_Battle(tex) ? tex : "attack_effect.tga";
    fx.w = fx.h = size; fx.grow = 0.25f; fx.rotSpd = 2.6f; fx.r = 1; fx.g = 1; fx.b = 1; fx.a = 0.95f; fx.alive = true;
    BS.vfx.push_back(fx);
}
static inline void VfxSpawnSparksRadial(const BVec2& p, int n, float base, float dur)
{
    for (int i = 0; i < n; i++) {
        float ang = (6.2831853f / n) * i + Battle::frand(0.0f, 0.3f);
        BVFX fx; fx.pos = p; fx.dur = dur; fx.t = 0; fx.tex = LoadIconCached_Battle("spark.tga") ? "spark.tga" : "attack_effect.tga";
        fx.w = fx.h = base * Battle::frand(0.6f, 1.2f);
        fx.rot = ang; fx.rotSpd = (Battle::frand(-2.0f, 2.0f));
        fx.grow = 0.35f; fx.r = 1; fx.g = 0.8f; fx.b = 0.2f; fx.a = 0.9f; fx.alive = true;
        BS.vfx.push_back(fx);
    }
}
static inline void VfxSpawnLaser(const BVec2& a, const BVec2& b, float life = 0.6f)
{
    Battle::Beam be; be.a = a; be.b = b; be.maxLife = life; be.life = life; be.tex = LoadIconCached_Battle("laser.tga") ? "laser.tga" : "light.tga";
    BS.beams.push_back(be);
}
static inline void VfxSpawnChain(const BVec2& a, const BVec2& b, float life = 0.4f)
{
    Battle::Beam be; be.a = a; be.b = b; be.maxLife = life; be.life = life; be.tex = LoadIconCached_Battle("chain_beam.tga") ? "chain_beam.tga" : "light.tga";
    BS.beams.push_back(be);
}

// -------------------------------------------------------
// —— BOSS 击退免疫
static inline bool IsKnockbackImmune(const BUnit& u) {
    return (u.team == BTeam::Enemy && u.kind == BKind::EnemyBoss);
}

// 穹罩减击退（盾兵守护穹罩）
static inline float DomeKnockbackScaleForAlly(const BUnit& target) {
    for (const auto& d : BS.domes) {
        if (!d.alive) continue;
        if (Battle::dist(d.pos, target.pos) <= d.r) return 0.3f; // 减70%
    }
    return 1.0f;
}

// 开始一次击退（带时长/剩余距离推进）
static void StartKnockback(BUnit& target, const BVec2& dir, float pixels, float duration)
{
    if (!target.alive || pixels <= 0.0f || duration <= 0.0f) return;
    if (IsKnockbackImmune(target)) return;

    float L = std::sqrt(dir.x * dir.x + dir.y * dir.y);
    if (L < 1e-4f) return;

    float scale = (target.team == BTeam::Ally ? DomeKnockbackScaleForAlly(target) : 1.0f);
    pixels *= scale;

    target.kbDir = { dir.x / L, dir.y / L };
    target.kbDur = duration;
    target.kbTimer = duration;
    target.kbRem = pixels;
}

// 每帧推进击退
static void TickKnockback(BUnit& u, float dt)
{
    if (IsKnockbackImmune(u)) { u.kbTimer = u.kbDur = u.kbRem = 0.0f; u.kbDir = { 0,0 }; return; }
    if (u.kbTimer <= 0.0f || u.kbRem <= 0.0f) return;

    float p = (u.kbDur > 0.0f) ? (u.kbTimer / u.kbDur) : 0.0f;
    float vmax = (u.kbDur > 0.0f) ? (2.0f * u.kbRem / u.kbDur) : 0.0f;
    float speed = vmax * p;
    float step = speed * dt;
    if (step > u.kbRem) step = u.kbRem;

    u.pos.x += u.kbDir.x * step;
    u.pos.y += u.kbDir.y * step;
    u.kbRem -= step;
    u.kbTimer -= dt;

    u.pos.x = Battle::clamp(u.pos.x, BS.L, BS.L + BS.Wf);
    u.pos.y = Battle::clamp(u.pos.y, BS.T, BS.T + BS.Hf);

    if (u.kbTimer <= 0.0f || u.kbRem <= 0.5f) {
        u.kbTimer = u.kbDur = u.kbRem = 0.0f;
        u.kbDir = { 0,0 };
    }
}

static inline bool IsMelee(const BUnit& u) { return !(u.kind == BKind::Mage || u.kind == BKind::EnemyArcher); }

// 瞬时位移击退
static void ApplyKnockback(BUnit& target, const BVec2& from, float distance)
{
    if (!target.alive || distance <= 0.0f) return;
    if (IsKnockbackImmune(target)) return;

    float scale = (target.team == BTeam::Ally ? DomeKnockbackScaleForAlly(target) : 1.0f);
    distance *= scale;

    BVec2 dv{ target.pos.x - from.x, target.pos.y - from.y };
    float L = std::sqrt(dv.x * dv.x + dv.y * dv.y);
    if (L < 0.0001f) { dv = { 1, 0 }; L = 1.0f; }
    dv.x /= L; dv.y /= L;

    target.pos.x += dv.x * distance;
    target.pos.y += dv.y * distance;
    target.pos.x = Battle::clamp(target.pos.x, BS.L, BS.L + BS.Wf);
    target.pos.y = Battle::clamp(target.pos.y, BS.T, BS.T + BS.Hf);
}

// 线段AB到圆(中心C, 半径r)是否相交
static inline bool SegmentHitsCircle(const BVec2& A, const BVec2& B, const BVec2& C, float r) {
    BVec2 AB{ B.x - A.x, B.y - A.y };
    BVec2 AC{ C.x - A.x, C.y - A.y };
    float ab2 = AB.x * AB.x + AB.y * AB.y;
    float t = (ab2 > 1e-6f) ? ((AC.x * AB.x + AC.y * AB.y) / ab2) : 0.0f;
    t = Battle::clamp(t, 0.0f, 1.0f);
    BVec2 P{ A.x + AB.x * t, A.y + AB.y * t };
    float dx = P.x - C.x, dy = P.y - C.y;
    return (dx * dx + dy * dy) <= r * r;
}

static inline bool OutOfField(const BVec2& p) {
    const float pad = 24.0f * VISUAL_SCALE;
    return (p.x < BS.L - pad || p.x > BS.L + BS.Wf + pad ||
        p.y < BS.T - pad || p.y > BS.T + BS.Hf + pad);
}

// ========== 群体作战：向量/工具 ==========
static inline BVec2 BAdd(const BVec2& a, const BVec2& b) { return { a.x + b.x, a.y + b.y }; }
static inline BVec2 BSub(const BVec2& a, const BVec2& b) { return { a.x - b.x, a.y - b.y }; }
static inline BVec2 BScale(const BVec2& a, float s) { return { a.x * s, a.y * s }; }
static inline float BLen(const BVec2& a) { return std::sqrt(a.x * a.x + a.y * a.y); }
static inline void  BNorm(BVec2& v) { float L = BLen(v); if (L > 1e-5f) { v.x /= L; v.y /= L; } }
static inline BVec2 BLimited(const BVec2& v, float m) { float L = BLen(v); if (L <= m) return v; return BScale(v, m / (L > 1e-6f ? L : 1.f)); }

static inline float LaneCenterY(int lane) {
    lane = (lane < 0 ? 0 : (lane > 2 ? 2 : lane));
    const float slots[3] = { 0.25f, 0.50f, 0.75f };
    return BS.T + BS.Hf * slots[lane];
}
static inline int PickLaneRoundRobin(int seq) { return (seq % 3); }

static BVec2 SteerSeparationIdx(int meIdx, float radius, float strength) {
    BVec2 acc{ 0,0 };
    const BUnit& me = BS.units[meIdx];
    float r2 = radius * radius;
    for (int j = 0; j < (int)BS.units.size(); ++j) {
        if (j == meIdx) continue;
        const BUnit& o = BS.units[j];
        if (!o.alive || o.team != me.team) continue;
        float dx = o.pos.x - me.pos.x, dy = o.pos.y - me.pos.y;
        float d2 = dx * dx + dy * dy;
        if (d2 > 1e-4f && d2 < r2) {
            float inv = 1.0f - std::sqrt(d2) / radius;
            acc.x -= (dx)*inv;
            acc.y -= (dy)*inv;
        }
    }
    BNorm(acc);
    return BScale(acc, strength);
}

// （省略：MageBrainStep / ShieldBrainStep / SmartPickTarget ...）
// —— 为节省篇幅，以下核心逻辑完全保留你原有实现，仅对关键点做增改 ——
// ★★ 从这里开始，除特别标注的“新增/修改”外，都与你给的原版一致 ★★

static BVec2 SteerDodgeProjectiles(const BUnit& u, float senseR, float force, float cooldown) {
    if (u.dodgeCD > 0.0f) return { 0,0 };
    for (const auto& b : BS.bullets) {
        // —— 原实现：只让我方闪敌弹；现在复用逻辑足够用，这里不改 team
        if (!b.alive || b.team != BTeam::Enemy) continue;
        BVec2 tp;
        if (b.targetTower) tp = BS.tower.pos;
        else if (b.target >= 0 && b.target < (int)BS.units.size() && BS.units[b.target].alive) tp = BS.units[b.target].pos;
        else continue;
        BVec2 dir = BSub(tp, b.pos); BNorm(dir);
        BVec2 ru = BSub(u.pos, b.pos);
        float along = ru.x * dir.x + ru.y * dir.y;
        if (along <= 0) continue;
        float perp = std::abs(ru.x * (-dir.y) + ru.y * (dir.x));
        if (perp < senseR) {
            BVec2 side{ -dir.y, dir.x };
            float sideSign = (ru.x * side.x + ru.y * side.y) >= 0 ? 1.f : -1.f;
            return BScale(side, force * sideSign);
        }
    }
    return { 0,0 };
}

static void ShieldLaneRank(int meIdx, int lane, int& outRank, int& outTotal) {
    outRank = 0; outTotal = 0;
    for (int i = 0; i < (int)BS.units.size(); ++i) {
        const auto& u = BS.units[i];
        if (!u.alive || u.team != BTeam::Ally || u.kind != BKind::Shield) continue;
        if (u.lane != lane) continue;
        if (i == meIdx) outRank = outTotal;
        ++outTotal;
    }
}
static void AssignLanesAndSlots() {
    int mgSeq = 0, shSeq = 0;
    for (int i = 0; i < (int)BS.units.size(); ++i) {
        auto& u = BS.units[i];
        if (!u.alive || u.team != BTeam::Ally) continue;
        if (u.kind == BKind::Mage)   u.lane = PickLaneRoundRobin(mgSeq++);
        if (u.kind == BKind::Shield) u.lane = PickLaneRoundRobin(shSeq++);
    }
    for (int i = 0; i < (int)BS.units.size(); ++i) {
        auto& u = BS.units[i];
        if (!u.alive || u.team != BTeam::Ally || u.kind != BKind::Shield) continue;
        int r = 0, t = 0; ShieldLaneRank(i, u.lane, r, t);
        u.slot = r;
    }
}

static void MageBrainStep(int i, BUnit& u, float dt) {
    u.brainNextThink -= dt; u.dodgeCD = std::max(0.0f, u.dodgeCD - dt);
    if (u.desiredR <= 0.0f) u.desiredR = u.range * 0.9f;
    if (u.brainNextThink <= 0.0f) {
        u.desiredR = u.range * Battle::frand(0.70f, 0.98f);
        if (Battle::frand(0.0f, 1.0f) < 0.18f) u.orbitSign = -u.orbitSign;
        u.jitterAmp = Battle::frand(0.20f, 0.50f);
        u.jitterPh = Battle::frand(0.0f, 6.28318f);
        u.brainNextThink = Battle::frand(0.8f, 1.6f);
    }
    int ti = MagePickTarget(u);
    BVec2 vel{ 0,0 };
    const float laneY = LaneCenterY(u.lane);

    if (ti != -1) {
        const BUnit& t = BS.units[ti];
        BVec2 to{ t.pos.x - u.pos.x, t.pos.y - u.pos.y }; BNorm(to);
        float d = Battle::dist(u.pos, t.pos);
        float nearB = u.range * 0.70f;
        float farB = u.range * 1.00f;

        if (d > farB) { vel = BAdd(vel, BScale(to, u.speed * 1.00f)); }
        else if (d < nearB) { vel = BAdd(vel, BScale(to, -u.speed * 0.80f)); }
        else {
            BVec2 tang{ -to.y * (float)u.orbitSign, to.x * (float)u.orbitSign };
            vel = BAdd(vel, BScale(tang, u.speed * 0.78f));
            float w = 1.7f + 0.3f * std::sin(gBattleTime * 1.3f + i * 0.47f);
            float j = std::sin(gBattleTime * w + u.jitterPh) * u.jitterAmp;
            vel = BAdd(vel, BScale(BAdd(BScale(to, 0.20f), BScale(tang, 0.35f)), u.speed * 0.55f * j));
        }
        BVec2 dodge = SteerDodgeProjectiles(u, 28.0f * VISUAL_SCALE, u.speed * 1.35f, 0.9f);
        if (BLen(dodge) > 0.001f) { vel = BAdd(vel, dodge); const_cast<BUnit&>(u).dodgeCD = 0.9f; }
    }
    else {
        float ang = gBattleTime * 1.8f + i * 0.7f;
        vel = BAdd(vel, BVec2{ std::cos(ang), std::sin(ang) });
        BNorm(vel); vel = BScale(vel, u.speed * 0.6f);
    }

    float dy = (laneY - u.pos.y);
    vel.y += Battle::clamp(dy, -40.f, 40.f);
    vel = BAdd(vel, SteerSeparationIdx(i, 56.f * VISUAL_SCALE, u.speed * 0.35f));
    float jig = std::sin(gBattleTime * 2.1f + i * 0.6f) * 0.12f;
    vel.x += jig * 8.0f; vel.y += jig * 4.0f;

    vel = BLimited(vel, u.speed);
    u.pos.x += vel.x * dt; u.pos.y += vel.y * dt;
}

static void ShieldBrainStep(int i, BUnit& u, float dt) {
    // 参考目标：优先就近法师；没有法师就用圣女/塔
    auto [nearMage, md] = NearestAllyOfKind(BKind::Mage, u.pos);
    int protectIdx = (nearMage != -1 ? nearMage : BS.saint);
    if (protectIdx < 0) { protectIdx = -1; } // 极端兜底

    const bool brave = (u.level >= 4);

    const float laneY = LaneCenterY(u.lane);
    int myRank = 0, total = 0; ShieldLaneRank(i, u.lane, myRank, total);
    const float SPACING_Y = 40.0f * VISUAL_SCALE;
    float offset = (total > 0 ? (myRank - (total - 1) * 0.5f) * SPACING_Y : 0.0f);

    // —— 前置/回收参数（可按手感微调）
    const float KEEP_AHEAD = 90.0f * VISUAL_SCALE;                     // 必须领先法师的 X 距离
    const float EXTRA_PUSH = (brave ? BS.Wf * 0.08f : BS.Wf * 0.02f);  // 再往前一点点
    const float MAX_LEAD_EXTRA = (brave ? 220.0f : 160.0f) * VISUAL_SCALE; // 超出这个领先就回收
    const float RUSH_EDGE_IN = 0.95f, RUSH_EDGE_OUT = 1.40f;           // 射程边缘 rush 区间

    // 计算“应处于”的锚点（X 轴：在法师前 KEEP_AHEAD；没有法师就沿用旧逻辑）
    float baseX = (protectIdx >= 0 ? BS.units[protectIdx].pos.x : BS.tower.pos.x);
    if (nearMage != -1) baseX = std::max(baseX, BS.units[nearMage].pos.x + KEEP_AHEAD);
    float anchorX = baseX + EXTRA_PUSH;
    float anchorY = laneY + offset;

    // === ★ 新增：后场威胁优先（保护对象身后存在敌人时，优先回防并禁止前冲/追击） ===
    bool backUnsafe = false;
    int rearThreat = -1;
    if (protectIdx >= 0) {
        const float REAR_MARGIN = 24.0f * VISUAL_SCALE;   // “在身后”的判定余量
        const float REAR_RADIUS = 260.0f * VISUAL_SCALE;  // 后场威胁检测半径
        const float REAR_RADIUS2 = REAR_RADIUS * REAR_RADIUS;
        const BVec2 pivot = BS.units[protectIdx].pos;

        for (int k = 0; k < (int)BS.units.size(); ++k) {
            const auto& e = BS.units[k];
            if (!e.alive || e.team != BTeam::Enemy) continue;
            float dx = e.pos.x - pivot.x;
            float dy = e.pos.y - pivot.y;
            float d2 = dx * dx + dy * dy;
            if (dx < -REAR_MARGIN && d2 <= REAR_RADIUS2) {
                rearThreat = k;
                backUnsafe = true;
                break;
            }
        }

        if (rearThreat != -1) {
            // 直接回防/拦截后场敌人
            BVec2 v{ BS.units[rearThreat].pos.x - u.pos.x, BS.units[rearThreat].pos.y - u.pos.y }; BNorm(v);
            BVec2 vel = BScale(v, u.speed * (brave ? 1.20f : 1.10f));
            vel = BAdd(vel, SteerSeparationIdx(i, 64.f * VISUAL_SCALE, u.speed * (brave ? 0.28f : 0.33f)));
            u.pos.x += vel.x * dt; u.pos.y += vel.y * dt;
            return;
        }
    }

    // —— 勇敢：先判断是否“落在法师后面”，在后面则强制超车到锚点
    if (brave && nearMage != -1) {
        const float margin = 10.0f * VISUAL_SCALE;
        bool behindMage = (u.pos.x < BS.units[nearMage].pos.x + margin);
        if (behindMage) {
            BVec2 goal{ anchorX - u.pos.x, anchorY - u.pos.y }; BNorm(goal);
            BVec2 vel = BScale(goal, u.speed * 1.55f); // 强势提速“超车”
            vel = BAdd(vel, SteerSeparationIdx(i, 56.f * VISUAL_SCALE, u.speed * 0.22f));
            u.pos.x += vel.x * dt; u.pos.y += vel.y * dt;
            return;
        }
    }

    // === ★ 调整顺序：保护逻辑（保护对象附近有威胁）放到追击之前 ===
    int threat = -1;
    if (protectIdx >= 0) threat = FindThreatNear(BS.units[protectIdx].pos, 180.0f * VISUAL_SCALE);
    if (threat != -1) {
        BVec2 v{ BS.units[threat].pos.x - u.pos.x, BS.units[threat].pos.y - u.pos.y }; BNorm(v);
        BVec2 vel = BScale(v, u.speed * (brave ? 1.08f : 1.0f));
        vel = BAdd(vel, SteerSeparationIdx(i, 64.f * VISUAL_SCALE, u.speed * (brave ? 0.28f : 0.35f)));
        u.pos.x += vel.x * dt; u.pos.y += vel.y * dt;
        return;
    }

    // —— 勇敢：可追击（但别超过“最大领先”太多）+ ★ 后场不安全时禁止追击
    if (brave) {
        auto [ei, ed] = NearestEnemyVisible(u.pos);
        if (ei != -1) {
            float chaseR = u.range * 2.4f;
            bool allowChase = (ed <= chaseR);

            // 若已经领先过多，则先回收队形，不追
            if (nearMage != -1 && u.pos.x > BS.units[nearMage].pos.x + KEEP_AHEAD + MAX_LEAD_EXTRA) {
                allowChase = false;
            }
            // ★ 新增：后场不安全时，关闭追击
            if (backUnsafe) {
                allowChase = false;
            }

            if (allowChase) {
                BVec2 v{ BS.units[ei].pos.x - u.pos.x, BS.units[ei].pos.y - u.pos.y }; BNorm(v);
                float rushSpd = u.speed * 1.30f;
                if (ed > u.range * RUSH_EDGE_IN && ed < u.range * RUSH_EDGE_OUT) rushSpd = u.speed * 1.65f; // 边缘小冲刺
                BVec2 vel = BScale(v, rushSpd);
                // 降低分离，便于“挤出前线”
                vel = BAdd(vel, SteerSeparationIdx(i, 56.f * VISUAL_SCALE, u.speed * 0.24f));
                u.pos.x += vel.x * dt; u.pos.y += vel.y * dt;
                return;
            }
        }
    }

    // —— 常态：朝锚点移动（锚点已确保在法师前方）
    BVec2 goal{ anchorX - u.pos.x, anchorY - u.pos.y }; BNorm(goal);
    BVec2 vel = BScale(goal, u.speed * (brave ? 1.08f : 0.95f));

    // 勇敢：更紧密一些、抖动更小，便于维持“前排墙”
    vel = BAdd(vel, SteerSeparationIdx(i, 64.f * VISUAL_SCALE, u.speed * (brave ? 0.25f : 0.30f)));
    float jig = std::sin(gBattleTime * 2.3f + i * 0.9f) * (brave ? 0.10f : 0.15f);
    vel.x += jig * 8.0f; vel.y += jig * 4.0f;

    // 超领先回收：如果跑得太前，就把目标往 anchorX 拉一拉
    if (brave && nearMage != -1) {
        float maxLeadX = BS.units[nearMage].pos.x + KEEP_AHEAD + MAX_LEAD_EXTRA;
        if (u.pos.x > maxLeadX) {
            // 在 X 上加一点回拉力（不影响 Y 的编队）
            vel.x += (anchorX - u.pos.x) * 0.8f; // 柔性弹簧
        }
    }

    vel = BLimited(vel, u.speed * (brave ? 1.10f : 1.0f));
    u.pos.x += vel.x * dt; u.pos.y += vel.y * dt;
}


static int MagePickTarget(const BUnit& m)
{
    int best = -1; float bestScore = -1e9f;
    for (int i = 0; i < (int)BS.units.size(); ++i) {
        const auto& e = BS.units[i];
        if (!e.alive || e.team != BTeam::Enemy) continue;
        if (e.kind == BKind::EnemyInvisible && !e.revealed) continue;
        float w = 1.0f;
        switch (e.kind) {
        case BKind::EnemyRunner:   w = 2.1f; break;
        case BKind::EnemyInvisible:w = 1.8f; break;
        case BKind::EnemyBoss:     w = 1.5f; break;
        case BKind::EnemySummoner: w = 1.4f; break;
        case BKind::EnemyArcher:   w = 1.2f; break;
        case BKind::EnemyTank:     w = 1.1f; break;
        default:                   w = 1.0f; break;
        }
        float d = Battle::dist(m.pos, e.pos);
        float sc = w * (1.0f / (0.001f + d));
        if (sc > bestScore) { bestScore = sc; best = i; }
    }
    return best;
}

static std::pair<int, float> NearestAllyOfKind(BKind kind, const BVec2& from)
{
    int idx = -1; float best = 1e9f;
    for (int i = 0; i < (int)BS.units.size(); ++i) {
        const auto& u = BS.units[i];
        if (!u.alive || u.team != BTeam::Ally) continue;
        if (u.kind != kind) continue;
        float d = Battle::dist(u.pos, from);
        if (d < best) { best = d; idx = i; }
    }
    return { idx, best };
}

// ====== 友军选最近敌人（忽略未显形的隐形者） ======
static std::pair<int, float> NearestEnemyVisible(const BVec2& from)
{
    int idx = -1; float best = 1e9f;
    for (int i = 0; i < (int)BS.units.size(); ++i) {
        const auto& u = BS.units[i];
        if (!u.alive || u.team != BTeam::Enemy) continue;
        if (u.kind == BKind::EnemyInvisible && !u.revealed) continue;
        float d = Battle::dist(u.pos, from);
        if (d < best) { best = d; idx = i; }
    }
    return { idx, best };
}

static int FindThreatNear(const BVec2& protectPos, float radius)
{
    int best = -1; float bestD = 1e9f;
    for (int i = 0; i < (int)BS.units.size(); ++i) {
        const auto& e = BS.units[i];
        if (!e.alive || e.team != BTeam::Enemy) continue;
        if (e.kind == BKind::EnemyInvisible && !e.revealed) continue;
        float d = Battle::dist(e.pos, protectPos);
        if (d < radius && d < bestD) { bestD = d; best = i; }
    }
    return best;
}

// 【★】全场最低HP友军
static int FindLowestHpAlly() {
    int idx = -1; float minHp = 1e9f;
    for (int i = 0; i < (int)BS.units.size(); ++i) {
        const auto& a = BS.units[i];
        if (!a.alive || a.team != BTeam::Ally) continue;
        if (a.hp < minHp) { minHp = a.hp; idx = i; }
    }
    return idx;
}

// 【★】智能敌人选目标（按兵种逻辑）
static void SmartPickTarget(const BUnit& self, int& outIndex, bool& outTower, BVec2& outPos)
{
    outIndex = -1; outTower = false; outPos = { 0,0 };
    auto pickHighestHpAllyIn = [&](float senseR)->int {
        int idx = -1; float bestHp = -1e9f;
        for (int i = 0; i < (int)BS.units.size(); ++i) {
            const auto& a = BS.units[i];
            if (!a.alive || a.team != BTeam::Ally) continue;
            if (senseR > 0 && Battle::dist(self.pos, a.pos) > senseR) continue;
            if (a.hp > bestHp) { bestHp = a.hp; idx = i; }
        }
        return idx;
        };
    auto pickLowestHpAlly = [&]()->int {
        int idx = -1; float bestHp = 1e9f;
        for (int i = 0; i < (int)BS.units.size(); ++i) {
            const auto& a = BS.units[i];
            if (!a.alive || a.team != BTeam::Ally) continue;
            if (a.hp < bestHp) { bestHp = a.hp; idx = i; }
        }
        return idx;
        };
    auto pickNearestAlly = [&]()->int {
        int idx = -1; float best = 1e9f;
        for (int i = 0; i < (int)BS.units.size(); ++i) {
            const auto& a = BS.units[i];
            if (!a.alive || a.team != BTeam::Ally) continue;
            float d = Battle::dist(self.pos, a.pos);
            if (d < best) { best = d; idx = i; }
        }
        return idx;
        };

    switch (self.kind)
    {
    case BKind::EnemyBoss:
        // 【改】BOSS 改为“最近友军优先”，若无友军才打塔
    {
        int idx = pickNearestAlly();
        if (idx != -1) { outIndex = idx; outTower = false; outPos = BS.units[idx].pos; return; }
        outIndex = -1; outTower = true; outPos = BS.tower.pos; return;
    }
    case BKind::EnemyRunner: {
        int idx = pickHighestHpAllyIn(RUNNER_SENSE_R);
        if (idx == -1) idx = pickNearestAlly();
        if (idx != -1) { outIndex = idx; outTower = false; outPos = BS.units[idx].pos; return; }
        outIndex = -1; outTower = true; outPos = BS.tower.pos; return;
    }
    case BKind::EnemyInvisible: {
        int idx = pickLowestHpAlly();
        if (idx != -1) { outIndex = idx; outTower = false; outPos = BS.units[idx].pos; return; }
        outIndex = -1; outTower = true; outPos = BS.tower.pos; return;
    }
    default: {
        int idx = pickNearestAlly();
        if (idx != -1) { outIndex = idx; outTower = false; outPos = BS.units[idx].pos; return; }
        outIndex = -1; outTower = true; outPos = BS.tower.pos; return;
    }
    }
}

static BUnit MakeEnemy(BKind kind)
{
    BUnit u; u.team = BTeam::Enemy; u.atkT = Battle::frand(0, 1.f);
    u.pos = { BS.L + BS.Wf * 0.95f, BS.T + Battle::frand(BS.Hf * 0.1f, BS.Hf * 0.9f) };

    const int wave = std::max(1, BS.wave);

    switch (kind)
    {
    case BKind::EnemyMinion:
        u.kind = BKind::EnemyMinion; u.maxhp = u.hp = 10;
        u.atk = 2; u.range = 50 * VISUAL_SCALE; u.atkCD = 1.0f; u.speed = 80; u.tex = "xb.tga";
        u.maxhp *= WaveLinear(wave, 0.15f); u.hp = u.maxhp;
        break;

    case BKind::EnemyRunner:
        u.kind = BKind::EnemyRunner; u.maxhp = u.hp = 50;
        u.walkSpd = 120; u.chargeSpd = 500; u.curSpd = u.walkSpd;
        u.atk = 5; u.range = 60 * VISUAL_SCALE; u.atkCD = 0.7f;
        u.chargeCD = 3; u.chargeT = Battle::frand(0, 3);
        u.speed = 120; u.tex = "ccs.tga";
        u.atk *= WaveLinear(wave, 0.15f); u.maxhp *= WaveLinear(wave, 0.15f); u.hp = u.maxhp;
        break;

    case BKind::EnemyArcher:
        u.kind = BKind::EnemyArcher; u.maxhp = u.hp = 10;
        u.atk = 2; u.range = 250 * VISUAL_SCALE; u.atkCD = 2.0f; u.speed = 85; u.tex = "arrowman.tga";
        u.atk *= WaveLinear(wave, 0.15f); u.maxhp *= WaveLinear(wave, 0.15f); u.hp = u.maxhp;
        break;

    case BKind::EnemyInvisible:
        u.kind = BKind::EnemyInvisible; u.maxhp = u.hp = 20;
        u.atk = 15; u.range = 120 * VISUAL_SCALE; u.atkCD = 2.0f; u.speed = 100; u.tex = "yin.tga";
        u.invisible = true; u.revealed = false; u.atk *= WaveLinear(wave, 0.10f);
        break;

    case BKind::EnemySummoner:
        u.kind = BKind::EnemySummoner; u.maxhp = u.hp = 100;
        u.atk = 0; u.range = 0; u.atkCD = 0; u.speed = 50; u.tex = "summoner.tga";
        u.summonCD = 7.0f; u.summonT = Battle::frand(1.0f, 5.0f);
        u.maxhp *= WaveLinear(wave, 0.15f); u.hp = u.maxhp;
        break;

    case BKind::EnemyMeat:
        u.kind = BKind::EnemyMeat; u.maxhp = u.hp = 70;
        u.atk = 10; u.range = 100 * VISUAL_SCALE; u.atkCD = 10.0f; u.speed = 60; u.tex = "meat.tga";
        u.maxhp *= WaveLinear(wave, 0.15f); u.hp = u.maxhp;
        break;

    case BKind::EnemySword:
        u.kind = BKind::EnemySword; u.maxhp = u.hp = 10;
        u.atk = 20; u.range = 100 * VISUAL_SCALE; u.atkCD = 10.0f; u.speed = 1000; u.tex = "sword.tga";
        u.suicideAfterAttack = true;
        u.atk *= WaveLinear(wave, 0.15f);
        break;

    case BKind::EnemyTank:
        u.kind = BKind::EnemyTank; u.maxhp = u.hp = 500;
        u.atk = 10; u.range = 70 * VISUAL_SCALE; u.atkCD = 3.0f; u.speed = 60; u.tex = "tk.tga";
        u.pos = { BS.L + BS.Wf * 0.6f, BS.T + BS.Hf * 0.5f };
        break;

    case BKind::EnemyBoss:
        u.kind = BKind::EnemyBoss; u.maxhp = u.hp = 1500;
        u.atk = 15; u.range = 150 * VISUAL_SCALE; u.atkCD = 1.7f; u.speed = 60; u.tex = "boss.tga";
        u.maxhp *= WaveLinear(wave, 0.15f); u.atk *= WaveLinear(wave, 0.10f); u.hp = u.maxhp;
        // 初始化技能计时随即打散
        u.skill1T = Battle::frand(3.0f, 7.0f);
        u.skill2T = Battle::frand(5.0f, 9.0f);
        u.skill3T = Battle::frand(7.0f, 11.0f);
        break;

    default:
        u.kind = BKind::EnemyMinion; u.maxhp = u.hp = 10;
        u.atk = 2; u.range = 50 * VISUAL_SCALE; u.atkCD = 1.0f; u.speed = 80; u.tex = "xb.tga"; break;
    }
    return u;
}

static int SummonEnemy(BKind kind, const BVec2& p)
{
    BUnit u = MakeEnemy(kind);
    u.pos = p;
    BS.units.push_back(u);
    return (int)BS.units.size() - 1;
}

// ===== HUD/背景 =====
static void DrawRectBorder(const BVec2& center, float w, float h, float thickness = 6.0f)
{
    DrawSolidQuad({ center.x, center.y - h * 0.5f + thickness * 0.5f }, w, thickness, 0, 0, 0, 1);
    DrawSolidQuad({ center.x, center.y + h * 0.5f - thickness * 0.5f }, w, thickness, 0, 0, 0, 1);
    DrawSolidQuad({ center.x - w * 0.5f + thickness * 0.5f, center.y }, thickness, h, 0, 0, 0, 1);
    DrawSolidQuad({ center.x + w * 0.5f - thickness * 0.5f, center.y }, thickness, h, 0, 0, 0, 1);
}
static void DrawFullScreenBG(const char* preferred, const char* fallback = nullptr)
{
    const float W = (float)Direct3D_GetBackBufferWidth();
    const float H = (float)Direct3D_GetBackBufferHeight();

    unsigned int tex = 0;
    if (preferred && *preferred) tex = LoadIconCached_Battle(preferred);
    if (tex == 0 && fallback && *fallback) tex = LoadIconCached_Battle(fallback);
    if (tex == 0) return;

    Object_2D o(0, 0, W, H);
    o.Transform.Position = { 0, 0 };
    o.texNo = tex;
    o.Color = { 1,1,1,1 };
    BoxVertexsMake(&o);
    DrawObject_2D(&o);
}

// ===== 初始化/生成友军（原逻辑保留，略去注释） =====
extern int GetBuildingLevel(BuildingType type);
static void PullBuildingAndSpawnAllies()
{
    const int lvSaint = HighestLevelInGrid(gBuild, RTS9::BuildingType::SaintHouse);
    const int lvShield = HighestLevelInGrid(gBuild, RTS9::BuildingType::ShieldHouse);
    const int lvMage = HighestLevelInGrid(gBuild, RTS9::BuildingType::MageHouse);

    BS.healAmount = (lvSaint == 0 ? 1 : (lvSaint == 1 ? 5 : (lvSaint == 2 ? 10 : 20)));
    BS.saintHP = (lvSaint == 0 ? 50 : (lvSaint == 1 ? 70 : (lvSaint == 2 ? 100 : 100)));
    BS.saintTex = "player.tga";
    BS.healRadius = (120.0f + 50.0f * lvSaint) * VISUAL_SCALE;
    BS.healCD = 3.0f;

    BS.saintDamageOnPulse = 0;
    if (lvSaint >= 4) {
        BS.healAmount *= 2;
        BS.healRadius *= 1.3f;
        BS.saintDamageOnPulse = BS.healAmount / 3;
        BS.healCD = 2.0f;
    }

    switch (lvSaint) {
    case 1: BS.tower.maxhp = 220; BS.tower.cd = 2.5f; BS.tower.atk = 25; BS.towerDrawScale = 1.1f; break;
    case 2: BS.tower.maxhp = 270; BS.tower.cd = 2.2f; BS.tower.atk = 30; BS.towerDrawScale = 1.2f; break;
    case 3: BS.tower.maxhp = 330; BS.tower.cd = 1.8f; BS.tower.atk = 35; BS.towerDrawScale = 1.2f; break;
    case 4: BS.tower.maxhp = 500; BS.tower.cd = 1.5f; BS.tower.atk = 50; BS.towerDrawScale = 1.5f; break;
    default: break;
    }
    BS.tower.hp = BS.tower.maxhp;

    const int spawnMage = gBuild.res.mageUnits;
    const int spawnShield = gBuild.res.shieldUnits;

    // 圣女
    {
        BUnit u;
        u.team = BTeam::Ally; u.kind = BKind::Saint;
        u.pos = { BS.tower.pos.x, BS.tower.pos.y + BS.Hf * 0.12f };
        u.maxhp = BS.saintHP; u.hp = u.maxhp;
        u.speed = 170;
        u.tex = BS.saintTex;
        BS.units.push_back(u);
        ClampUnitToField(BS.units.back());
        BS.saint = (int)BS.units.size() - 1;
    }

    // 法师
    BS.heroMageActive = (lvMage >= 4);
    if (BS.heroMageActive) {
        const int n = std::max(1, spawnMage);
        BUnit u;
        u.team = BTeam::Ally; u.kind = BKind::Mage; u.level = 4;
        u.maxhp = 50.0f * n; u.hp = u.maxhp;
        u.atk = 30.0f + 2.0f * n;
        u.range = 700.0f * VISUAL_SCALE;
        u.speed = 100.0f;
        u.atkCD = std::max(0.05f, 0.3f + 3.0f / n);
        u.atkT = Battle::frand(0, u.atkCD);
        u.attackTargets = 3;
        u.tex = "magicman4.tga";
        u.pos = { BS.L + BS.Wf * 0.12f, BS.T + Battle::frand(BS.Hf * 0.2f, BS.Hf * 0.8f) };
        BS.units.push_back(u);
    }
    else {
        for (int i = 0; i < spawnMage; ++i) {
            BUnit u;
            u.team = BTeam::Ally; u.kind = BKind::Mage;
            const int lv = std::max(1, lvMage);
            if (lv == 1) { u.maxhp = 10;  u.atk = 5;  u.tex = "magicman1.tga"; }
            if (lv == 2) { u.maxhp = 15;  u.atk = 10; u.tex = "magicman2.tga"; }
            if (lv == 3) { u.maxhp = 30;  u.atk = 25; u.tex = "magicman3.tga"; }
            u.hp = u.maxhp; u.range = 300.0f * VISUAL_SCALE; u.atkCD = 2.0f; u.atkT = Battle::frand(0, u.atkCD);
            u.speed = 120;
            u.pos = { BS.L + Battle::frand(0, BS.Wf * 0.10f), BS.T + Battle::frand(BS.Hf * 0.1f, BS.Hf * 0.9f) };
            BS.units.push_back(u);
        }
    }

    // 盾兵
    BS.heroShieldActive = (lvShield >= 4);
    if (BS.heroShieldActive) {
        const int n = std::max(1, spawnShield);
        BUnit u;
        u.team = BTeam::Ally; u.kind = BKind::Shield; u.level = 4;
        u.maxhp = 150.0f * n; u.hp = u.maxhp;
        u.atk = 25.0f + n;
        u.range = 100.0f * VISUAL_SCALE;
        u.atkCD = 1.0f; u.atkT = Battle::frand(0, u.atkCD);
        u.speed = 120.0f;
        u.tex = "sheldman4.tga";
        u.pos = { BS.L + BS.Wf * 0.22f, BS.T + Battle::frand(BS.Hf * 0.2f, BS.Hf * 0.8f) };
        BS.units.push_back(u);
    }
    else {
        for (int i = 0; i < spawnShield; ++i) {
            BUnit u;
            u.team = BTeam::Ally; u.kind = BKind::Shield;
            const int lv = std::max(1, lvShield);
            if (lv == 1) { u.maxhp = 20;  u.atk = 2;  u.tex = "sheldman1.tga"; }
            if (lv == 2) { u.maxhp = 40;  u.atk = 3;  u.tex = "sheldman2.tga"; }
            if (lv == 3) { u.maxhp = 100; u.atk = 10; u.tex = "sheldman3.tga"; }
            u.hp = u.maxhp; u.range = 60 * VISUAL_SCALE; u.atkCD = 1.0f; u.atkT = Battle::frand(0, u.atkCD);
            u.speed = 60;
            u.pos = { BS.L + Battle::frand(BS.Wf * 0.20f, BS.Wf * 0.40f), BS.T + Battle::frand(BS.Hf * 0.1f, BS.Hf * 0.9f) };
            BS.units.push_back(u);
        }
    }
}

// ====== 波次/奖励（原逻辑保持） ======
static void SpawnWave() {
    int budget = BS.difficulty;
    int minionCount = 0;
    const int minionCap = 12;
    BS.bossAnnounced = false;

    if (BS.wave == 1 && !BS.wave1BonusSpawned) {
        BS.wave1BonusSpawned = true;
    }

    struct Opt { BKind kind; int cost; float baseW; };
    auto pickOne = [&](int budget)->Opt {
        std::vector<Opt> opts;
        if (budget >= 1 && minionCount < minionCap) opts.push_back({ BKind::EnemyMinion, 1, 0.6f });
        if (budget >= 2)  opts.push_back({ BKind::EnemyArcher, 2, 0.8f });
        if (budget >= 12) opts.push_back({ BKind::EnemyRunner, 5, 1.8f });
        if (budget >= 15) opts.push_back({ BKind::EnemyTank, 12, 1.5f });
        if (budget >= 7)  opts.push_back({ BKind::EnemyInvisible, 7, 1.0f });
        if (budget >= 20) opts.push_back({ BKind::EnemySummoner, 20, 0.7f });
        if (budget >= 35) opts.push_back({ BKind::EnemyBoss, 35, 1.0f });

        if (opts.empty()) return Opt{ BKind::EnemyMinion, 9999, 0.0f };

        float totalW = 0.0f;
        std::vector<float> ws; ws.reserve(opts.size());
        for (auto& o : opts) {
            float w = o.baseW * std::sqrt((float)budget / (float)o.cost);
            if (o.kind == BKind::EnemyMinion && (float)minionCount >= minionCap * 0.7f) w *= 0.2f;
            totalW += w; ws.push_back(w);
        }
        std::uniform_real_distribution<float> d(0.0f, totalW);
        float r = d(Battle::RNG());
        float acc = 0.0f;
        for (size_t i = 0; i < opts.size(); ++i) { acc += ws[i]; if (r <= acc) return opts[i]; }
        return opts.back();
        };

    while (budget > 0) {
        Opt o = pickOne(budget);
        if (o.cost > budget || o.cost >= 9999) break;
        BUnit en = MakeEnemy(o.kind);
        BS.units.push_back(en);
        if (o.kind == BKind::EnemyBoss && !BS.bossAnnounced) { TriggerBossBanner(); BS.bossAnnounced = true; }
        budget -= o.cost;
        if (o.kind == BKind::EnemyMinion) ++minionCount;
    }

    BS.wave++;
    BS.difficulty += 2 + BS.wave * 2;
}

static void GrantWaveRewards()
{
    int addGold = 0;
    int addUnits = 0;
    for (const auto& t : gBuild.grid) {
        const auto& b = t.b;
        if (b.level <= 0) continue;
        if (b.type == RTS9::BuildingType::Gold)         addGold += (b.level == 1 ? 10 : b.level == 2 ? 20 : 40);
        else if (b.type == RTS9::BuildingType::Recruit) addUnits += (b.level == 1 ? 1 : b.level == 2 ? 2 : 4);
    }
    addGold += 20; addUnits += 1;
    gBuild.res.coins += addGold;
    gBuild.res.units += addUnits;
}

void InitializeGameBattle(void)
{
    RECT rc; GetClientRect(*GethWnd(), &rc);
    BS.W = float(rc.right - rc.left);
    BS.H = float(rc.bottom - rc.top);

    BS.Wf = BS.W * 0.77f * VISUAL_SCALE;
    BS.Hf = BS.Wf / 3.0f;
    if (BS.Hf > BS.H * 0.90f) { BS.Hf = BS.H * 0.90f; BS.Wf = BS.Hf * 3.0f; }

    BS.L = -BS.Wf * 0.5f;
    BS.T = -BS.Hf * 0.5f;

    BS.tower.pos = { BS.L + BS.Wf * 0.07f, BS.T + BS.Hf * 0.5f };
    BS.tower.hp = BS.tower.maxhp = 200;
    BS.tower.atk = 20; BS.tower.cd = 3; BS.tower.t = 0;
    BS.towerDrawScale = 1.0f;

    BS.units.clear(); BS.bullets.clear(); BS.effects.clear();
    BS.vfx.clear(); BS.slows.clear(); BS.domes.clear();
    gFloatNums.clear(); gDeaths.clear();

    gShakeT = gShakeDur = gShakeAmp = 0.0f;
    gBossBanner = BossBanner::None; gBossBannerT = 0.0f;
    gFlashT = gFlashDur = gFlashA = 0.0f;

    PullBuildingAndSpawnAllies();
    AssignLanesAndSlots();

    BS.wave = (gNextWave <= 0 ? 1 : gNextWave);
    BS.difficulty = (gNextDifficulty <= 0 ? 4 : gNextDifficulty);
    BS.wave1BonusSpawned = false;
    SpawnWave();

    BS.healT = 0;
    BS.running = true;
}

// 【改】统一的翻转尝试函数（带0.5s冷却）
static inline void TryFace(BUnit& u, bool wantRight, float dt) {
    if (u.faceLockT > 0.0f) {
        u.faceLockT -= dt;
        if (u.faceLockT < 0.0f) u.faceLockT = 0.0f;
    }
    if (u.faceLockT <= 0.0f && u.facingRight != wantRight) {
        u.facingRight = wantRight;
        u.faceLockT = FACE_TURN_CD;
        u.faceFlipT = 0.18f;   // 【新增】翻转瞬间启动“纸片挤压”动画
    }

}

void UpdateGameBattle(void)
{
    const float dt = Dt();
    gBattleTime += dt;

    if (gShakeT > 0.0f) { gShakeT -= dt; if (gShakeT < 0.0f) gShakeT = 0.0f; }
    if (gFlashT > 0.0f) { gFlashT -= dt; if (gFlashT < 0.0f) gFlashT = 0.0f; }

    if (gBossBanner != BossBanner::None) {
        gBossBannerT += dt;
        if (gBossBanner == BossBanner::FadingIn && gBossBannerT >= gBossInDur) { gBossBanner = BossBanner::Hold; gBossBannerT = 0.0f; }
        else if (gBossBanner == BossBanner::Hold && gBossBannerT >= gBossHoldDur) { gBossBanner = BossBanner::FadingOut; gBossBannerT = 0.0f; }
        else if (gBossBanner == BossBanner::FadingOut && gBossBannerT >= gBossOutDur) { gBossBanner = BossBanner::None; gBossBannerT = 0.0f; }
    }

    if (gWinPhase != WinPhase::None) {
        gWinT += dt;
        if (gWinPhase == WinPhase::FadingIn && gWinT >= gWinIn) { gWinPhase = WinPhase::Hold; gWinT = 0.0f; }
        else if (gWinPhase == WinPhase::Hold && gWinT >= gWinHold) { gWinPhase = WinPhase::FadingOut; gWinT = 0.0f; }
        else if (gWinPhase == WinPhase::FadingOut && gWinT >= gWinOut) { gWinPhase = WinPhase::None; gWinT = 0.0f; SetcurrentScene(GameScene::Ready); return; }
        return;
    }

    // 安全恢复
    {
        bool haveEnemy = false;
        for (const auto& u : BS.units) { if (u.alive && u.team == BTeam::Enemy) { haveEnemy = true; break; } }
        if (!BS.running && gFailPhase == FailPhase::None && gWinPhase == WinPhase::None) {
            if (haveEnemy) BS.running = true;
        }
    }

    // 失败过场推进
    if (!BS.running) {
        if (gFailPhase != FailPhase::None) {
            if (gFailPhase == FailPhase::FadingOut) {
                gFailT += dt;
                if (gFailT >= gFailDur) {
                    gBuild = RTS9::GameState{};
                    gNextWave = 1; gNextDifficulty = 5;
                    BS.wave = 1;  BS.difficulty = 5;
                    gFailPhase = FailPhase::FadingIn; gFailT = 0.0f;
                }
            }
            else if (gFailPhase == FailPhase::FadingIn) {
                gFailT += dt;
                if (gFailT >= gFailDur) { gFailPhase = FailPhase::None; SetcurrentScene(GameScene::Ready); }
            }
        }
        return;
    }

    // 圣女移动
    BVec2 dir{ 0,0 };
    if (kbd.KeyIsPressed('A') || kbd.KeyIsPressed(VK_LEFT))  dir.x -= 1;
    if (kbd.KeyIsPressed('D') || kbd.KeyIsPressed(VK_RIGHT)) dir.x += 1;
    if (kbd.KeyIsPressed('W') || kbd.KeyIsPressed(VK_UP))    dir.y -= 1;
    if (kbd.KeyIsPressed('S') || kbd.KeyIsPressed(VK_DOWN))  dir.y += 1;
    if (BS.saint >= 0) {
        BUnit& s = BS.units[BS.saint];
        s.pos.x += dir.x * s.speed * dt;
        s.pos.y += dir.y * s.speed * dt;
        s.pos.x = Battle::clamp(s.pos.x, BS.L, BS.L + BS.Wf);
        s.pos.y = Battle::clamp(s.pos.y, BS.T, BS.T + BS.Hf);
    }

    // 圣女脉冲治疗（原逻辑 + 伤害）
    BS.healT -= dt;
    if (BS.saint >= 0 && BS.healT <= 0.0f) {
        BVec2 c = BS.units[BS.saint].pos;
        for (auto& u : BS.units) {
            if (!u.alive) continue;
            if (u.team == BTeam::Ally && Battle::dist(u.pos, c) <= BS.healRadius) {
                float before = u.hp;
                u.hp = (std::min)(u.maxhp, u.hp + float(BS.healAmount));
                int healed = (int)std::round(u.hp - before);
                if (healed > 0) { AddFloatNumber(u.pos, healed, true); u.healFlashT = 0.25f; }
            }
        }
        if (BS.saintDamageOnPulse > 0) {
            for (auto& e : BS.units) {
                if (!e.alive || e.team != BTeam::Enemy) continue;
                if (Battle::dist(e.pos, c) <= BS.healRadius) {
                    int dmg = BS.saintDamageOnPulse;
                    e.hp -= (float)dmg;
                    AddFloatNumber(e.pos, dmg, false);
                    e.hitFlashT = 0.18f;
                    if (e.hp <= 0 && e.alive) { e.alive = false; AddDeathFx(e.pos); OnUnitDeath(e); }
                }
            }
        }
        // 小小绿光圈
        VfxSpawnRing(c, BS.healRadius * 2.0f, 0.6f, 0.6f, 1.0f, 0.6f, 0.35f);
        BS.healT = BS.healCD;
    }

    // 车道/槽位补位
    static float s_laneReassignT = 0.0f;
    s_laneReassignT -= dt;
    if (s_laneReassignT <= 0.0f) { AssignLanesAndSlots(); s_laneReassignT = 2.0f; }

    // 防御塔攻击（原逻辑 + 光束）
    BS.tower.t += dt;
    if (BS.tower.t >= BS.tower.cd) {
        BS.tower.t = 0;
        int best = -1; float bd = 1e9f;
        for (int i = 0; i < (int)BS.units.size(); ++i) {
            auto& e = BS.units[i]; if (!e.alive || e.team != BTeam::Enemy) continue;
            if (e.kind == BKind::EnemyInvisible && !e.revealed) continue;
            float d = Battle::dist(e.pos, BS.tower.pos);
            if (d < bd) { bd = d; best = i; }
        }
        if (best != -1) {
            auto& e = BS.units[best];
            e.hp -= BS.tower.atk;
            e.hitFlashT = 0.18f;
            AddFloatNumber(e.pos, (int)BS.tower.atk, false);
            if (e.hp <= 0) { e.alive = false; AddDeathFx(e.pos); OnUnitDeath(e); }
            VfxSpawnLaser(BS.tower.pos, e.pos, 0.4f);
        }
    }

    // === 单位 AI & 攻击 ===
    for (int i = 0; i < (int)BS.units.size(); ++i) {
        auto& u = BS.units[i];
        if (!u.alive) continue;

        // Buff计时衰减
        if (u.atkBuffT > 0.0f) { u.atkBuffT -= dt; if (u.atkBuffT <= 0.0f) u.atkCdMul = 1.0f; }
        if (u.spdBuffT > 0.0f) { u.spdBuffT -= dt; if (u.spdBuffT <= 0.0f) u.spdMul = 1.0f; }

        // 敌方行为
        if (u.team == BTeam::Enemy)
        {
            // —— BOSS HP阈值触发三连冲（目标=全场最低HP友军）
            if (u.kind == BKind::EnemyBoss) {
                if (!u.rageTrig66 && u.hp <= u.maxhp * 0.66f) {
                    u.rageTrig66 = true; u.raging = true; u.rageCharges = BOSS_RAMPAGE_CHARGES; u.rageDashT = 0.0f;
                    u.rageHitCount = 0;
                    u.rageHitIdx[0] = u.rageHitIdx[1] = u.rageHitIdx[2] = -1;

                    AddScreenShake(10.0f * VISUAL_SCALE, 0.3f); AddScreenFlash(0.18f, 0.25f);
                }
                if (!u.rageTrig33 && u.hp <= u.maxhp * 0.33f) {
                    u.rageTrig33 = true; u.raging = true; u.rageCharges = BOSS_RAMPAGE_CHARGES; u.rageDashT = 0.0f;
                    u.rageHitCount = 0;
                    u.rageHitIdx[0] = u.rageHitIdx[1] = u.rageHitIdx[2] = -1;

                    AddScreenShake(12.0f * VISUAL_SCALE, 0.35f); AddScreenFlash(0.20f, 0.28f);
                }

                // 三连冲逻辑
                if (u.raging) {
                    if (u.rageDashT <= 0.0f) {
                        if (u.rageCharges <= 0) {
                            u.raging = false;
                            u.rageHitCount = 0; // 【新增】安全复位
                        }

                        else {
                            int targetIdx = -1;
                            float bestHp = 1e9f;
                            // 先选一个没被本轮选过的最低血友军
                            for (int ai = 0; ai < (int)BS.units.size(); ++ai) {
                                const auto& a = BS.units[ai];
                                if (!a.alive || a.team != BTeam::Ally) continue;
                                bool used = false;
                                for (int k = 0; k < u.rageHitCount; ++k) {
                                    if (u.rageHitIdx[k] == ai) { used = true; break; }
                                }
                                if (used) continue;
                                if (a.hp < bestHp) { bestHp = a.hp; targetIdx = ai; }
                            }
                            // 若友军不足（< rageHitCount+1），则退化为原逻辑
                            if (targetIdx == -1) targetIdx = FindLowestHpAlly();

                            // 记录本次目标，供后续去重
                            if (targetIdx >= 0 && u.rageHitCount < 3) {
                                u.rageHitIdx[u.rageHitCount++] = targetIdx;
                            }

                            BVec2 tp = (targetIdx >= 0 ? BS.units[targetIdx].pos : BS.tower.pos);
                            BVec2 dir{ tp.x - u.pos.x, tp.y - u.pos.y }; BNorm(dir);
                            u.rageDir = dir; u.rageDashT = BOSS_RAMPAGE_DASH_DUR;
                            // VFX：起冲
                            VfxSpawnRing(u.pos, 120.0f * VISUAL_SCALE, 0.35f, 1, 0.4f, 0.4f, 0.5f);

                          /*  PlaySFX("juliezhendong.wav", 0.60f, 1.0f);*/

                        }
                    }
                    if (u.rageDashT > 0.0f) {
                        u.pos.x += u.rageDir.x * BOSS_RAMPAGE_SPEED * dt;
                        u.pos.y += u.rageDir.y * BOSS_RAMPAGE_SPEED * dt;

                        bool hitWall = false;
                        if (u.pos.x < BS.L || u.pos.x > BS.L + BS.Wf) { u.rageDir.x = -u.rageDir.x; hitWall = true; }
                        if (u.pos.y < BS.T || u.pos.y > BS.T + BS.Hf) { u.rageDir.y = -u.rageDir.y; hitWall = true; }
                        if (hitWall) { ClampUnitToField(u); AddScreenShake(8.0f * VISUAL_SCALE, 0.2f); }

                        // 碰撞伤害
                        auto collideRadius = [&](const BUnit& uu)->float {
                            switch (uu.kind) {
                            case BKind::EnemyBoss:  return 30.0f;
                            case BKind::EnemyTank:  return 26.0f;
                            case BKind::Shield:     return (uu.level >= 4 ? 20.0f * HERO_VISUAL_SCALE : 20.0f);
                            case BKind::Saint:      return 18.0f;
                            case BKind::Mage:       return (uu.level >= 4 ? 16.0f * HERO_VISUAL_SCALE : 16.0f);
                            default:                return 14.0f;
                            }
                            };
                        float rBoss = collideRadius(u);
                        for (auto& a : BS.units) {
                            if (!a.alive || a.team != BTeam::Ally) continue;
                            float r2 = collideRadius(a);
                            if (Battle::dist(u.pos, a.pos) <= (rBoss + r2)) {
                                a.hp -= BOSS_RAMPAGE_HITDMG;
                                a.hitFlashT = 0.18f;
                                AddFloatNumber(a.pos, (int)BOSS_RAMPAGE_HITDMG, false);
                                if (a.hp <= 0 && a.alive) { a.alive = false; AddDeathFx(a.pos); OnUnitDeath(a); }

                                BVec2 push{ a.pos.x - u.pos.x, a.pos.y - u.pos.y }; BNorm(push);
                                StartKnockback(a, push, 110.0f * VISUAL_SCALE, 0.25f);
                                AddScreenShake(8.0f * VISUAL_SCALE, 0.2f);
                            }
                        }
                        if (Battle::dist(u.pos, BS.tower.pos) <= 80.0f * VISUAL_SCALE) {
                            BS.tower.hp -= BOSS_RAMPAGE_HITDMG;
                            u.rageDir.x = -u.rageDir.x; u.rageDir.y = -u.rageDir.y;
                            AddScreenShake(10.0f * VISUAL_SCALE, 0.25f);
                        }

                        u.rageDashT -= dt;
                        if (u.rageDashT <= 0.0f) {
                            u.rageCharges--;
                            // 冲刺结束爆花
                            VfxSpawnBurst(u.pos, 120.0f * VISUAL_SCALE, 0.35f);
                            VfxSpawnRing(u.pos, 140.0f * VISUAL_SCALE, 0.4f, 1, 0.5f, 0.5f, 0.6f);

                        /*    PlaySFX("baozha.wav", 0.80f, RandPitch(18.0f));*/

                        }
                    }
                }

                // —— BOSS 主动技（不与冲刺冲突过多）
                u.skill1T -= dt; u.skill2T -= dt; u.skill3T -= dt;

                // 地裂重踏（近距AOE+击退）
                if (u.skill1T <= 0.0f && !u.raging) {
                    u.skill1T = u.skill1CD;
                    float R = 160.0f * VISUAL_SCALE; int DMG = 12;
                    for (auto& a : BS.units) {
                        if (!a.alive || a.team != BTeam::Ally) continue;
                        float d = Battle::dist(u.pos, a.pos);
                        if (d <= R) {
                            a.hp -= DMG; a.hitFlashT = 0.18f; AddFloatNumber(a.pos, DMG, false);

                            if (a.hp <= 0 && a.alive) { a.alive = false; AddDeathFx(a.pos); OnUnitDeath(a); }

                            BVec2 push{ a.pos.x - u.pos.x, a.pos.y - u.pos.y }; BNorm(push);
                            StartKnockback(a, push, 120.0f * VISUAL_SCALE, 0.25f);
                        }
                    }
                    VfxSpawnRing(u.pos, R * 2.0f, 0.5f, 1, 0.6f, 0.3f, 0.65f);
                    VfxSpawnSparksRadial(u.pos, 16, 48.0f * VISUAL_SCALE, 0.45f);
                    AddScreenShake(10.0f * VISUAL_SCALE, 0.32f); AddScreenFlash(0.12f, 0.18f);

               /*     PlaySFX("baozha.wav", 0.95f, RandPitch(8.0f));
                    PlaySFX("juliezhendong.wav", 0.75f, 1.0f);*/

                }

                // 棱镜横扫（线性穿透）
                if (u.skill2T <= 0.0f && !u.raging) {
                    u.skill2T = u.skill2CD;
                    // 朝塔或圣女方向
                    BVec2 tp = (BS.saint >= 0 ? BS.units[BS.saint].pos : BS.tower.pos);
                    BVec2 dir{ tp.x - u.pos.x, tp.y - u.pos.y }; BNorm(dir);
                    BVec2 a = u.pos;
                    BVec2 b = { u.pos.x + dir.x * BS.Wf, u.pos.y + dir.y * BS.Hf };
                    float hitR = 28.0f * VISUAL_SCALE; int DMG = 14;
                    for (auto& aUnit : BS.units) {
                        if (!aUnit.alive || aUnit.team != BTeam::Ally) continue;
                        if (SegmentHitsCircle(a, b, aUnit.pos, hitR + 16.0f)) {
                            aUnit.hp -= DMG; aUnit.hitFlashT = 0.18f; AddFloatNumber(aUnit.pos, DMG, false);
                            if (u.kind == BKind::EnemySword) {
                         /*       PlaySFX("jianguang.wav", 0.85f, RandPitch(25.0f));*/
                            }

                            if (aUnit.hp <= 0 && aUnit.alive) { aUnit.alive = false; AddDeathFx(aUnit.pos); OnUnitDeath(aUnit); }

                        }
                    }
                    VfxSpawnLaser(a, b, 0.6f);
                    AddScreenShake(8.0f * VISUAL_SCALE, 0.25f);
                }

                // 陨星雨（敌方AOE弹）
                if (u.skill3T <= 0.0f && !u.raging) {
                    u.skill3T = u.skill3CD;
                    int N = 8;
                    for (int k = 0; k < N; ++k) {
                        BBullet m;
                        m.team = BTeam::Enemy; m.tex = LoadIconCached_Battle("meteor.tga") ? "meteor.tga" : "attack_effecten.tga";
                        float rx = Battle::frand(BS.L + 20.0f, BS.L + BS.Wf - 20.0f);
                        m.pos = { rx, BS.T - 30.0f };
                        m.vel = { 0.0f, 380.0f };
                        m.dmg = 8.0f; m.aoe = 80.0f * VISUAL_SCALE; m.big = false;
                        m.ttl = 3.5f; m.hitRadius = 14.0f * VISUAL_SCALE;
                        BS.bullets.push_back(m);
                    }
                    AddScreenFlash(0.08f, 0.2f);
                }
            }

            // Summoner 原逻辑保留（略）……
            if (u.kind == BKind::EnemySummoner) {
                u.summonT -= dt;
                if (u.summonT <= 0.0f) {
                    bool callSword = (Battle::frand(0.0f, 1.0f) < 0.5f);
                    BKind k = callSword ? BKind::EnemySword : BKind::EnemyMeat;
                    BVec2 sp = { u.pos.x + Battle::frand(-24.0f, 24.0f), u.pos.y + Battle::frand(-24.0f, 24.0f) };
                    SummonEnemy(k, sp);
                    u.summonT = u.summonCD;
                }
                auto [allyIdx, allyDist] = NearestAllyOfKind(BKind::Shield, u.pos);
                if (allyIdx == -1) { auto p = NearestAllyOfKind(BKind::Mage, u.pos); allyIdx = p.first; allyDist = p.second; }
                if (allyIdx != -1 && allyDist < 140.0f * VISUAL_SCALE) {
                    BVec2 v{ u.pos.x - BS.units[allyIdx].pos.x, u.pos.y - BS.units[allyIdx].pos.y }; Battle::norm(v);
                    u.pos.x += v.x * u.speed * dt;
                    u.pos.y += v.y * u.speed * dt;
                }
                else {
                    if (Battle::dist(u.pos, u.wanderTarget) < 8.0f || (u.wanderTarget.x == 0 && u.wanderTarget.y == 0)) {
                        u.wanderTarget = { BS.L + Battle::frand(BS.Wf * 0.2f, BS.Wf * 0.95f),
                                           BS.T + Battle::frand(BS.Hf * 0.12f, BS.Hf * 0.88f) };
                    }
                    BVec2 v{ u.wanderTarget.x - u.pos.x, u.wanderTarget.y - u.pos.y }; Battle::norm(v);
                    u.pos.x += v.x * u.speed * dt;
                    u.pos.y += v.y * u.speed * dt;
                }
            }
            else {
                int tgt = -1; bool tgtTower = false; BVec2 tgtPos{};
                SmartPickTarget(u, tgt, tgtTower, tgtPos);

                float moveSpd = u.speed * u.spdMul;

                if (u.kind == BKind::EnemyRunner) {
                    u.chargeT -= dt;
                    if (u.chargeT <= 0) { u.chargeT = u.chargeCD; u.curSpd = u.chargeSpd; }
                    u.curSpd = (std::max)(u.walkSpd, u.curSpd - 140 * dt);
                    moveSpd = u.curSpd * u.spdMul;
                }

                float d = Battle::dist(u.pos, tgtPos);
                if (u.kind == BKind::EnemyArcher) {
                    float nearB = u.range * 0.70f;
                    float farB = u.range * 1.00f;
                    BVec2 to{ tgtPos.x - u.pos.x, tgtPos.y - u.pos.y }; BNorm(to);
                    if (d > farB) {
                        u.pos.x += to.x * moveSpd * dt; u.pos.y += to.y * moveSpd * dt;
                    }
                    else if (d < nearB) {
                        u.pos.x -= to.x * moveSpd * 0.8f * dt; u.pos.y -= to.y * moveSpd * 0.8f * dt;
                    }
                    else {
                        BVec2 tang{ -to.y, to.x };
                        u.pos.x += tang.x * moveSpd * 0.75f * dt;
                        u.pos.y += tang.y * moveSpd * 0.75f * dt;
                    }
                    BVec2 sep = SteerSeparationIdx(i, 56.f * VISUAL_SCALE, moveSpd * 0.35f);
                    u.pos.x += sep.x * dt; u.pos.y += sep.y * dt;
                }
                else {
                    if (d > u.range * 0.9f) {
                        BVec2 v{ tgtPos.x - u.pos.x, tgtPos.y - u.pos.y }; Battle::norm(v);
                        u.pos.x += v.x * moveSpd * dt; u.pos.y += v.y * moveSpd * dt;
                    }
                    BVec2 sep = SteerSeparationIdx(i, 48.f * VISUAL_SCALE, moveSpd * 0.30f);
                    u.pos.x += sep.x * dt; u.pos.y += sep.y * dt;
                }
            }
        }
        else // 我方
        {
            if (u.kind == BKind::Mage) {
                MageBrainStep(i, u, dt);
            }
            else if (u.kind == BKind::Shield) {
                ShieldBrainStep(i, u, dt);
            }

            // 【★】4级盾兵：超级震退（原逻辑 + VFX）
            if (u.kind == BKind::Shield && u.level >= 4) {
                u.superKnockT -= dt;
                if (u.superKnockT <= 0.0f) {
                    u.superKnockT = u.superKnockCD;
                    int dmg = (int)std::round(u.atk * 0.5f);
                    for (auto& e : BS.units) {
                        if (!e.alive || e.team != BTeam::Enemy) continue;
                        BVec2 push{ e.pos.x - u.pos.x, e.pos.y - u.pos.y }; BNorm(push);
                        StartKnockback(e, push, SHIELD_SUPER_KB_DIST, SHIELD_SUPER_KB_DUR);
                        e.hp -= dmg; e.hitFlashT = 0.18f;
                        AddFloatNumber(e.pos, dmg, false);
                        if (e.hp <= 0) { e.alive = false; AddDeathFx(e.pos); OnUnitDeath(e); }
                    }
                    AddScreenShake(10.0f * VISUAL_SCALE, 0.35f);
                    BS.effects.push_back(BEffect{ u.pos, 0.35f, "attack_effect.tga", true });

                    if (u.kind == BKind::Shield) {
                        // SFX: 剑光斩击
                       /* PlaySFX("jianguang.wav", 0.80f, RandPitch(35.0f));*/
                    }

                    // VFX Combo
                    VfxSpawnRing(u.pos, 220.0f * VISUAL_SCALE, 0.4f, 1, 1, 1, 0.45f, "shock_ring.tga");
                    VfxSpawnSparksRadial(u.pos, 24, 60.0f * VISUAL_SCALE, 0.35f);
                    AddScreenFlash(0.10f, 0.18f);


                    // SFX: 冲击波 + 震动
               /*     PlaySFX("baozha.wav", 0.90f, RandPitch(12.0f));
                    PlaySFX("juliezhendong.wav", 0.70f, 1.0f);*/

                }

                // 4级盾兵大招们：按CD触发
                // 1) 守护穹罩
                u.heroB1 -= dt;
                if (u.heroB1 <= 0.0f) {
                    u.heroB1 = u.heroB1CD;
                    BDome d; d.pos = u.pos; d.r = 150.0f * VISUAL_SCALE; d.t = 0; d.dur = 3.0f; d.alive = true;
                    BS.domes.push_back(d);
                    // 穹罩瞬时治疗微量
                    for (auto& a : BS.units) {
                        if (a.team != BTeam::Ally || !a.alive) continue;
                        if (Battle::dist(a.pos, d.pos) <= d.r) { a.hp = std::min(a.maxhp, a.hp + 8.0f); a.healFlashT = 0.25f; }
                    }
                    VfxSpawnRing(u.pos, d.r * 2.0f, 0.6f, 0.7f, 1.0f, 0.8f, 0.4f, "dome.tga");
                }
                // 2) 大地裂波（直线伤害+击退）
                u.heroB2 -= dt;
                if (u.heroB2 <= 0.0f) {
                    u.heroB2 = u.heroB2CD;
                    // 面向最近敌人
                    auto [ei, ed] = NearestEnemyVisible(u.pos);
                    if (ei != -1) {
                        BVec2 dir{ BS.units[ei].pos.x - u.pos.x, BS.units[ei].pos.y - u.pos.y }; BNorm(dir);
                        BVec2 a = u.pos, b = { u.pos.x + dir.x * (BS.Wf * 0.7f), u.pos.y + dir.y * (BS.Hf * 0.7f) };
                        int DMG = 18; float hitR = 24.0f * VISUAL_SCALE;
                        for (auto& e : BS.units) {
                            if (!e.alive || e.team != BTeam::Enemy) continue;
                            if (SegmentHitsCircle(a, b, e.pos, hitR + 16.0f)) {
                                e.hp -= DMG; e.hitFlashT = 0.18f; AddFloatNumber(e.pos, DMG, false);
                                StartKnockback(e, dir, 140.0f * VISUAL_SCALE, 0.25f);
                                if (e.hp <= 0) { e.alive = false; AddDeathFx(e.pos); OnUnitDeath(e); }
                            }
                        }
                        VfxSpawnLaser(a, b, 0.5f); // 用激光图做裂波线
                        VfxSpawnRing(u.pos, 160.0f * VISUAL_SCALE, 0.35f, 0.9f, 0.9f, 1.0f, 0.5f, "earth_wave.tga");
                        AddScreenShake(7.0f * VISUAL_SCALE, 0.25f);

                       /* PlaySFX("juliezhendong.wav", 0.70f, 1.0f);
                        PlaySFX("baozha.wav", 0.80f, RandPitch(20.0f));*/

                    }
                }
                // 3) 战号加持（攻速Buff）
                u.heroB3 -= dt;
                if (u.heroB3 <= 0.0f) {
                    u.heroB3 = u.heroB3CD;
                    /*PlaySFX("fireballfashe.wav", 0.35f, 0.9f);*/

                    for (auto& a : BS.units) {
                        if (a.team != BTeam::Ally || !a.alive) continue;
                        if (Battle::dist(a.pos, u.pos) <= 220.0f * VISUAL_SCALE) {
                            a.atkCdMul = 0.7f; a.atkBuffT = 3.0f;
                        }
                    }
                    VfxSpawnRing(u.pos, 200.0f * VISUAL_SCALE, 0.5f, 1, 0.95f, 0.7f, 0.45f, "flare.tga");
                }
            }

            // 4级法师大招们
            if (u.kind == BKind::Mage && u.level >= 4) {
                // 链式闪雷：S1
                u.heroS1 -= dt;
                if (u.heroS1 <= 0.0f) {
                    u.heroS1 = u.heroS1CD;
                    // 选最多3个最近敌人
                    std::vector<int> es;
                    for (int j = 0; j < (int)BS.units.size(); ++j) {
                        auto& e = BS.units[j]; if (!e.alive || e.team != BTeam::Enemy) continue;
                        es.push_back(j);
                    }
                    std::sort(es.begin(), es.end(), [&](int a, int b) {
                        return Battle::dist(u.pos, BS.units[a].pos) < Battle::dist(u.pos, BS.units[b].pos);
                        });
                    int cnt = std::min(3, (int)es.size());
                    for (int k = 0; k < cnt; ++k) {
                        int idx = es[k]; auto& e = BS.units[idx];
                        VfxSpawnChain(u.pos, e.pos, 0.4f);
                        e.hp -= 16.0f; e.hitFlashT = 0.18f; AddFloatNumber(e.pos, 16, false);
                        if (e.hp <= 0) { e.alive = false; AddDeathFx(e.pos); OnUnitDeath(e); }
                    }
                    AddScreenFlash(0.08f, 0.14f);
                }
                // 缓时领域：S2
                u.heroS2 -= dt;
                if (u.heroS2 <= 0.0f) {
                    u.heroS2 = u.heroS2CD;
                    BSlow s; s.pos = u.pos; s.r = 200.0f * VISUAL_SCALE; s.t = 0; s.dur = 4.0f; s.alive = true;
                    BS.slows.push_back(s);
                    VfxSpawnRing(u.pos, s.r * 2.0f, 0.6f, 0.7f, 0.8f, 1.0f, 0.45f, "slow_field.tga");
                }
                // 棱镜新星：S3
                u.heroS3 -= dt;
                if (u.heroS3 <= 0.0f) {
                    u.heroS3 = u.heroS3CD;
                    int N = 8;
                    for (int k = 0; k < N; ++k) {
                        float ang = 6.2831853f / N * k;
                        BVec2 dir{ std::cos(ang), std::sin(ang) };
                        BBullet b; b.team = BTeam::Ally;
                        b.tex = LoadIconCached_Battle("prismshot.tga") ? "prismshot.tga" : "fireball.tga";
                        b.pos = u.pos; b.dmg = 10.0f; b.speed = 520.0f; b.ttl = 2.2f; b.hitRadius = 12.0f * VISUAL_SCALE;
                        b.vel = BScale(dir, b.speed);
                        BS.bullets.push_back(b);

                        // SFX: 火球发射
                       /* PlaySFX("fireballfashe.wav", 0.70f, RandPitch(25.0f));*/

                    }
                    VfxSpawnBurst(u.pos, 120.0f * VISUAL_SCALE, 0.35f);
                }
            }
        }

        // 攻击（带攻速Buff）
        if (u.atkCD > 0) {
            u.atkT -= dt;
            if (u.atkT <= 0) {
                if (u.team == BTeam::Enemy) {
                    int tgt = -1; bool tgtTower = false; BVec2 tgtPos{};
                    SmartPickTarget(u, tgt, tgtTower, tgtPos);
                    float d = Battle::dist(u.pos, tgtPos);
                    if (d <= u.range) {
                        if (u.kind == BKind::EnemyArcher) {
                            // 20%三连射
                            bool tri = (Battle::frand(0.0f, 1.0f) < 0.2f);
                            int shots = tri ? 3 : 1;
                            float spread = tri ? (10.0f * 3.1415926f / 180.0f) : ARROW_SPREAD_RD;
                            for (int s = 0; s < shots; ++s) {
                                BBullet b;
                                b.pos = u.pos;
                                b.dmg = u.atk;
                                b.team = u.team;
                                b.tex = "arrow.tga";
                                b.speed = ARROW_SPEED;
                                b.ttl = ARROW_TTL;
                                b.hitRadius = ARROW_HIT_R;
                                BVec2 dir{ tgtPos.x - u.pos.x, tgtPos.y - u.pos.y }; BNorm(dir);
                                float jitter = Battle::frand(-spread, spread);
                                // 三连射左右分担角度
                                if (tri && shots == 3) { jitter = (-spread + (spread * 2.0f) * (float)s / 2.0f); }
                                dir = Rot(dir, jitter);
                                b.vel = BScale(dir, b.speed);
                                BS.bullets.push_back(b);

                                /*PlaySFX("fireballfashe.wav", 0.80f, RandPitch(20.0f));*/

                            }
                        }
                        else {
                            if (tgtTower) {
                                BS.tower.hp -= u.atk;
                                if (u.kind == BKind::EnemyBoss) AddScreenShake(8.0f * VISUAL_SCALE, 0.25f);
                            }
                            else if (tgt >= 0 && tgt < (int)BS.units.size() && BS.units[tgt].alive) {
                                auto& t = BS.units[tgt];
                                float dmg = u.atk;
                                if (u.kind == BKind::EnemyRunner) {
                                    float sp = (u.curSpd - u.walkSpd) / (std::max)(1.0f, (u.chargeSpd - u.walkSpd));
                                    dmg = 5.0f + Battle::clamp(sp, 0.0f, 1.0f) * 10.0f;
                                    u.curSpd = u.walkSpd;
                                }
                                t.hp -= dmg; t.hitFlashT = 0.18f;
                                AddFloatNumber(t.pos, (int)std::round(dmg), false);
                                if (t.hp <= 0) { t.alive = false; AddDeathFx(t.pos); OnUnitDeath(t); }
                                BVec2 dv{ t.pos.x - u.pos.x, t.pos.y - u.pos.y };
                                StartKnockback(t, dv, KNOCKBACK_DIST, KNOCKBACK_TIME);
                                BS.effects.push_back(BEffect{ t.pos, 0.20f, "attack_effecten.tga", true });

                                if (u.kind == BKind::EnemyBoss) {
                                    for (auto& a : BS.units) {
                                        if (!a.alive || a.team != BTeam::Ally) continue;
                                        float dd = Battle::dist(a.pos, t.pos);
                                        if (dd <= 100.0f * VISUAL_SCALE) {
                                            BVec2 push{ a.pos.x - u.pos.x, a.pos.y - u.pos.y }; Battle::norm(push);
                                            StartKnockback(a, push, 90.0f * VISUAL_SCALE, 0.35f);
                                        }
                                    }
                                    AddScreenShake(8.0f * VISUAL_SCALE, 0.25f);
                                }
                            }
                            if (u.kind == BKind::EnemyInvisible && u.invisible && !u.revealed) u.revealed = true;


                            if (u.suicideAfterAttack) { u.alive = false; AddDeathFx(u.pos); OnUnitDeath(u); }
                        }
                        u.atkT += u.atkCD * u.atkCdMul;
                    }
                    else u.atkT = 0.03f;
                }
                else { // 我方
                    auto [ti, d] = NearestEnemyVisible(u.pos);
                    if (ti != -1 && d <= u.range) {
                        auto& t = BS.units[ti];
                        if (u.kind == BKind::Mage) {
                            u.attackCounter++;
                            bool giant = (u.level >= 4 && (u.attackCounter % MAGE_GIANT_TRIGGER) == 0);

                            BVec2 tp = BS.units[ti].pos;
                            BVec2 dir{ tp.x - u.pos.x, tp.y - u.pos.y };
                            Battle::norm(dir);
                            float jitter = Battle::frand(-FIREBALL_SPREAD_RD, FIREBALL_SPREAD_RD);
                            dir = Rot(dir, jitter);

                            BBullet b;
                            b.pos = u.pos;
                            b.team = BTeam::Ally;

                            if (giant) {
                                b.dmg = (float)MAGE_GIANT_DMG;
                                b.aoe = (float)MAGE_GIANT_AOE;
                                b.big = true;
                                b.tex = "fireball_big.tga";
                                AddScreenShake(6.0f * VISUAL_SCALE, 0.20f);
                            }
                            else {
                                b.dmg = u.atk;
                                b.aoe = 0.0f;
                                b.big = false;
                                b.tex = "fireball.tga";
                            }

                            b.speed = FIREBALL_SPEED;
                            b.ttl = FIREBALL_TTL;
                            b.hitRadius = FIREBALL_HIT_R;
                            b.vel = BScale(dir, b.speed);
                            BS.bullets.push_back(b);


                        }
                        else {
                            t.hp -= u.atk; t.hitFlashT = 0.18f;
                            AddFloatNumber(t.pos, (int)u.atk, false);
                            if (t.hp <= 0) { t.alive = false; AddDeathFx(t.pos); OnUnitDeath(t); }
                            BVec2 dv{ t.pos.x - u.pos.x, t.pos.y - u.pos.y };
                            StartKnockback(t, dv, KNOCKBACK_DIST, KNOCKBACK_TIME);
                            BS.effects.push_back(BEffect{ t.pos, 0.18f, "attack_effect.tga", true });
                            if (u.kind != BKind::Mage) ApplyKnockback(BS.units[ti], u.pos, 110.0f * VISUAL_SCALE);
                        }
                        u.atkT += u.atkCD * u.atkCdMul;
                    }
                    else u.atkT = 0.03f;
                }
            }
        }

        // “蹦跳”动画 & 面向方向（统一冷却翻转）
        {
            float dx = u.pos.x - u.lastPos.x;
            float dy = u.pos.y - u.lastPos.y;
            float dlen = std::sqrt(dx * dx + dy * dy);
            u.moving = (dlen > 0.5f);

            // 【改】带CD的材质翻转：每帧尝试，但只在冷却到期时切换
            bool wantRight = (std::abs(dx) > 0.2f ? (dx >= 0.0f) : u.facingRight);
            if (u.team == BTeam::Ally) wantRight = !wantRight;
            TryFace(u, wantRight, dt);

            if (u.moving) {
                float speedFactor = (u.speed > 1.0f ? u.speed : 80.0f) / 80.0f;
                u.animPhase += Dt() * 10.0f * speedFactor;
                if (u.animPhase > 1000.0f) u.animPhase = std::fmod(u.animPhase, 6.2831853f);
            }
            else {
                u.animPhase *= (1.0f - std::min(1.0f, 4.0f * Dt()));
            }
            u.lastPos = u.pos;
        }

        if (u.hitFlashT > 0.0f) u.hitFlashT -= dt;
        if (u.healFlashT > 0.0f) u.healFlashT -= dt;
        if (u.faceFlipT > 0.0f) u.faceFlipT -= dt;   // 【新增】挤压动画计时衰减


        TickKnockback(u, dt);
        ClampUnitToField(u);
        if (u.hp <= 0 && u.alive) { u.alive = false; AddDeathFx(u.pos); OnUnitDeath(u); }

    }

    // —— 单位间轻推开（保持原逻辑） ——（略，与你原版一致）
    auto UnitRadius = [](const BUnit& u)->float {
        switch (u.kind) {
        case BKind::EnemyBoss:  return 30.0f;
        case BKind::EnemyTank:  return 26.0f;
        case BKind::Shield:     return(u.level >= 4 ? 20.0f * HERO_VISUAL_SCALE : 20.0f);
        case BKind::Saint:      return 18.0f;
        case BKind::Mage:       return (u.level >= 4 ? 16.0f * HERO_VISUAL_SCALE : 16.0f);
        case BKind::EnemyRunner:return 16.0f;
        case BKind::EnemyArcher:return 14.0f;
        default:                return 14.0f;
        }
        };
    const int N = (int)BS.units.size();
    for (int i = 0; i < N; ++i) {
        BUnit& a = BS.units[i]; if (!a.alive) continue;
        for (int j = i + 1; j < N; ++j) {
            BUnit& b = BS.units[j]; if (!b.alive) continue;
            float ra = UnitRadius(a), rb = UnitRadius(b);
            float dx = b.pos.x - a.pos.x, dy = b.pos.y - a.pos.y;
            float d2 = dx * dx + dy * dy;
            float minD = ra + rb;
            if (d2 < minD * minD && d2 > 0.0001f) {
                float d = std::sqrt(d2);
                float pen = (minD - d);
                float nx = dx / d, ny = dy / d;
                a.pos.x -= nx * (pen * 0.5f);
                a.pos.y -= ny * (pen * 0.5f);
                b.pos.x += nx * (pen * 0.5f);
                b.pos.y += ny * (pen * 0.5f);
                a.pos.x = Battle::clamp(a.pos.x, BS.L, BS.L + BS.Wf);
                a.pos.y = Battle::clamp(a.pos.y, BS.T, BS.T + BS.Hf);
                b.pos.x = Battle::clamp(b.pos.x, BS.L, BS.L + BS.Wf);
                b.pos.y = Battle::clamp(b.pos.y, BS.T, BS.T + BS.Hf);
            }
        }
    }

    // === 子弹 ===（直线飞行 + 扫段碰撞 + 尾迹 + AOE仅命中时触发）
    for (auto& b : BS.bullets) {
        if (!b.alive) continue;

        // 记录尾点
        b.trail.push_front(b.pos);
        if (b.trail.size() > 6) b.trail.pop_back();

        BVec2 old = b.pos;
        b.pos.x += b.vel.x * Dt();
        b.pos.y += b.vel.y * Dt();
        b.ttl -= Dt();

        if (b.ttl <= 0.0f || OutOfField(b.pos)) { b.alive = false; continue; }

        auto CollideUnit = [&](BUnit& t)->bool {
            if (!t.alive) return false;
            if ((b.team == BTeam::Ally && t.team != BTeam::Enemy) ||
                (b.team == BTeam::Enemy && t.team != BTeam::Ally)) return false;
            if (t.kind == BKind::EnemyInvisible && !t.revealed && b.team == BTeam::Ally) return false;

            float ur = 0.0f;
            switch (t.kind) {
            case BKind::EnemyBoss:  ur = 30.0f; break;
            case BKind::EnemyTank:  ur = 26.0f; break;
            case BKind::Shield:     ur = (t.level >= 4 ? 20.0f * HERO_VISUAL_SCALE : 20.0f); break;
            case BKind::Saint:      ur = 18.0f; break;
            case BKind::Mage:       ur = (t.level >= 4 ? 16.0f * HERO_VISUAL_SCALE : 16.0f); break;
            case BKind::EnemyRunner:ur = 16.0f; break;
            case BKind::EnemyArcher:ur = 14.0f; break;
            default:                ur = 14.0f; break;
            }
            return SegmentHitsCircle(old, b.pos, t.pos, b.hitRadius + ur);
            };

        bool hitSomething = false;
        BVec2 impactPos = b.pos;

        // 撞单位
        for (auto& u : BS.units) {
            if (CollideUnit(u)) {
                impactPos = u.pos;
                u.hp -= b.dmg;
                u.hitFlashT = 0.18f;
                AddFloatNumber(u.pos, (int)b.dmg, /*isHeal*/false);
                if (u.hp <= 0 && u.alive) { u.alive = false; AddDeathFx(u.pos); OnUnitDeath(u); }
                BS.effects.push_back(BEffect{ u.pos, 0.18f, (b.team == BTeam::Ally ? "attack_effect.tga" : "attack_effecten.tga"), true });
                hitSomething = true;
                b.alive = false;
                break;
            }
        }
        // 撞塔（敌方弹）
        if (!hitSomething && b.team == BTeam::Enemy) {
            float towerR = 24.0f * VISUAL_SCALE;
            if (SegmentHitsCircle(old, b.pos, BS.tower.pos, b.hitRadius + towerR)) {
                impactPos = BS.tower.pos;
                BS.tower.hp -= b.dmg;
                hitSomething = true; b.alive = false;
            }
        }


        // === SFX: 命中音 ===
        if (hitSomething) {
            // 我方火球命中
            if (b.team == BTeam::Ally && (b.tex == "fireball.tga" || b.tex == "fireball_big.tga")) {
              /*  PlaySFX("fireballmingzhong.wav", 0.85f, RandPitch(30.0f));*/
            }
            // 敌方陨星命中塔/单位
            if (b.team == BTeam::Enemy && (b.tex == (LoadIconCached_Battle("meteor.tga") ? "meteor.tga" : "attack_effecten.tga"))) {
              /*  PlaySFX("baozha.wav", 0.90f, RandPitch(15.0f));*/
            }
        }



        // AOE命中触发
        if (hitSomething && b.aoe > 0.0f && !b.exploded) {
            // SFX: 巨爆 + 低频震动
          /*  PlaySFX("baozha.wav", 0.95f, RandPitch(10.0f));
            PlaySFX("juliezhendong.wav", 0.65f, 1.0f);*/

            b.exploded = true;
            const float R2 = b.aoe * b.aoe;
            for (auto& e : BS.units) {
                if (!e.alive) continue;
                if ((b.team == BTeam::Ally && e.team != BTeam::Enemy) ||
                    (b.team == BTeam::Enemy && e.team != BTeam::Ally)) continue;
                if (b.team == BTeam::Ally && e.kind == BKind::EnemyInvisible && !e.revealed) continue;

                float dx = e.pos.x - impactPos.x, dy = e.pos.y - impactPos.y;
                if (dx * dx + dy * dy <= R2) {
                    e.hp -= b.dmg;
                    e.hitFlashT = 0.18f;
                    AddFloatNumber(e.pos, (int)b.dmg, false);
                    if (e.hp <= 0 && e.alive) { e.alive = false; AddDeathFx(e.pos); OnUnitDeath(e); }
                }
            }
            // 巨型火球爆裂VFX
            if (b.big) {
                VfxSpawnBurst(impactPos, 200.0f * VISUAL_SCALE, 0.5f);
                VfxSpawnRing(impactPos, MAGE_GIANT_AOE * 2.0f, 0.5f, 1, 0.9f, 0.6f, 0.45f, "rainbow_glow.tga");
                VfxSpawnSparksRadial(impactPos, 24, 56.0f * VISUAL_SCALE, 0.45f);
                AddScreenShake(9.0f * VISUAL_SCALE, 0.28f);
                AddScreenFlash(0.12f, 0.20f);
            }
        }
    }

    // 特效/领域寿命
    for (auto& e : BS.effects) { if (!e.alive) continue; e.life -= Dt(); if (e.life <= 0) e.alive = false; }
    for (auto& v : BS.vfx) {
        if (!v.alive) continue;
        v.t += Dt(); if (v.t >= v.dur) { v.alive = false; continue; }
        v.rot += v.rotSpd * Dt();
    }
    for (auto& s : BS.slows) { if (!s.alive) continue; s.t += Dt(); if (s.t >= s.dur) s.alive = false; }
    for (auto& d : BS.domes) { if (!d.alive) continue; d.t += Dt(); if (d.t >= d.dur) d.alive = false; }

    // 死亡FX寿命
    for (auto& dfx : gDeaths) { if (!dfx.alive) continue; dfx.t += Dt(); if (dfx.t >= dfx.dur) dfx.alive = false; }

    // ==== 飘字推进 ====
    for (auto& f : gFloatNums) {
        if (!f.alive) continue;
        f.t += Dt();
        // 位置更新（初速度向上，带一点阻尼）
        f.pos.x += f.vel.x * Dt();
        f.pos.y += f.vel.y * Dt();
        f.vel.x *= 0.98f;
        f.vel.y *= 0.98f;   // 也可以加轻微浮力/下坠改手感
        if (f.t >= f.dur) f.alive = false;
    }
    // 清理过期飘字，避免越堆越多
    gFloatNums.erase(
        std::remove_if(gFloatNums.begin(), gFloatNums.end(),
            [](const FloatNum& fn) { return !fn.alive; }),
        gFloatNums.end()
    );


    // 失败判定
    bool saintDead = true;
    if (BS.saint >= 0 && BS.saint < (int)BS.units.size()) {
        const auto& s = BS.units[BS.saint];
        saintDead = (!s.alive || s.hp <= 0);
    }
    if (saintDead || BS.tower.hp <= 0) {
        BS.running = false;
        gFailPhase = FailPhase::FadingOut;
        gFailT = 0.0f;
        return;
    }

    // 胜利：敌人清空
    bool anyEnemy = false;
    for (auto& u : BS.units) { if (u.alive && u.team == BTeam::Enemy) { anyEnemy = true; break; } }
    if (!anyEnemy) {
        GrantWaveRewards();
        gNextWave = BS.wave;
        gNextDifficulty = BS.difficulty;
        BS.bullets.clear();
        BS.effects.clear();
        BS.beams.clear();
        BS.vfx.clear();
        BS.slows.clear();
        BS.domes.clear();
        BS.units.clear();
        gFloatNums.clear();
        gDeaths.clear();

        BS.running = false;
        gWinPhase = WinPhase::FadingIn;
        gWinT = 0.0f;
        return;
    }

    // 光束寿命
    for (auto& be : BS.beams) { if (!be.alive) continue; be.life -= Dt(); if (be.life <= 0) be.alive = false; }
}

// ====== 事件：单位死亡（敌方专属Bloodlust & 小兵溅射） ======
static void OnUnitDeath(const BUnit& dead)
{
    // 敌方死亡：附近敌人Bloodlust
    if (dead.team == BTeam::Enemy) {
        for (auto& e : BS.units) {
            if (!e.alive || e.team != BTeam::Enemy) continue;
            if (Battle::dist(e.pos, dead.pos) <= 200.0f * VISUAL_SCALE) {
                e.atkCdMul = 0.7f; e.atkBuffT = 3.0f; e.spdMul = 1.15f; e.spdBuffT = 3.0f;
            }
        }
        VfxSpawnRing(dead.pos, 160.0f * VISUAL_SCALE, 0.5f, 1, 0.3f, 0.3f, 0.35f, "glow.tga");
    }
    // 敌方小兵死亡溅射
    if (dead.team == BTeam::Enemy && dead.kind == BKind::EnemyMinion) {
        float R = 60.0f * VISUAL_SCALE; int DMG = 3;
        for (auto& a : BS.units) {
            if (!a.alive || a.team != BTeam::Ally) continue;
            if (Battle::dist(a.pos, dead.pos) <= R) {
                a.hp -= DMG; a.hitFlashT = 0.12f; AddFloatNumber(a.pos, DMG, false);
                if (a.hp <= 0 && a.alive) { a.alive = false; AddDeathFx(a.pos); OnUnitDeath(a); }

            }
        }
        VfxSpawnBurst(dead.pos, 80.0f * VISUAL_SCALE, 0.3f);
    }
}

// ====== 对外：法师手动巨型火球/普通火球 ======
void Battle_SpawnAllyFireballGiant(float sx, float sy, float dirx, float diry)
{
    BVec2 dir{ dirx, diry };
    float L = std::sqrt(dir.x * dir.x + dir.y * dir.y);
    if (L > 1e-6f) { dir.x /= L; dir.y /= L; }
    else { dir = { 1,0 }; }
    float jitter = Battle::frand(-FIREBALL_SPREAD_RD, FIREBALL_SPREAD_RD);
    dir = Rot(dir, jitter);

    BBullet b;
    b.pos = { sx, sy };
    b.team = BTeam::Ally;
    b.dmg = (float)MAGE_GIANT_DMG;
    b.aoe = (float)MAGE_GIANT_AOE;
    b.big = true;
    b.tex = "fireball_big.tga";
    b.speed = FIREBALL_SPEED;
    b.ttl = FIREBALL_TTL;
    b.hitRadius = FIREBALL_HIT_R;
    b.vel = BScale(dir, b.speed);
    BS.bullets.push_back(b);
   /* PlaySFX("fireballfashe.wav", 0.70f, RandPitch(25.0f));*/

}

void Battle_SpawnAllyFireball(float sx, float sy, float dirx, float diry, float dmg)
{
    BVec2 dir{ dirx, diry };
    float L = std::sqrt(dir.x * dir.x + dir.y * dir.y);
    if (L > 1e-6f) { dir.x /= L; dir.y /= L; }
    else { dir = { 1.0f, 0.0f }; }
    float jitter = Battle::frand(-FIREBALL_SPREAD_RD, FIREBALL_SPREAD_RD);
    dir = Rot(dir, jitter);

    BBullet b;
    b.pos = { sx, sy };
    b.team = BTeam::Ally;
    b.dmg = dmg;
    b.tex = "fireball.tga";
    b.speed = FIREBALL_SPEED;
    b.ttl = FIREBALL_TTL;
    b.hitRadius = FIREBALL_HIT_R;
    b.vel = BScale(dir, b.speed);
    BS.bullets.push_back(b);
}

// 贴图约定
static constexpr const char* TEX_BEAM = "light.tga";
static constexpr const char* TEX_HALO_MAGE = "halo_mage.tga";
static constexpr const char* TEX_HALO_SHIELD = "halo_shield.tga";
static constexpr const char* TEX_HALO_BOSS = "halo_boss.tga";
static constexpr const char* TEX_HALO_FALLBACK = "halo_soft.tga";

// === 光晕尺寸/弹跳可调 ===
static constexpr float HALO_MAGE_GROWTH = 1.4f;
static constexpr float HALO_SHIELD_SCALE = 1.75f;
static constexpr float HALO_BOSS_SCALE = 1.7f;
static constexpr float HALO_BOUNCE_SCALE = 0.06f;
static constexpr float HALO_BOUNCE_Y = 1.0f;

// === 渲染 ===
void DrawGameBattle(void)
{
    gRenderCam = ComputeCamShakeOffset();

    DrawFullScreenBG("bg_01.tga");
    const BVec2 fieldCenter{ BS.L + BS.Wf * 0.5f, BS.T + BS.Hf * 0.5f };
    DrawSpriteCenter("bg_02.tga", fieldCenter, BS.Wf, BS.Hf, 1.0f);
    DrawRectBorder(fieldCenter, BS.Wf, BS.Hf, 6.0f);

    // 防御塔
    float towerBright = (BS.tower.cd > 0 ? Battle::clamp(BS.tower.t / BS.tower.cd, 0.0f, 1.0f) : 1.0f);
    float towerSize = 72.0f * VISUAL_SCALE * (BS.towerDrawScale > 0 ? BS.towerDrawScale : 1.0f);
    DrawSpriteCenter(BS.tower.tex, BS.tower.pos, towerSize, towerSize, 1.0f, towerBright, towerBright, towerBright);

    // 塔血量
    {
        float hNum = (BS.W > 0 ? BS.W : 1600.0f) * 0.02f;
        std::string s = std::to_string((int)std::round(BS.tower.hp)) + "/" + std::to_string((int)std::round(BS.tower.maxhp));
        DrawNumberString_World(s, BS.tower.pos.x, BS.tower.pos.y - 52.0f * VISUAL_SCALE, hNum, 0.95f, 1, 1, 1);
    }

    // 圣女治疗范围圈（旋转）
    if (BS.saint >= 0 && BS.saint < (int)BS.units.size() && BS.units[BS.saint].alive) {
        const BVec2 c = BS.units[BS.saint].pos;
        const float size = BS.healRadius * 2.0f;
        float readyFactor = 1.0f - (BS.healT / BS.healCD);
        readyFactor = Battle::clamp(readyFactor, 0.0f, 1.0f);
        float alpha = 0.48f * (0.7f + 0.3f * readyFactor);
        float angle = gBattleTime * 0.8f;
        DrawSpriteCenterRot("fanwei.tga", c, size, size, angle, alpha, readyFactor, readyFactor, readyFactor);
    }

    // 单位
    for (int i = 0; i < (int)BS.units.size(); ++i) {
        const auto& u = BS.units[i];
        if (!u.alive) continue;

        float a = 1.0f, r = 1.0f, g = 1.0f, b = 1.0f;

        if (u.kind == BKind::EnemyInvisible && !u.revealed) {
            float shimmer = 0.85f + 0.15f * std::sin(gBattleTime * 8.0f + i * 1.3f);
            a = gInvisibleAlpha * shimmer;
        }

        float baseW = 48.0f * VISUAL_SCALE;
        float baseH = 48.0f * VISUAL_SCALE;
        if (u.team == BTeam::Enemy && u.kind == BKind::EnemyTank) {
            baseW *= TANK_VISUAL_SCALE; baseH *= TANK_VISUAL_SCALE;
        }
        else if (u.team == BTeam::Enemy && u.kind == BKind::EnemyBoss) {
            baseW *= BOSS_VISUAL_SCALE; baseH *= BOSS_VISUAL_SCALE;
        }
        if (u.team == BTeam::Ally && u.level >= 4) {
            baseW *= HERO_VISUAL_SCALE; baseH *= HERO_VISUAL_SCALE;
        }
        float scaleX = 1.0f, scaleY = 1.0f, yoff = 0.0f;
        if (u.moving) {
            float k = 0.5f * (1.0f + std::sin(u.animPhase));
            scaleY = 1.0f + 0.12f * k;
            scaleX = 1.0f - 0.08f * k;
            yoff = (6.0f * VISUAL_SCALE) * k;
        }
        // 【新增】翻转“纸片挤压”动效（横向收缩、纵向微拉）
        if (u.faceFlipT > 0.0f) {
            float dur = 0.18f;
            float t = 1.0f - clamp01(u.faceFlipT / dur);     // 0→1
            float k = std::sin(t * 3.1415926f);              // 半个正弦，柔和起落
            scaleX *= (1.0f - 0.36f * k);                      // 横向最多缩 28%
            scaleY *= (1.0f + 0.27f * k);                      // 纵向最多拉 18%
        }

        if (u.kbTimer > 0.0f && u.kbDur > 0.0f) {
            float p = u.kbTimer / u.kbDur;
            float s = 0.12f * p;
            scaleX *= (1.0f + s);
            scaleY *= (1.0f - s);
        }

        float hopK = (u.moving ? 0.5f * (1.0f + std::sin(u.animPhase)) : 0.0f);
        float haloScaleBounce = 1.0f + HALO_BOUNCE_SCALE * hopK;
        float haloY = u.pos.y - yoff * HALO_BOUNCE_Y;

        // 受击/治疗亮层
        if (u.hitFlashT > 0.0f) {
            float k = clamp01(u.hitFlashT / 0.18f);
            DrawSpriteCenterFlip(u.tex, { u.pos.x, u.pos.y - yoff },
                baseW * scaleX * (1.0f + 0.57f * k),
                baseH * scaleY * (1.0f + 0.57f * k),
                u.facingRight, 0.7f * k * a, 1.0f, 0.6f, 0.6f);
        }
        if (u.healFlashT > 0.0f) {
            float k = clamp01(u.healFlashT / 0.25f);
            DrawSpriteCenterFlip(u.tex, { u.pos.x, u.pos.y - yoff },
                baseW * scaleX * (1.0f + 0.76f * k),
                baseH * scaleY * (1.0f + 0.76f * k),
                u.facingRight, 1.f * k * a, 0.6f, 1.0f, 0.6f);
        }

        // 击退拖影
        if (u.kbTimer > 0.0f && u.kbDur > 0.0f) {
            float p = Battle::clamp(u.kbTimer / u.kbDur, 0.0f, 1.0f);
            for (int gi = 1; gi <= KNOCKBACK_GHOSTS; ++gi) {
                float falloff = 1.0f - (float)gi / (float)(KNOCKBACK_GHOSTS + 1);
                float ga = 0.28f * p * falloff * a;
                BVec2 gp{
                    u.pos.x - u.kbDir.x * (KNOCKBACK_GHOST_STEP * VISUAL_SCALE) * gi * (0.6f + 0.4f * p),
                    u.pos.y - u.kbDir.y * (KNOCKBACK_GHOST_STEP * VISUAL_SCALE) * gi * (0.6f + 0.4f * p)
                };
                DrawSpriteCenterFlip(u.tex, gp, baseW * scaleX, baseH * scaleY, u.facingRight, ga, 1, 1, 1);
            }
        }

        // 职业光晕
        if (u.kind == BKind::Mage && u.atkCD > 0.0f) {
            float charge = 1.0f - clamp01(u.atkT / u.atkCD);
            float cs = 1.0f + HALO_MAGE_GROWTH * charge;
            const char* texMage = LoadIconCached_Battle(TEX_HALO_MAGE) ? TEX_HALO_MAGE : TEX_HALO_FALLBACK;
            DrawSpriteCenter(texMage, { u.pos.x, haloY },
                baseW * cs * haloScaleBounce, baseH * cs * haloScaleBounce,
                0.35f * charge, 0.6f, 0.8f, 1.0f);
        }
        if (u.kind == BKind::Shield) {
            auto [ei, nd] = NearestEnemyVisible(u.pos);
            if (ei != -1 && nd < u.range * 1.4f) {
                float pulse = 0.3f + 0.2f * std::sin(gBattleTime * 10.0f + i);
                const char* texShield = LoadIconCached_Battle(TEX_HALO_SHIELD) ? TEX_HALO_SHIELD : TEX_HALO_FALLBACK;
                DrawSpriteCenter(texShield, { u.pos.x, haloY },
                    baseW * HALO_SHIELD_SCALE * haloScaleBounce, baseH * HALO_SHIELD_SCALE * haloScaleBounce,
                    0.25f + pulse, 1.0f, 1.0f, 1.0f);
            }
        }
        if (u.kind == BKind::EnemyBoss) {
            float pulse = 0.5f + 0.5f * std::sin(gBattleTime * 3.0f);
            const char* texBoss = LoadIconCached_Battle(TEX_HALO_BOSS) ? TEX_HALO_BOSS : TEX_HALO_FALLBACK;
            DrawSpriteCenter(texBoss, { u.pos.x, haloY },
                baseW * HALO_BOSS_SCALE * haloScaleBounce, baseH * HALO_BOSS_SCALE * haloScaleBounce,
                0.18f + 0.10f * pulse, 1.0f, 0.4f, 0.4f);
        }

        // 4级单位彩虹辉光叠层
        if (u.team == BTeam::Ally && u.level >= 4) {
            float t = gBattleTime * 2.2f;
            float rr = 0.5f + 0.5f * std::sin(t + 0.0f), gg = 0.5f + 0.5f * std::sin(t + 2.1f), bb = 0.5f + 0.5f * std::sin(t + 4.2f);
            DrawSpriteCenter("rainbow_glow.tga", { u.pos.x, u.pos.y - yoff }, baseW * 1.2f, baseH * 1.2f, 0.35f, rr, gg, bb);
        }

        // 本体
        DrawSpriteCenterFlip(u.tex, { u.pos.x, u.pos.y - yoff }, baseW * scaleX, baseH * scaleY, u.facingRight, a, r, g, b);

        // 血条 + 数字（原样）
        const float barY = u.pos.y - (baseH * 0.625f);
        const float barW = baseW * 0.875f;
        const float barBgH = 6.0f;
        const float barFgH = 4.0f;

        float hpRatio = (u.maxhp > 0 ? (u.hp / u.maxhp) : 0.0f);
        float hpw = barW * hpRatio;

        if (hpRatio < 0.2f) {
            float pulse = 0.5f + 0.5f * std::sin(gBattleTime * 8.0f + i);
            DrawSolidQuad({ u.pos.x, barY }, barW + 8.0f, barBgH + 6.0f, 0.6f, 0.1f, 0.1f, 0.25f * pulse);
        }

        DrawSolidQuad({ u.pos.x, barY }, barW + 2.0f, barBgH, 0, 0, 0, 0.6f);
        DrawSolidQuad({ u.pos.x - (barW - hpw) * 0.5f, barY }, hpw, barFgH,
            (u.team == BTeam::Ally ? 0.2f : 0.7f),
            (u.team == BTeam::Ally ? 0.9f : 0.1f),
            0.1f, 0.9f);

        float hNum = (BS.W > 0 ? BS.W : 1600.0f) * 0.018f;
        std::string s = std::to_string((int)std::round(u.hp)) + "/" + std::to_string((int)std::round(u.maxhp));
        DrawNumberString_World(s, u.pos.x, barY - 14.0f, hNum, 0.95f, 1, 1, 1);
    }

    // 子弹（本体+尾迹）
    for (const auto& b : BS.bullets) {
        if (!b.alive) continue;
        // 尾迹
        int idx = 0;
        for (auto it = b.trail.begin(); it != b.trail.end(); ++it, ++idx) {
            float alpha = 0.2f * (1.0f - (float)idx / 6.0f);
            float sz = (b.big ? 36.0f : 24.0f) * VISUAL_SCALE * (1.0f - 0.05f * idx);
            DrawSpriteCenter(LoadIconCached_Battle("trail.tga") ? "trail.tga" : b.tex, *it, sz, sz, alpha);
            if (idx >= 5) break;
        }
        float sz = b.big ? (40.0f * VISUAL_SCALE) : (28.0f * VISUAL_SCALE);
        DrawSpriteCenter(b.tex, b.pos, sz, sz, 1.0f);
    }

    // 近战特效（原）
    for (const auto& e : BS.effects) {
        if (!e.alive) continue;
        DrawSpriteCenter(e.tex, e.pos, 42.0f * VISUAL_SCALE, 42.0f * VISUAL_SCALE, 0.85f);
    }

    // 新VFX
    for (const auto& v : BS.vfx) {
        if (!v.alive) continue;
        float p = clamp01(v.t / v.dur);
        float scale = 1.0f + v.grow * p;
        float alpha = v.a * (1.0f - p);
        DrawSpriteCenterRot(v.tex, v.pos, v.w * scale, v.h * scale, v.rot, alpha, v.r, v.g, v.b);
    }

    // 光束
    for (const auto& be : BS.beams) if (be.alive) DrawBeam(be);

    // 飘字
    for (const auto& f : gFloatNums) {
        if (!f.alive) continue;
        float t = Battle::clamp(f.t / f.dur, 0.0f, 1.0f);
        float alpha = (t < 0.4f) ? (t / 0.4f) : (1.0f - (t - 0.4f) / 0.6f);
        alpha = Battle::clamp(alpha, 0.0f, 1.0f);
        float hNum = (BS.W > 0 ? BS.W : 1600.0f) * 0.020f;
        DrawNumberString_World(f.text, f.pos.x, f.pos.y, hNum, alpha, f.r, f.g, f.b);
    }

    // 死亡FX
    static unsigned int s_die = 0;
    if (!s_die) s_die = LoadIconCached_Battle("siwang.tga");
    for (const auto& dfx : gDeaths) {
        if (!dfx.alive || !s_die) continue;
        float t = Battle::clamp(dfx.t / dfx.dur, 0.0f, 1.0f);
        float alpha = (t < 0.5f) ? (t / 0.5f) : (1.0f - (t - 0.5f) / 0.5f);
        float scale = (t < 0.5f) ? (0.6f + 0.4f * (t / 0.5f))
            : (1.0f + 0.4f * ((t - 0.5f) / 0.5f));
        float sz = 56.0f * VISUAL_SCALE * scale;
        DrawSpriteCenter("siwang.tga", dfx.pos, sz, sz, alpha, 1, 1, 1);
    }

    // BOSS横幅
    if (gBossBanner != BossBanner::None) {
        float a = 0.0f;
        if (gBossBanner == BossBanner::FadingIn)       a = clamp01(gBossBannerT / gBossInDur);
        else if (gBossBanner == BossBanner::Hold)      a = 1.0f;
        else if (gBossBanner == BossBanner::FadingOut) a = clamp01(1.0f - gBossBannerT / gBossOutDur);
        DrawSolidQuad({ 0, 0 }, BS.W, BS.H, 0, 0, 0, 0.25f * a);
        unsigned int bossTxt = LoadIconCached_Battle("boss_text.tga");
        if (bossTxt == 0) bossTxt = LoadIconCached_Battle("warning.tga");
        if (bossTxt != 0) {
            Object_2D o(0, 0, BS.W * 0.8f, BS.H * 0.25f);
            o.Transform.Position = { 0, -BS.H * 0.18f };
            o.texNo = bossTxt;
            o.Color = { 1, 1, 1, a };
            BoxVertexsMake(&o);
            DrawObject_2D(&o);
        }
    }

    // 失败过场黑幕 + YOUDIE（原）
    {
        float k = 0.0f;
        if (gFailPhase == FailPhase::FadingOut)      k = Battle::clamp(gFailT / gFailDur, 0.0f, 1.0f);
        else if (gFailPhase == FailPhase::FadingIn)  k = Battle::clamp(1.0f - gFailT / gFailDur, 0.0f, 1.0f);
        else if (!BS.running && gWinPhase == WinPhase::None) k = 1.0f;

        if (k > 0.001f) {
            CamVec2 save = gRenderCam;
            gRenderCam = { 0,0 };
            DrawSolidQuad({ 0, 0 }, BS.W, BS.H, 0, 0, 0, k);

            static unsigned int s_youdieTex = 0;
            if (s_youdieTex == 0) {
                s_youdieTex = LoadIconCached_Battle("youdie.tga");
                if (s_youdieTex == 0) s_youdieTex = LoadIconCached_Battle("youdie.tga");
            }
            bool showDie = (gFailPhase != FailPhase::None) || (!BS.running && gWinPhase == WinPhase::None);
            if (s_youdieTex && showDie) {
                float base = std::min(BS.W, BS.H) * gDieScale;
                float w = gDieUseSquare ? base : (BS.W * gDieScale);
                float h = gDieUseSquare ? base : (BS.H * gDieScale);

                Object_2D o(0, 0, w, h);
                o.Transform.Position = { 0, 0 };
                o.texNo = s_youdieTex;
                o.Color = { 1, 1, 1, k };
                BoxVertexsMake(&o);
                DrawObject_2D(&o);
            }
            gRenderCam = save;
        }
    }

    // 胜利过场（原）
    if (gWinPhase != WinPhase::None) {
        float alpha = 0.0f;
        if (gWinPhase == WinPhase::FadingIn)      alpha = Battle::clamp(gWinT / gWinIn, 0.0f, 1.0f);
        else if (gWinPhase == WinPhase::Hold)     alpha = 1.0f;
        else if (gWinPhase == WinPhase::FadingOut)alpha = Battle::clamp(1.0f - gWinT / gWinOut, 0.0f, 1.0f);

        static unsigned int s_winTex = 0;
        if (s_winTex == 0) {
            s_winTex = LoadIconCached_Battle("win.tga");
            if (s_winTex == 0) s_winTex = LoadIconCached_Battle("win.tga");
        }
        if (s_winTex && alpha > 0.001f) {
            CamVec2 save = gRenderCam;
            gRenderCam = { 0,0 };

            float base = std::min(BS.W, BS.H) * gWinScale;
            float w = gWinUseSquare ? base : (BS.W * gWinScale);
            float h = gWinUseSquare ? base : (BS.H * gWinScale);

            Object_2D o(0, 0, w, h);
            o.Transform.Position = { 0, 0 };
            o.texNo = s_winTex;
            o.Color = { 1, 1, 1, alpha };
            BoxVertexsMake(&o);
            DrawObject_2D(&o);

            gRenderCam = save;
        }
    }

    // 【★】屏幕白闪叠加层
    if (gFlashT > 0.0f && gFlashDur > 0.0f) {
        float p = clamp01(gFlashT / gFlashDur);
        float a = gFlashA * p;
        DrawSolidQuad({ 0,0 }, BS.W, BS.H, 1, 1, 1, a);
    }
}

void FinalizeGameBattle(void) { BS = BState{}; }

float Battle_GetFailFadeAlpha() {
    if (gFailPhase == FailPhase::FadingOut) return Battle::clamp(gFailT / gFailDur, 0.0f, 1.0f);
    if (gFailPhase == FailPhase::FadingIn)  return Battle::clamp(1.0f - gFailT / gFailDur, 0.0f, 1.0f);
    return 0.0f;
}

// 给HUD用
int Battle_GetWaveForHUD_Ready() { return (gNextWave <= 0 ? 1 : gNextWave); }
int Battle_GetWaveForHUD_Battle() { return (std::max)(1, BS.wave - 1); }

// [INVIS] 对外接口：调整/读取隐形者透明度
void Battle_SetInvisibleAlpha(float a) { gInvisibleAlpha = Battle::clamp(a, 0.0f, 1.0f); }
float Battle_GetInvisibleAlpha() { return gInvisibleAlpha; }
