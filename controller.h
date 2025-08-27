
#pragma once
// collision.h




void InitController();
void UninitController();
void UpdateController();

bool GetControllerPress(int button);
bool GetControllerTrigger(int button);

Float2 GetControllerLeftStick();
Float2 GetControllerRightStick();

void SetControllerLeftVibration(int frame);
void SetControllerRightVibration(int frame);

Float3 GetControllerLeftAcceleration();
Float3 GetControllerRightAcceleration();
Float3 GetControllerLeftAngle();
Float3 GetControllerRightAngle();

bool GetControllerTouchScreen();
Float2 GetControllerTouchScreenPosition();

bool CheckCollisionSAT(Float2 PosA, Float2 PosB, Float2 SizeA, Polygon_* SizeB);
bool CheckCollision_Rect_Triangle(Float2 rectPos, Float2 rectSize, Float2 tri[3]);