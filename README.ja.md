
# === 日文版 README ===
cat > README.ja.md << 'MD'






<!-- lang switch --> 

<p align="right">🌐 言語: <a href="./README.md">简体中文</a> · <b>日本語</b></p> <!-- hero --> <h1 align="center">光霊伝説 · Ledgen of Light</h1> <p align="center"><em>文明が退いたとしても、光を灯す者がいる。</em></p> <p align="center"> <a href="../../releases"><img alt="Releases" src="https://img.shields.io/badge/download-releases-3b82f6"></a> <a href="../../issues"><img alt="Issues" src="https://img.shields.io/github/issues/enisisuko/-"></a> <a href="../../commits"><img alt="Last Commit" src="https://img.shields.io/github/last-commit/enisisuko/-"></a> <a href="../../stargazers"><img alt="Stars" src="https://img.shields.io/github/stars/enisisuko/-?style=social"></a> </p> <p align="center"> <a href="#-概要">概要</a> · <a href="#-コアゲームプレイと表現">ゲーム性</a> · <a href="#-世界観とナラティブ">世界観</a> · <a href="#-操作--入力マッピング">操作</a> · <a href="#-システム設計">システム</a> · <a href="#-クイックスタート">開始</a> · <a href="#-roadmap">Roadmap</a> · <a href="#-コントリビュート">貢献</a> </p>

---

## ✨ 概要
一言で：建設 × 募兵 × 職業進化による「ウェーブ防衛」ゲーム。焦土で祭壇を築き、光霊を招き、経済と職業曲線を編成して、聖女と最後の浄土を護ります。

- **プラットフォーム**Windows（DirectX レンダリング）。将来的にコンソール後段へ（OpenGL 4.5 パスは抽象化済み）
- **ステータス**継続開発中
- **プレイ：** **[Releases](../../releases)** から最新ビルドを入手
- **要望/不具合：** **[Issues](../../Issues)** または Projects へ


---


## 🎮 コアゲームプレイと表現
4 種の建物・4 つの成長曲線

ゴールド建物：ウェーブ終了時にコインを精算し、経済を駆動。

  - **光霊建物** 人口と戦闘基盤を供給。

  - **職業建物** 星空魔導師 / 森林戦士 に転化。アップグレードで階位上昇、最終段で唯一の英雄ユニットが誕生。

  - **聖女祭壇** 全体バフの中枢。最終段では至高の聖女となり、陣域が大きく拡張し敵へ持続ダメージ。

編成と圧力：歩兵／弓兵／突撃／突撃／重盾／召喚士／BOSSの拍動的前進が、経済・軍備・祭壇の取捨選択を迫る。

軽量かつ豊かな表現：HUD 等比描画、残像／カメラシェイク／フローティングテキスト、ノックバック、光輪・波紋などを過不足なく配置。

>  デザイン哲学：数値は秩序、リズムは物語。美しいウェーブ運用そのものが章となる。

---



🜂 世界観とナラティブ
暗黒が満ち、文明は縁から後退した。コインは秩序を保ち、光霊は燃料であり火種。職業は知のかたち、そして祭壇は信の増幅器。
建設と人口のリズムを編むとき、陣域は呼吸し、希望はあなたの手で編曲される。

## ⌨️ 操作 · 入力マッピング


- **移動**  W A S D
- **選択切替**  ↑ / ↓（または十字キー上下）
- **決定 / インタラクト**  Space（または右トリガー）

> 予定：左スティック ↔ WASD、十字キー ↔ 方向キー、右トリガー ↔ Space。


---



## 🧩 システム設計
```
┌────────── Economy ──────────┐                     ┌────── Combat ────────┐
│ Gold → Coin Flow    ─┐      │                     │ Units / AI           │
│ Lumen→ Population      ──────────────────────────►│ Waves / Formations   │
│ Jobs → Classes      ─┘      │          │          │ Boss Phases          │
│ Altar→ Global Buffs         │          │          └────────┬─────────────┘
└───────────┬─────────────────┘          │                   │
            ▼                            ▼                   ▼
      Hero Emergence              Economy Tuning      Visual Feedback
```
希少性とボトルネック：職業進化と祭壇バフには唯一性と臨界があり、分岐解を促す。

ウェーブの律動：建設→戦闘→内省→再建設。短期の猶予と長期曲線が併存。

> 資源流：コイン／人口／職枠／祭壇バフが交差し、彫刻可能な戦略空間を形成。


---


## 🎮 クイックスタート
必要：Visual Studio 2022（C++デスクトップ開発）、Windows 10/11 SDK

```
git clone https://github.com/enisisuko/-.git
cd -
# ソリューションを開いてビルド（Release 推奨）
# もしくは： msbuild Game.sln /p:Configuration=Release
アセット指針：

大容量のテクスチャ/オーディオには Git LFS を推奨（.tga 等）

制限付き/商用 SDK はバージョン管理に含めないでください1.
```

---



## 🜂  Roadmap

 - **英雄ユニット専用の特性とイベント**
 - **パッド完全対応 + 振動**
- **OpenGL 4.5 レンダリングパス拡充（コンソール移植に備える）**
- **ウェーブ Modifiers と細かな難易度曲線**
- **CI/CD による自動ビルドと配布**


---


## 🤝 コントリビュート
PR / Issue を歓迎します。PR 前に 貢献ガイド（コード規約、コミットメッセージ、ブランチ運用）をご一読ください——Wiki を参照。

## 📝 ライセンス
本リポジトリは個人プロジェクトとして公開中であり、商用利用は禁止です。最終的なライセンスは Releases および LICENSE にて告知します。



<p align="center"> <sub>もし気に入っていただけたら ⭐ を。あなたの視線が、浄土の半径を広げる。</sub> </p>

