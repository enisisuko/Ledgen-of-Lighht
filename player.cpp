// player.cpp  —— 带“蹦蹦跳跳 + 挤压回弹 + 转向冷却/翻转挤压”的主角（含空指针保护/懒加载）
#include "main.h"
#include "texture.h"
#include "sprite.h"
#include "controller.h"
#include "player.h"

#include <cmath>
#include <algorithm>

#define PLAYER_MOVE_AXCEL (2.5f)

static inline float P_dt() { return 1.0f / 60.0f; }
static inline float WrapTau(float x) { x = std::fmod(x, 6.2831853f); if (x < 0.0f) x += 6.2831853f; return x; }

// —— 基础尺寸 —— //
static constexpr float BASE_W = 150.0f;
static constexpr float BASE_H = 150.0f;

// —— 转向冷却（与战场单位一致 0.5s）—— //
static constexpr float FACE_TURN_CD = 0.5f;
// —— 翻转挤压动画时长 —— //
static constexpr float FACE_FLIP_SQUASH_DUR = 0.18f;
// —— 翻转挤压强度（横收/纵拉）—— //
static constexpr float FACE_FLIP_SQUASH_X = 0.28f;
static constexpr float FACE_FLIP_SQUASH_Y = 0.18f;

unsigned int        PlayerTextureId = 0;
static Object_2D* player = nullptr;
static Object_2D::Vec2 vel{ 0.0f, 0.0f };

// —— 跳动动画状态（相位/振幅解耦）—— //
static float s_phase = 0.0f;  // 0..2π
static float s_ampPx = 0.0f;  // 垂直抖动振幅（像素）

// —— 转向状态 —— //
static bool  s_facingRight = true;   // 当前是否朝右
static float s_faceLockT = 0.0f;   // 转向冷却计时
static float s_faceFlipT = 0.0f;   // 翻转挤压剩余时间

bool Player_IsInitialized(void) { return player != nullptr; }

void Player_EnsureAllocated(void) {
    if (!player) {
        InitializePlayer();
    }
}

void InitializePlayer(void)
{
    if (player) return; // 防重入
    player = new Object_2D(0.0f, 0.0f, BASE_W, BASE_H);
    vel = { 0.0f, 0.0f };

    PlayerTextureId = LoadTexture("rom:/iconmonstr-circle-lined-240.tga");
    player->texNo = PlayerTextureId;
    player->Color.A = 0.9f;

    s_phase = 0.0f;
    s_ampPx = 0.0f;

    s_facingRight = true;
    s_faceLockT = 0.0f;
    s_faceFlipT = 0.0f;
}

void UpdatePlayer(void)
{
    if (!player) return; // ★ 空指针保护

    // 基础移动（手柄 + 键盘）
    if (manager.Gamepad_Parameters.DPad_Left || kbd.KeyIsPressed('A')) vel.x -= PLAYER_MOVE_AXCEL;
    if (manager.Gamepad_Parameters.DPad_Right || kbd.KeyIsPressed('D')) vel.x += PLAYER_MOVE_AXCEL;
    if (manager.Gamepad_Parameters.DPad_Up || kbd.KeyIsPressed('W')) vel.y -= PLAYER_MOVE_AXCEL; // ↑ 向上为负y
    if (manager.Gamepad_Parameters.DPad_Down || kbd.KeyIsPressed('S')) vel.y += PLAYER_MOVE_AXCEL; // ↓ 向下为正y

    // 阻尼
    vel.x -= vel.x * 0.2f;
    vel.y -= vel.y * 0.2f;

    // 应用位移
    player->Transform.Position.x += vel.x;
    player->Transform.Position.y += vel.y;

    // ===== 蹦蹦跳跳：相位与振幅解耦 =====
    const float dt = P_dt();

    const float sp = std::sqrt(vel.x * vel.x + vel.y * vel.y);
    const bool  moving = (sp > 0.15f);
    const float sp01 = std::fmin(1.0f, sp / 18.0f); // 0..1

    if (moving) {
        const float omega = 7.0f + 4.0f * sp01; // rad/s
        s_phase = WrapTau(s_phase + omega * dt);

        const float targetAmp = 6.0f + 7.0f * sp01;
        const float kRise = 10.0f;
        s_ampPx += (targetAmp - s_ampPx) * (1.0f - std::exp(-kRise * dt));
    }
    else {
        const float kFall = 28.0f;
        s_ampPx += (0.0f - s_ampPx) * (1.0f - std::exp(-kFall * dt));
    }

    // ===== 转向（带冷却 + 启动翻转挤压）=====
    if (s_faceLockT > 0.0f) s_faceLockT -= dt;
    if (s_faceFlipT > 0.0f) s_faceFlipT -= dt;

    // 根据水平速度判断“想要的朝向”
    if (std::fabs(vel.x) > 0.2f) {
        bool wantRight = (vel.x >= 0.0f);
        if (s_faceLockT <= 0.0f && wantRight != s_facingRight) {
            s_facingRight = wantRight;         // 真正翻转
            s_faceLockT = FACE_TURN_CD;      // 冷却
            s_faceFlipT = FACE_FLIP_SQUASH_DUR; // 开始“纸片挤压”
        }
    }
}

