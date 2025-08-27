
#include "Keyboard.h"


// 判断指定按键是否被按下
bool Keyboard::KeyIsPressed(unsigned char keycode) const noexcept
{
    return keystates[keycode]; // 返回按键的状态
}

bool Keyboard::KeyJustPressed(unsigned char keycode) noexcept
{
    if (keystates[keycode] && !old_keystates[keycode])
    {
        old_keystates[keycode] = true;
        return true;
    }

    if (!keystates[keycode])
    {
        old_keystates[keycode] = false;
    }

    return false;
}

// 读取按键事件
std::optional<Keyboard::Event> Keyboard::ReadKey() noexcept
{
    // 如果按键缓冲区中有事件
    if (keybuffer.size() > 0u)
    {
        Keyboard::Event e = keybuffer.front(); // 获取缓冲区中的第一个事件
        keybuffer.pop();                       // 将其从缓冲区中移除
        return e;                              // 返回事件
    }
    return {}; // 返回空值表示没有事件
}

// 检查按键缓冲区是否为空
bool Keyboard::KeyIsEmpty() const noexcept
{
    return keybuffer.empty(); // 返回缓冲区是否为空
}

// 读取字符事件
std::optional<char> Keyboard::ReadChar() noexcept
{
    // 如果字符缓冲区中有字符
    if (charbuffer.size() > 0u)
    {
        unsigned char charcode = charbuffer.front(); // 获取缓冲区中的第一个字符
        charbuffer.pop();                            // 将其从缓冲区中移除
        return charcode;                             // 返回字符
    }
    return {}; // 返回空值表示没有字符
}

// 检查字符缓冲区是否为空
bool Keyboard::CharIsEmpty() const noexcept
{
    return charbuffer.empty(); // 返回字符缓冲区是否为空
}

// 清空按键事件缓冲区
void Keyboard::FlushKey() noexcept
{
    keybuffer = std::queue<Event>(); // 清空按键缓冲区
}

// 清空字符事件缓冲区
void Keyboard::FlushChar() noexcept
{
    charbuffer = std::queue<char>(); // 清空字符缓冲区
}

// 清空所有缓冲区
void Keyboard::Flush() noexcept
{
    FlushKey();  // 清空按键缓冲区
    FlushChar(); // 清空字符缓冲区
}

// 启用自动重复
void Keyboard::EnableAutorepeat() noexcept
{
    autorepeatEnabled = true; // 将自动重复标记设为 true
}

// 禁用自动重复
void Keyboard::DisableAutorepeat() noexcept
{
    autorepeatEnabled = false; // 将自动重复标记设为 false
}

// 检查自动重复是否启用
bool Keyboard::AutorepeatIsEnabled() const noexcept
{
    return autorepeatEnabled; // 返回自动重复状态
}

// 处理按键按下事件
void Keyboard::OnKeyPressed(unsigned char keycode) noexcept
{
    keystates[keycode] = true; // 设置按键状态为按下
    keybuffer.push(Keyboard::Event(Keyboard::Event::Type::Press, keycode)); // 将按键按下事件加入缓冲区
    TrimBuffer(keybuffer); // 修剪缓冲区，保持其大小不超过 bufferSize
}

// 处理按键释放事件
void Keyboard::OnKeyReleased(unsigned char keycode) noexcept
{
    keystates[keycode] = false; // 设置按键状态为释放
    keybuffer.push(Keyboard::Event(Keyboard::Event::Type::Release, keycode)); // 将按键释放事件加入缓冲区
    TrimBuffer(keybuffer); // 修剪缓冲区
}

// 处理字符输入事件
void Keyboard::OnChar(char character) noexcept
{
    charbuffer.push(character); // 将字符加入字符缓冲区
    TrimBuffer(charbuffer);     // 修剪缓冲区
}

// 清除所有按键的状态
void Keyboard::ClearState() noexcept
{
    old_keystates.reset();
    keystates.reset(); // 将所有按键的状态重置为未按下
}


// 修剪缓冲区，保持其大小不超过 bufferSize
template<typename T>
void Keyboard::TrimBuffer(std::queue<T>& buffer) noexcept
{
    while (buffer.size() > bufferSize)
    {
        buffer.pop(); // 当缓冲区大小超过指定值时，移除最早的事件
    }
}
