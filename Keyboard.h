
#pragma once
#include <queue>
#include <bitset>
#include <optional>

// 键盘类，用于处理键盘输入事件
class Keyboard
{
    // 声明 Window 类为友元类，可以访问 Keyboard 的私有成员
    friend class Window;
public:
    // 定义事件类，用于存储键盘事件
    class Event
    {
    public:
        // 键盘事件的类型：按下、释放和无效
        enum class Type
        {
            Press,    // 按下事件
            Release,  // 释放事件
            Invalid   // 无效事件（表示没有发生按键操作）
        };
    private:
        // 事件类型和按键码
        Type type;          // 事件类型
        unsigned char code; // 按键码
    public:
        // 构造函数，初始化事件类型和按键码
        Event(Type type, unsigned char code) noexcept
            :
            type(type),
            code(code)
        {}
        // 判断事件是否为按下事件
        bool IsPress() const noexcept
        {
            return type == Type::Press;
        }
        // 判断事件是否为释放事件
        bool IsRelease() const noexcept
        {
            return type == Type::Release;
        }
        // 获取事件的按键码
        unsigned char GetCode() const noexcept
        {
            return code;
        }
    };
public:

    // 默认构造函数
    Keyboard() = default;
    // 禁用拷贝构造函数和赋值运算符，防止对象拷贝
    Keyboard(const Keyboard&) = delete;
    Keyboard& operator=(const Keyboard&) = delete;

    // 按键事件相关功能
    // 判断某个按键是否被按下
    bool KeyIsPressed(unsigned char keycode) const noexcept;

    bool KeyJustPressed(unsigned char keycode) noexcept;

    // 读取一个按键事件
    std::optional<Event> ReadKey() noexcept;
    // 检查按键缓冲区是否为空
    bool KeyIsEmpty() const noexcept;
    // 清空按键事件缓冲区
    void FlushKey() noexcept;

    // 字符事件相关功能
    // 读取一个字符事件
    std::optional<char> ReadChar() noexcept;
    // 检查字符缓冲区是否为空
    bool CharIsEmpty() const noexcept;
    // 清空字符缓冲区
    void FlushChar() noexcept;
    // 清空按键和字符缓冲区
    void Flush() noexcept;

    // 自动重复按键控制
    // 启用自动重复
    void EnableAutorepeat() noexcept;
    // 禁用自动重复
    void DisableAutorepeat() noexcept;
    // 检查是否启用自动重复
    bool AutorepeatIsEnabled() const noexcept;
public:
    // 处理按键按下事件
    void OnKeyPressed(unsigned char keycode) noexcept;
    // 处理按键释放事件
    void OnKeyReleased(unsigned char keycode) noexcept;
    // 处理字符输入事件
    void OnChar(char character) noexcept;
    // 清除按键状态
    void ClearState() noexcept;

    // 修剪缓冲区以保持其大小不超过 bufferSize
    template<typename T>
    static void TrimBuffer(std::queue<T>& buffer) noexcept;
private:
    // 常量定义
    static constexpr unsigned int nKeys = 256u;      // 键盘上的按键数量
    static constexpr unsigned int bufferSize = 16u;  // 按键/字符缓冲区大小

    // 成员变量
    bool autorepeatEnabled = false;           // 是否启用按键自动重复
    std::bitset<nKeys> keystates;             // 存储按键状态（按下或释放）
    std::bitset<nKeys> old_keystates;             // 存储按键状态（按下或释放）
    std::queue<Event> keybuffer;              // 按键事件缓冲区
    std::queue<char> charbuffer;              // 字符事件缓冲区
};