void DrawPlayer(void)
{
    if (!player) return; // ★ 空指针保护

    const float bob = std::sinf(s_phase) * s_ampPx;

    // —— 移动弹跳的基础挤压（上下弹性）——
    const float ampNorm = (s_ampPx > 1e-3f) ? (bob / s_ampPx) : 0.0f; // -1..1
    const float squashFromBob = 0.14f * (-ampNorm);
    float sy = 1.0f - squashFromBob;
    float sx = 1.0f + squashFromBob * 0.6f;

    // —— 翻转瞬间的“纸片挤压”（叠乘到 sx/sy）——
    if (s_faceFlipT > 0.0f) {
        float t = 1.0f - std::clamp(s_faceFlipT / FACE_FLIP_SQUASH_DUR, 0.0f, 1.0f); // 0→1
        float k = std::sin(t * 3.1415926f); // 0→1→0，柔和
        sx *= (1.0f - FACE_FLIP_SQUASH_X * k); // 横向收
        sy *= (1.0f + FACE_FLIP_SQUASH_Y * k); // 纵向拉
    }

    Object_2D o(0.0f, 0.0f, BASE_W * sx, BASE_H * sy);
    o.Transform.Position = player->Transform.Position;
    o.Transform.Position.y += bob;
    o.Color = player->Color;
    o.texNo = player->texNo;

    // —— 根据朝向翻转 UV（默认朝右；朝左时水平镜像）——
    if (s_facingRight) {
        o.UV[0] = { 0,0 }; o.UV[1] = { 1,0 }; o.UV[2] = { 0,1 }; o.UV[3] = { 1,1 };
    }
    else {
        o.UV[0] = { 1,0 }; o.UV[1] = { 0,0 }; o.UV[2] = { 1,1 }; o.UV[3] = { 0,1 };
    }

    BoxVertexsMake(&o);
    DrawObject_2D(&o);

    // （可选）在翻转瞬间做一层淡淡亮度：如果你喜欢可取消注释
    
    if (s_faceFlipT > 0.0f) {
        float t = 1.0f - std::clamp(s_faceFlipT / FACE_FLIP_SQUASH_DUR, 0.0f, 1.0f);
        float a = 0.25f * std::sin(t * 3.1415926f);
        Object_2D glow(0.0f, 0.0f, BASE_W * sx, BASE_H * sy);
        glow.Transform.Position = o.Transform.Position;
        glow.Color = { 1, 1, 1, a * player->Color.A };
        glow.texNo = o.texNo;
        glow.UV[0] = o.UV[0]; glow.UV[1] = o.UV[1]; glow.UV[2] = o.UV[2]; glow.UV[3] = o.UV[3];
        BoxVertexsMake(&glow);
        DrawObject_2D(&glow);
    }
    
}

void FinalizePlayer(void)
{
    if (PlayerTextureId) UnloadTexture(PlayerTextureId);
    PlayerTextureId = 0;
    delete player;
    player = nullptr;
}

Object_2D* GetPlayer(void)
{
    return player;
}
