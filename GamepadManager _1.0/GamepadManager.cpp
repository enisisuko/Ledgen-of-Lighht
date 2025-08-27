#include "GamepadManager.h"


LPDIRECTINPUT8 pDirectInput = NULL;
LPDIRECTINPUTDEVICE8 pGameController = NULL;
std::string name;

// 将 TCHAR 转换为 std::string
std::string TCHARToString(const TCHAR* tcharStr) {
    // 如果项目是 Unicode 模式
#ifdef UNICODE
    int len = WideCharToMultiByte(CP_ACP, 0, tcharStr, -1, NULL, 0, NULL, NULL);
    std::string str(len, '\0');
    WideCharToMultiByte(CP_ACP, 0, tcharStr, -1, &str[0], len, NULL, NULL);
    return str;
#else
    // 如果项目是多字节模式，直接返回字符串
    return std::string(tcharStr);
#endif
}

// 灵敏度调整函数，对摇杆输入值进行微调，如果数值很小则归0
int new_thuml(int old) {
    if (old < 2 && old > -2) {
        old = 0;
    }
    return old;
}

// 检测哪个按键被按下
std::string GetPressedButtons(WORD buttons, Gamepad_Parameters& Gamepad_Parameters) {
    std::string result = "";

    Gamepad_Parameters.Reset();  // 重置手柄状态，确保每次检测到的状态都是最新的

    // 检测常见按键
    if (buttons & XINPUT_GAMEPAD_A) {
        Gamepad_Parameters.A = true;
        result += "A ";
    }
    if (buttons & XINPUT_GAMEPAD_B) {
        Gamepad_Parameters.B = true;
        result += "B ";
    }
    if (buttons & XINPUT_GAMEPAD_X) {
        Gamepad_Parameters.X = true;
        result += "X ";
    }
    if (buttons & XINPUT_GAMEPAD_Y) {
        Gamepad_Parameters.Y = true;
        result += "Y ";
    }

    // 检测方向键
    if (buttons & XINPUT_GAMEPAD_DPAD_UP) {
        Gamepad_Parameters.DPad_Up = true;
        result += "DPad Up ";
    }
    if (buttons & XINPUT_GAMEPAD_DPAD_DOWN) {
        Gamepad_Parameters.DPad_Down = true;
        result += "DPad Down ";
    }
    if (buttons & XINPUT_GAMEPAD_DPAD_LEFT) {
        Gamepad_Parameters.DPad_Left = true;
        result += "DPad Left ";
    }
    if (buttons & XINPUT_GAMEPAD_DPAD_RIGHT) {
        Gamepad_Parameters.DPad_Right = true;
        result += "DPad Right ";
    }

    // 检测肩键
    if (buttons & XINPUT_GAMEPAD_LEFT_SHOULDER) {
        Gamepad_Parameters.Left_Shoulder = true;
        result += "Left Shoulder ";
    }
    if (buttons & XINPUT_GAMEPAD_RIGHT_SHOULDER) {
        Gamepad_Parameters.Right_Shoulder = true;
        result += "Right Shoulder ";
    }

    // 检测扩展按键，如Menu和Options
    if (buttons & XINPUT_GAMEPAD_BACK) {
        Gamepad_Parameters.Menu_Button = true;
        result += "Menu Button ";
    }
    if (buttons & XINPUT_GAMEPAD_START) {
        Gamepad_Parameters.Options_Button = true;
        result += "Options Button ";
    }


    // 如果没有按键按下
    if (result == "") {
        result = "No buttons pressed";
    }

    return result;
}


