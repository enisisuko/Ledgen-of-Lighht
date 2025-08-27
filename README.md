# === 中文版 README ===
cat > README.md << 'MD'
<!-- lang switch -->
<p align="right">🌐 Language: <b>简体中文</b> · <a href="./README.ja.md">日本語</a></p>

<!-- hero -->
<h1 align="center">光灵传说 · Lumen Saga</h1>
<p align="center"><em>当文明褪去，仍需有人将光点燃。</em></p>
<p align="center">
  <a href="../../releases"><img alt="Releases" src="https://img.shields.io/badge/download-releases-3b82f6"></a>
  <a href="../../issues"><img alt="Issues" src="https://img.shields.io/github/issues/enisisuko/-"></a>
  <a href="../../commits"><img alt="Last Commit" src="https://img.shields.io/github/last-commit/enisisuko/-"></a>
  <a href="../../stargazers"><img alt="Stars" src="https://img.shields.io/github/stars/enisisuko/-?style=social"></a>
</p>

<p align="center">
  <a href="#-概览">概览</a> ·
  <a href="#-核心玩法与美术表现">玩法</a> ·
  <a href="#-世界观与叙事底色">世界观</a> ·
  <a href="#-操作--输入映射">操作</a> ·
  <a href="#-系统设计蓝图">系统</a> ·
  <a href="#-快速开始">开始</a> ·
  <a href="#-Roadmap">Roadmap</a> ·
  <a href="#-贡献">贡献</a>
</p>

---

## ✨ 概览
**一句话**：一款“建造 × 招募 × 职业进阶”的波次防卫游戏。你在焦土上修筑圣坛、招募光灵、规划经济与职业曲线，守护圣女与最后的净土。

- **平台**：Windows（DirectX 渲染）；计划适配主机后端（OpenGL 4.5 渲染路径已抽象）
- **状态**：持续开发 / 不断打磨
- **游玩**：前往 **[Releases](../../releases)** 下载最新构建
- **问题与建议**：在 **[Issues](../../issues)** 留言，或参与 **[Projects](../../projects)** 讨论

---

## 🎮 核心玩法与美术表现
- **四类建筑·四条增长曲线**
  - **金币建筑**：波次结算金币，拉动“宏观经济”。
  - **光灵建筑**：提供人口与战斗基数。
  - **职业建筑**：转化为 _星空魔法师 / 森林战士_；升级升阶，**满级诞生唯一英雄单位**。
  - **圣女祭坛**：全队增幅中枢；满级化作**至高圣女**，法阵范围暴涨并持续伤害敌方。
- **编队与压强**：小兵 / 弓手 / 冲锋手 / 重盾 /唤魔者 /隐身杀手 /BOSS的**节拍推进**，迫使你在“经济—军备—祭坛”间做取舍。
- **轻量而丰富的表现**：HUD 等比绘制、拖影/抖屏/飘字、击退震击、能量涟漪与光晕，细腻而不过载。

> 设计哲学：**数值是秩序，节奏是叙事**——一次漂亮的波次调度，本身就是篇章。

---

## 🜂 世界观与叙事底色
黑暗涨潮后，文明在边界逐块退却。**金币**维系秩序，**光灵**是燃料与火种，**职业**是知识的形态，而**祭坛**放大信念。  
当你调度建筑与人口的节奏，法阵边界便随之呼吸——**希望被你亲手编排**。

---

## ⌨️ 操作 · 输入映射
- **移动**：`W A S D`  
- **选择切换**：`↑ / ↓`（或手柄十字键上下）  
- **确认 / 交互**：`Space`（或手柄右扳机）  

> 计划中的完整手柄映射：左摇杆 ↔ WASD，十字键 ↔ 方向键，右扳机 ↔ Space。

---

## 🧩 系统设计蓝图
```
┌────────── Economy ───────────┐     ┌────── Combat ───────┐
│  Gold Bldg  →  Coins     ─┐  │     │  Units & AI         │
│  Lumen Bldg →  Population ─┬─┼──►  │  Waves / Formations │
│  Job Bldg   →  Classes   ─┘  │     │  Boss Phases        │
│  Altar      →  Global Buffs  │     └────────┬────────────┘
└─────────────┬────────────────┘              │
              ▼                               ▼
         Hero Emergence                  Visual Feedback
      (Unique, high impact)            (Shake/Trail/Glow)

```
进阶与稀缺：职业升阶与祭坛加成存在瓶颈与唯一性，鼓励差异化解。

波次节拍：建设→战斗→复盘→再建设；短期容错与长期曲线并存。

资源流：金币、人口、职业位与祭坛加成交错，形成可被“雕刻”的策略空间。

## 🎮 快速开始
需要：Visual Studio 2022（含“使用 C++ 的桌面开发”组件）、Windows 10/11 SDK

```
git clone https://github.com/enisisuko/-.git
cd -
# 打开解决方案用 VS 构建（建议 Release）
# 或使用命令行： msbuild Game.sln /p:Configuration=Release
资源放置建议：

大体积纹理/音频请使用 Git LFS（如 .tga）；

不要将任何受限/商业 SDK 文件纳入版本控制（例如专有主机 SDK）1.
```
## 🜂 Roadmap

 英雄级单位专属天赋与事件

 手柄全映射 + 震动反馈

 OpenGL 4.5 渲染路径完善（为主机移植铺路）

 关卡 Modifier 与更细难度曲线

 自动化构建与发布流程（CI/CD）

跟踪开发：看 Projects 与 Issues。

## 🤝 贡献
欢迎提交 PR / Issue。发起 PR 前请先阅读 贡献约定（代码风格、提交信息语义化、分支模型等）——见 Wiki。

## 📝 许可
本仓库以个人项目形式公开，未经书面许可禁止商业用途。最终许可证与商业条款将于 Releases 与 LICENSE 公告。


<p align="center"> <sub>如果你愿意，也请点一颗小星星 ⭐。你的注视，就是这片净土扩张的半径。</sub> </p>



---

出于合规与安全考虑，任何受限/商业 SDK（如某些主机平台 SDK）不得随仓库分发。
MD ↩ ↩2
