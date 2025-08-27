#pragma once

#define GAME_DEBUG

#ifndef GAME_DEBUG
#ifdef _DEBUG
#include "GamepadManager_DEDUG.h"
#endif // DEBUG
#endif // !GAME_DEBUG



#ifndef INCLUDE_IN
#define INCLUDE_IN

#include <iostream>  // 标准输入输出库，用于处理控制台输入输出
#include <Windows.h> // Windows API，提供与操作系统交互的功能，包括窗口、线程、输入设备管理等
#include <Xinput.h>  // XInput库，用于与Xbox手柄和其他兼容设备进行交互
#include <string>    // C++标准库，提供std::string类用于处理字符串操作
#include <thread>    // C++11标准线程库，用于创建和管理多线程
#include <mutex>     // C++11标准库中的互斥锁，用于线程同步，避免资源竞争
#include <locale>    // C++标准库，用于处理区域设置和字符分类
#include <codecvt>   // C++标准库，用于处理字符编码转换，特别是宽字符和多字节字符之间的转换
#include <sstream>   // C++标准库，提供字符串流，用于格式化输入输出

// 连接到XInput库，允许程序与Xbox兼容的手柄设备进行交互
#pragma comment(lib, "Xinput.lib")

#include <dinput.h>  // DirectInput库，提供对更广泛的输入设备（如旧式手柄、操纵杆）的支持

// 链接DirectInput所需的库
#pragma comment(lib, "dinput8.lib")  // DirectInput8库，支持DirectInput API，用于输入设备的管理
#pragma comment(lib, "dxguid.lib")   // DirectX GUID库，包含创建DirectInput设备所需的GUID定义

#include <atomic>
#endif

// 声明DirectInput设备接口指针，用于手柄设备的输入控制
extern LPDIRECTINPUTDEVICE8 pGameController;

//使用 DirectInput 进行震动
void VibrateWithDirectInput(IDirectInputDevice8* pGameController, LONG magnitude);

// DirectInput手柄设备枚举的回调函数，用于获取可用的手柄设备
BOOL CALLBACK EnumJoysticksCallback(const DIDEVICEINSTANCE* pdidInstance, VOID* pContext);

// 定义一个游戏手柄参数类，用于存储手柄按钮的状态和摇杆/触发器的数值
#ifndef GAMEPAD_PAR
#define GAMEPAD_PAR

// 定义一个二维向量结构体，用于存储摇杆的X轴和Y轴位置
struct vector2
{
    int X;
    int Y;
};

class Gamepad_Parameters
{
public:
    // 定义手柄按键的状态变量，默认状态为未按下（false）
    bool A = false;              // A键（对应手柄的A按钮，通常用于确认或跳跃）
    bool B = false;              // B键（对应手柄的B按钮，通常用于取消或攻击）
    bool X = false;              // X键（对应手柄的X按钮，通常用于动作或特殊技能）
    bool Y = false;              // Y键（对应手柄的Y按钮，通常用于切换或互动）

    bool DPad_Up = false;        // 方向键上（手柄上的方向键向上，用于控制移动或选择）
    bool DPad_Down = false;      // 方向键下（手柄上的方向键向下，用于控制移动或选择）
    bool DPad_Left = false;      // 方向键左（手柄上的方向键向左，用于控制移动或选择）
    bool DPad_Right = false;     // 方向键右（手柄上的方向键向右，用于控制移动或选择）

    bool Left_Shoulder = false;  // 左肩键（手柄的左上方肩键，通常为L1键，用于辅助操作）
    bool Right_Shoulder = false; // 右肩键（手柄的右上方肩键，通常为R1键，用于辅助操作）

    bool Left_Trigger_Button = false;  // 左触发器按钮（某些手柄将触发器也作为一个按钮）
    bool Right_Trigger_Button = false; // 右触发器按钮

    // 新增的扩展按键：按键1-按键4（可扩展为其他按钮）
    bool Button1 = false;  // 扩展按键1（根据手柄实际布局映射）
    bool Button2 = false;  // 扩展按键2
    bool Button3 = false;  // 扩展按键3
    bool Button4 = false;  // 扩展按键4

    bool Button5 = false;  // 扩展按键5（可用于第三方手柄的其他功能键）
    bool Button6 = false;  // 扩展按键6
    bool Button7 = false;  // 扩展按键7
    bool Button8 = false;  // 扩展按键8

    // 定义摇杆位置，使用 vector2 结构体存储X和Y轴数据
    vector2 leftThumb = { 0, 0 };   // 左摇杆
    vector2 rightThumb = { 0, 0 };  // 右摇杆

    // 定义手柄触发器的值（范围：0-255）
    int leftTrigger = 0;   // 左触发器
    int rightTrigger = 0;  // 右触发器

    // 定义更多按键，例如手柄上的其他扩展功能键（例如菜单键、选项键等）
    bool Menu_Button = false;     // 菜单按键
    bool Options_Button = false;  // 选项按键
    bool Left_Middle_Button = false;  // 手柄左侧中间按键
    bool Right_Middle_Button = false; // 手柄右侧中间按键
    bool Central_Button = false;      // 中央的功能键（例如Xbox的中央按钮，或第三方手柄的自定义键）

public:
    // 重置手柄按键状态的函数，将所有按钮状态设为 false
    void Reset() {
        A = B = X = Y = DPad_Up = DPad_Down = DPad_Left = DPad_Right = false;
        Left_Shoulder = Right_Shoulder = false;
        Left_Trigger_Button = Right_Trigger_Button = false;

        // 重置所有扩展按键
        Button1 = Button2 = Button3 = Button4 = false;
        Button5 = Button6 = Button7 = Button8 = false;

        // 重置中间功能键
        Menu_Button = Options_Button = Left_Middle_Button = Right_Middle_Button = Central_Button = false;
    }
};
#endif

// 手柄检测管理类，负责检测手柄输入、控制震动、处理状态等
class GamepadDetectorManager
{
private:
    std::atomic<bool> isControllerThreadRunning = false;  // 确保 runControllerStateCheck() 线程不重复创建

private:
    std::string pressedButtons;  // 存储按下的按钮名称
    WORD buttonStates;        // 存储按键状态的二进制表示
    std::mutex stateMutex; // 互斥锁，确保线程安全的访问手柄状态

    // 设置手柄震动，左右震动电机的震动强度取值范围为0到65535
    // 获取手柄状态函数，处理输入
    void GetControllerState();
    // 初始化函数，可能包含初始化DirectInput等操作
    void initialize();
    // 在独立线程中获取手柄状态
    void runControllerStateCheck();

    void gamepadAvailable();

private:

    int leftVibrateController = 0;  // 左马达震动强度
    int rightVibrateController = 0; // 右马达震动强度

#ifndef GAME_DEBUG
#ifdef _DEBUG
    createWindow Gamepad_Parameters_win;
#endif // DEBUG
#endif

public:
    // 用于控制手柄的震动
    bool vibrateController = false;  // 手柄是否震动的状态
    std::string Detected_Device = ""; // 存储检测到的设备名称
    const int stickSensitivity = 2000;  // 定义摇杆灵敏度缩放系数
    Gamepad_Parameters Gamepad_Parameters; // 手柄参数实例
public:
    void VibrateController(WORD leftMotorSpeed, WORD rightMotorSpeed);
    // 启动窗口并在独立线程中显示输出
    void run();


};