// 获取手柄状态信息，并显示按键和摇杆状态，同时处理震动逻辑
void GamepadDetectorManager::GetControllerState() {
    if (Detected_Device.find("Xbox") != std::string::npos) {
        // 使用 XInput 读取 Xbox 手柄输入
        XINPUT_STATE state;
        ZeroMemory(&state, sizeof(XINPUT_STATE));
        //VibrateController(0, 0);

        // 获取手柄状态
        DWORD dwResult = XInputGetState(0, &state);
        if (dwResult == ERROR_SUCCESS) {
            // 获取并处理摇杆和触发键的值
            // 加锁并更新手柄状态
            std::lock_guard<std::mutex> lock(stateMutex);

            Gamepad_Parameters.leftThumb.X = new_thuml(state.Gamepad.sThumbLX / stickSensitivity); // 左摇杆X值
            Gamepad_Parameters.leftThumb.Y = new_thuml(state.Gamepad.sThumbLY / stickSensitivity); // 左摇杆Y值
            Gamepad_Parameters.rightThumb.X = new_thuml(state.Gamepad.sThumbRX / stickSensitivity);// 右摇杆X值
            Gamepad_Parameters.rightThumb.Y = new_thuml(state.Gamepad.sThumbRY / stickSensitivity);// 右摇杆Y值

            Gamepad_Parameters.leftTrigger = (int)state.Gamepad.bLeftTrigger;   // 左触发键值
            Gamepad_Parameters.rightTrigger = (int)state.Gamepad.bRightTrigger; // 右触发键值



            buttonStates = state.Gamepad.wButtons;
            pressedButtons = GetPressedButtons(buttonStates, Gamepad_Parameters);


            // 如果触发键被按下，设置震动参数
            //if (true) {
            //    // 根据设置的震动参数调用震动函数
            //    VibrateController(state.Gamepad.bLeftTrigger, state.Gamepad.bRightTrigger);
            //}
#ifndef GAME_DEBUG
#ifdef _DEBUG
            Gamepad_Parameters_win.text_out(pressedButtons, buttonStates, Gamepad_Parameters);
#endif // DEBUG
#endif
        }
    }
    else {
        // 使用 DirectInput 读取第三方手柄输入
        if (pGameController) {
            DIJOYSTATE2 js; // DirectInput 的手柄状态结构体

            if (FAILED(pGameController->Poll())) {
                // 如果 Poll 失败，不执行任何操作，直接获取手柄状态
                // 可能是手柄不支持 Poll 模式
                if (FAILED(pGameController->Acquire())) {
                    MessageBox(NULL, "Failed to acquire the game controller after Poll failure.", "question", MB_OK | MB_ICONERROR);
                    return;
                }
            }

            if (SUCCEEDED(pGameController->GetDeviceState(sizeof(DIJOYSTATE2), &js))) {
                // 加锁并更新手柄状态
                std::lock_guard<std::mutex> lock(stateMutex);

                // 处理左摇杆
                Gamepad_Parameters.leftThumb.X = new_thuml(js.lX / stickSensitivity); // 左摇杆X值
                Gamepad_Parameters.leftThumb.Y = new_thuml(js.lY / stickSensitivity); // 左摇杆Y值

                // 处理右摇杆
                Gamepad_Parameters.rightThumb.X = new_thuml(js.lRx / stickSensitivity); // 右摇杆X值
                Gamepad_Parameters.rightThumb.Y = new_thuml(js.lRy / stickSensitivity); // 右摇杆Y值

                // 处理触发器
                Gamepad_Parameters.leftTrigger = js.lZ;   // DirectInput的Z轴通常用来表示触发器
                Gamepad_Parameters.rightTrigger = js.rglSlider[0]; // rglSlider[0]可用于表示另一个触发器

                // 处理按钮
                for (int i = 0; i < 128; ++i) {
                    if (js.rgbButtons[i] & 0x80) {
                        switch (i) {
                        case 0: Gamepad_Parameters.A = true; break;
                        case 1: Gamepad_Parameters.B = true; break;
                        case 2: Gamepad_Parameters.X = true; break;
                        case 3: Gamepad_Parameters.Y = true; break;
                        case 4: Gamepad_Parameters.Left_Shoulder = true; break;
                        case 5: Gamepad_Parameters.Right_Shoulder = true; break;
                        case 6: Gamepad_Parameters.Left_Trigger_Button = true; break;
                        case 7: Gamepad_Parameters.Right_Trigger_Button = true; break;
                        case 8: Gamepad_Parameters.Menu_Button = true; break;
                        case 9: Gamepad_Parameters.Options_Button = true; break;
                        default:
                            if (i >= 10 && i <= 17) {
                                // 扩展按键检测（根据手柄按键数量调整）
                                if (i == 10) Gamepad_Parameters.Button1 = true;
                                else if (i == 11) Gamepad_Parameters.Button2 = true;
                                else if (i == 12) Gamepad_Parameters.Button3 = true;
                                else if (i == 13) Gamepad_Parameters.Button4 = true;
                                else if (i == 14) Gamepad_Parameters.Button5 = true;
                                else if (i == 15) Gamepad_Parameters.Button6 = true;
                                else if (i == 16) Gamepad_Parameters.Button7 = true;
                                else if (i == 17) Gamepad_Parameters.Button8 = true;
                            }
                            break;
                        }
                    }
                }

                // D-Pad处理
                if (js.rgdwPOV[0] != -1) {
                    int pov = js.rgdwPOV[0] / 100;
                    if (pov == 0) Gamepad_Parameters.DPad_Up = true;
                    else if (pov == 4500) Gamepad_Parameters.DPad_Right = true;
                    else if (pov == 9000) Gamepad_Parameters.DPad_Down = true;
                    else if (pov == 13500) Gamepad_Parameters.DPad_Left = true;
                }
            }
            else
            {
                // 使用 XInput 读取 Xbox 手柄输入
                XINPUT_STATE state;
                ZeroMemory(&state, sizeof(XINPUT_STATE));
                //VibrateController(0, 0);

                // 获取手柄状态
                DWORD dwResult = XInputGetState(0, &state);
                if (dwResult == ERROR_SUCCESS) {
                    // 获取并处理摇杆和触发键的值
                    // 加锁并更新手柄状态
                    std::lock_guard<std::mutex> lock(stateMutex);

                    Gamepad_Parameters.leftThumb.X = new_thuml(state.Gamepad.sThumbLX / stickSensitivity); // 左摇杆X值
                    Gamepad_Parameters.leftThumb.Y = new_thuml(state.Gamepad.sThumbLY / stickSensitivity); // 左摇杆Y值
                    Gamepad_Parameters.rightThumb.X = new_thuml(state.Gamepad.sThumbRX / stickSensitivity);// 右摇杆X值
                    Gamepad_Parameters.rightThumb.Y = new_thuml(state.Gamepad.sThumbRY / stickSensitivity);// 右摇杆Y值

                    Gamepad_Parameters.leftTrigger = (int)state.Gamepad.bLeftTrigger;   // 左触发键值
                    Gamepad_Parameters.rightTrigger = (int)state.Gamepad.bRightTrigger; // 右触发键值



                    buttonStates = state.Gamepad.wButtons;
                    pressedButtons = GetPressedButtons(buttonStates, Gamepad_Parameters);


                    // 如果触发键被按下，设置震动参数
                    //if (true) {
                    //    // 根据设置的震动参数调用震动函数
                    //    VibrateController(state.Gamepad.bLeftTrigger, state.Gamepad.bRightTrigger);
                    //}
#ifndef GAME_DEBUG
#ifdef _DEBUG
                    Gamepad_Parameters_win.text_out(pressedButtons, buttonStates, Gamepad_Parameters);
#endif // DEBUG
#endif;

                }
            }
        }
#ifndef GAME_DEBUG
#ifdef _DEBUG
        Gamepad_Parameters_win.text_out(pressedButtons, buttonStates, Gamepad_Parameters);
#endif // DEBUG
#endif;
    }

}

