
#pragma once
#include <queue>
#include <optional>

// マウスクラス：マウス入力を管理する
class Mouse
{
	friend class Window;
public:
	// 生のマウス移動量
	struct RawDelta
	{
		int x, y;
	};

	// マウスイベント
	class Event
	{
	public:
		// イベントの種類
		enum class Type
		{
			LPress,     // 左クリック押下
			LRelease,   // 左クリック解放
			RPress,     // 右クリック押下
			RRelease,   // 右クリック解放
			MPress,     // 中ボタン押下
			MRelease,   // 中ボタン解放
			WheelUp,    // ホイール上回転
			WheelDown,  // ホイール下回転
			Move,       // 移動
			Enter,      // ウィンドウに入った
			Leave,      // ウィンドウから出た
		};
	private:
		Type type;                  // イベントの種類
		bool leftIsPressed;         // 左ボタンが押されているか
		bool rightIsPressed;        // 右ボタンが押されているか
		int x;                      // X座標
		int y;                      // Y座標
	public:
		// イベント生成
		Event(Type type, const Mouse& parent) noexcept
			:
			type(type),
			leftIsPressed(parent.leftIsPressed),
			rightIsPressed(parent.rightIsPressed),
			x(parent.x),
			y(parent.y)
		{
		}

		Type GetType() const noexcept { return type; }
		std::pair<int, int> GetPos() const noexcept { return { x, y }; }
		int GetPosX() const noexcept { return x; }
		int GetPosY() const noexcept { return y; }
		bool LeftIsPressed() const noexcept { return leftIsPressed; }
		bool RightIsPressed() const noexcept { return rightIsPressed; }
	};

public:
	Mouse() = default;
	Mouse(const Mouse&) = delete;
	Mouse& operator=(const Mouse&) = delete;

	// マウスの現在位置取得
	std::pair<int, int> GetPos() const noexcept;
	std::optional<RawDelta> ReadRawDelta() noexcept;
	int GetPosX() const noexcept;
	int GetPosY() const noexcept;

	// マウスがウィンドウ内にいるかどうか
	bool IsInWindow() const noexcept;

	// ボタン状態取得
	bool LeftIsPressed() const noexcept;
	bool RightIsPressed() const noexcept;
	bool MiddleIsPressed() const noexcept;

	// イベントキューからイベント取得
	std::optional<Mouse::Event> Read() noexcept;

	// イベントキューが空かどうか
	bool IsEmpty() const noexcept { return buffer.empty(); }

	// イベントバッファクリア
	void Flush() noexcept;

	// 生の入力有効化／無効化
	void EnableRaw() noexcept;
	void DisableRaw() noexcept;
	bool RawEnabled() const noexcept;

	// イベント受信（ウィンドウプロシージャ経由）
	void OnMouseMove(int x, int y) noexcept;
	void OnMouseLeave() noexcept;
	void OnMouseEnter() noexcept;
	void OnRawDelta(int dx, int dy) noexcept;
	void OnLeftPressed(int x, int y) noexcept;
	void OnLeftReleased(int x, int y) noexcept;
	void OnRightPressed(int x, int y) noexcept;
	void OnRightReleased(int x, int y) noexcept;
	void OnMiddlePressed(int x, int y) noexcept;
	void OnMiddleReleased(int x, int y) noexcept;
	void OnWheelUp(int x, int y) noexcept;
	void OnWheelDown(int x, int y) noexcept;
	void TrimBuffer() noexcept;
	void TrimRawInputBuffer() noexcept;
	void OnWheelDelta(int x, int y, int delta) noexcept;
	int GetWheelDeltaCarry(void) const noexcept;
	void SetWheelDeltaCarry(int i) noexcept;

	// 原点中心座標取得
	std::pair<float, float> GetCorrectedPos(int windowWidth, int windowHeight) const noexcept;

private:
	static constexpr unsigned int bufferSize = 16u;

	// マウス内部状態
	int x;                  // X座標
	int y;                  // Y座標
	bool leftIsPressed = false;    // 左ボタン押下状態
	bool rightIsPressed = false;   // 右ボタン押下状態
	bool middleIsPressed = false;  // 中ボタン押下状態
	bool isInWindow = false;       // ウィンドウ内フラグ
	int wheelDeltaCarry = 0;       // ホイール回転量
	bool rawEnabled = false;       // 生入力有効化フラグ

	// イベントキュー
	std::queue<Event> buffer;          // 通常イベントバッファ
	std::queue<RawDelta> rawDeltaBuffer;   // 生移動量バッファ
};
