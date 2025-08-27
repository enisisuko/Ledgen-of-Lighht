
#pragma once
#include <queue>
#include <optional>

// �}�E�X�N���X�F�}�E�X���͂��Ǘ�����
class Mouse
{
	friend class Window;
public:
	// ���̃}�E�X�ړ���
	struct RawDelta
	{
		int x, y;
	};

	// �}�E�X�C�x���g
	class Event
	{
	public:
		// �C�x���g�̎��
		enum class Type
		{
			LPress,     // ���N���b�N����
			LRelease,   // ���N���b�N���
			RPress,     // �E�N���b�N����
			RRelease,   // �E�N���b�N���
			MPress,     // ���{�^������
			MRelease,   // ���{�^�����
			WheelUp,    // �z�C�[�����]
			WheelDown,  // �z�C�[������]
			Move,       // �ړ�
			Enter,      // �E�B���h�E�ɓ�����
			Leave,      // �E�B���h�E����o��
		};
	private:
		Type type;                  // �C�x���g�̎��
		bool leftIsPressed;         // ���{�^����������Ă��邩
		bool rightIsPressed;        // �E�{�^����������Ă��邩
		int x;                      // X���W
		int y;                      // Y���W
	public:
		// �C�x���g����
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

	// �}�E�X�̌��݈ʒu�擾
	std::pair<int, int> GetPos() const noexcept;
	std::optional<RawDelta> ReadRawDelta() noexcept;
	int GetPosX() const noexcept;
	int GetPosY() const noexcept;

	// �}�E�X���E�B���h�E���ɂ��邩�ǂ���
	bool IsInWindow() const noexcept;

	// �{�^����Ԏ擾
	bool LeftIsPressed() const noexcept;
	bool RightIsPressed() const noexcept;
	bool MiddleIsPressed() const noexcept;

	// �C�x���g�L���[����C�x���g�擾
	std::optional<Mouse::Event> Read() noexcept;

	// �C�x���g�L���[���󂩂ǂ���
	bool IsEmpty() const noexcept { return buffer.empty(); }

	// �C�x���g�o�b�t�@�N���A
	void Flush() noexcept;

	// ���̓��͗L�����^������
	void EnableRaw() noexcept;
	void DisableRaw() noexcept;
	bool RawEnabled() const noexcept;

	// �C�x���g��M�i�E�B���h�E�v���V�[�W���o�R�j
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

	// ���_���S���W�擾
	std::pair<float, float> GetCorrectedPos(int windowWidth, int windowHeight) const noexcept;

private:
	static constexpr unsigned int bufferSize = 16u;

	// �}�E�X�������
	int x;                  // X���W
	int y;                  // Y���W
	bool leftIsPressed = false;    // ���{�^���������
	bool rightIsPressed = false;   // �E�{�^���������
	bool middleIsPressed = false;  // ���{�^���������
	bool isInWindow = false;       // �E�B���h�E���t���O
	int wheelDeltaCarry = 0;       // �z�C�[����]��
	bool rawEnabled = false;       // �����͗L�����t���O

	// �C�x���g�L���[
	std::queue<Event> buffer;          // �ʏ�C�x���g�o�b�t�@
	std::queue<RawDelta> rawDeltaBuffer;   // ���ړ��ʃo�b�t�@
};