// DirectInput枚举回调函数，用于识别连接的手柄设备
BOOL CALLBACK EnumJoysticksCallback(const DIDEVICEINSTANCE* pdidInstance, VOID* pContext) {
    
    // 输出设备名称，查看是否识别出手柄
    name = TCHARToString(pdidInstance->tszProductName);

    // 创建设备对象，绑定检测到的手柄
    if (FAILED(pDirectInput->CreateDevice(pdidInstance->guidInstance, &pGameController, NULL))) {
        return DIENUM_CONTINUE;  // 继续枚举其他设备
    }
    return DIENUM_STOP;  // 找到手柄后停止枚举
}

void GamepadDetectorManager::VibrateController(WORD leftMotorSpeed, WORD rightMotorSpeed)
{

    if (Detected_Device.find("Xbox") != std::string::npos)
    {
        XINPUT_VIBRATION vibration;
        ZeroMemory(&vibration, sizeof(XINPUT_VIBRATION));

        // 设置震动电机速度
        vibration.wLeftMotorSpeed = (int)((int)leftMotorSpeed / 255.0f * 65535);
        vibration.wRightMotorSpeed = (int)((int)rightMotorSpeed / 255.0f * 65535);

        // 应用震动设置
        if (XInputSetState(0, &vibration) == ERROR_SUCCESS) {
            vibrateController = true;
        }
        else {
            vibrateController = false;
        }

    }
    else
    {
        // DirectInput 震动
        LONG forceMagnitude = (LONG)((int)leftMotorSpeed / 255.0f * DI_FFNOMINALMAX);
        VibrateWithDirectInput(pGameController, forceMagnitude);
    }
}

void GamepadDetectorManager::initialize()
{
    // 初始化 DirectInput，获取 DirectInput 对象的指针 pDirectInput
    if (FAILED(DirectInput8Create(GetModuleHandle(NULL), DIRECTINPUT_VERSION, IID_IDirectInput8, (VOID**)&pDirectInput, NULL))) {
        MessageBox(NULL, "Failed to initialize DirectInput.", "BUG", MB_OK | MB_ICONERROR);
        return; // 如果 DirectInput 初始化失败，输出错误信息并退出函数
    }

    // 枚举系统中的所有手柄设备，调用枚举回调函数 EnumJoysticksCallback
    if (FAILED(pDirectInput->EnumDevices(DI8DEVCLASS_GAMECTRL, EnumJoysticksCallback, NULL, DIEDFL_ATTACHEDONLY))) {
        MessageBox(NULL, "Failed to enumerate devices.", "BUG", MB_OK | MB_ICONERROR);
        return; // 如果枚举设备失败，输出错误信息并退出函数
    }

    // 如果成功连接到手柄，设置数据格式并进入主循环
    if (pGameController) {
        Detected_Device = name;
        // 设置手柄数据格式，使用 DirectInput 预定义的格式 c_dfDIJoystick 来读取手柄输入
        if (FAILED(pGameController->SetDataFormat(&c_dfDIJoystick))) {
            MessageBox(NULL, "Failed to set data format.", "BUG", MB_OK | MB_ICONERROR);
            return; // 如果设置数据格式失败，输出错误信息并退出函数
        }

        // 设置合作级别，将手柄输入与当前控制台窗口共享，允许后台输入
        if (FAILED(pGameController->SetCooperativeLevel(GetConsoleWindow(), DISCL_BACKGROUND | DISCL_NONEXCLUSIVE))) {
            MessageBox(NULL, "Failed to set cooperative level.", "BUG", MB_OK | MB_ICONERROR);
            return; // 如果设置合作级别失败，输出错误信息并退出函数
        }

    }
    else {
        MessageBox(NULL, "Controller not connected.", "question", MB_OK | MB_ICONERROR);
        // 清理资源
        if (pGameController) pGameController->Unacquire(); // 释放手柄设备的占用，使其他程序可以使用手柄
        if (pDirectInput) pDirectInput->Release(); // 释放 DirectInput 对象，清理资源
    }
   

    
}


void GamepadDetectorManager::runControllerStateCheck() {
    while (isControllerThreadRunning.load()) {  // **如果 isControllerThreadRunning 变为 false，则线程退出**

        GetControllerState(); // 不断获取手柄状态并更新到控制台
        // 如果没有检测到按键按下，可以增加 sleep 时间，减少 CPU 负担
        if (buttonStates == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
        else {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

// 启动窗口并在独立线程中显示输出
void GamepadDetectorManager::run() {

    std::thread GamepadAv(&GamepadDetectorManager::gamepadAvailable, this);
    GamepadAv.detach();


}




void GamepadDetectorManager::gamepadAvailable() {

    bool wasConnected = false;  // 记录上一次的连接状态

    while (1)
    {
        XINPUT_STATE state;
        ZeroMemory(&state, sizeof(XINPUT_STATE));

        DWORD result = XInputGetState(0, &state);  // 仅检测 1P 手柄（索引 0）

        if (result == ERROR_SUCCESS && !wasConnected)
        {


            initialize();
#ifndef GAME_DEBUG
#ifdef _DEBUG
            if (!isWindowThreadRunning.load()) {  // 只在未运行时启动窗口线程
                //MessageBox(NULL, L"no off", L"BUG", MB_OK | MB_ICONERROR);
                isWindowThreadRunning.store(true);
                std::thread windowThread(&createWindow::run, &Gamepad_Parameters_win);
                windowThread.detach();
            }
#endif // DEBUG
#endif
            if (!isControllerThreadRunning.load())
            {
                isControllerThreadRunning.store(true);
                // 启动获取手柄状态的线程，传递当前对象的指针
                std::thread controllerThread(&GamepadDetectorManager::runControllerStateCheck, this);
                controllerThread.detach(); // 让线程独立运行，主线程不会等待它结束
            }
            
        }
        else if (result != ERROR_SUCCESS && wasConnected)  // 手柄断开
        {
            isControllerThreadRunning.store(false);  // 让 `runControllerStateCheck()` 线程自行退出
#ifndef GAME_DEBUG
#ifdef _DEBUG
            isWindowThreadRunning.store(false);  // 让窗口线程也停止
#endif // DEBUG
#endif;
        }

        wasConnected = (result == ERROR_SUCCESS);  // 记录当前连接状态

        std::this_thread::sleep_for(std::chrono::seconds(2));  // **2 秒检查一次，防止 CPU 过载**

    }

}

void VibrateWithDirectInput(IDirectInputDevice8* pGameController, LONG magnitude)
{
    if (!pGameController) return;

    DIEFFECT eff;
    ZeroMemory(&eff, sizeof(DIEFFECT));
    eff.dwSize = sizeof(DIEFFECT);
    eff.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    eff.dwDuration = INFINITE; // 震动持续时间
    eff.dwGain = DI_FFNOMINALMAX; // 震动强度
    eff.dwTriggerButton = DIEB_NOTRIGGER;
    eff.cAxes = 1; // 只影响一个轴（震动）
    eff.rgdwAxes = NULL;
    eff.rglDirection = NULL;
    eff.lpEnvelope = NULL;
    eff.cbTypeSpecificParams = sizeof(DICONSTANTFORCE);
    eff.lpvTypeSpecificParams = &magnitude;

    IDirectInputEffect* pEffect = nullptr;
    HRESULT hr = pGameController->CreateEffect(GUID_ConstantForce, &eff, &pEffect, NULL);
    if (SUCCEEDED(hr))
    {
        pEffect->Start(1, 0); // 触发震动
    }
}


