#include "../includes/cpu.h"
#include "../includes/apu.h"
#include "../includes/input.h"
#include "raylib.h"

#if defined(PLATFORM_NX)
#include <switch.h>
#endif

joypad joypad1;

bool update_keys() {
    for (int i = 0; i < 4; i++) {
        if (IsKeyDown(set.keyboard_keys[i]) || IsGamepadButtonDown(0, set.gamepad_keys[i]))
            joypad1.dpad[i] = 0;
        else
            joypad1.dpad[i] = 1;
        if (IsKeyDown(set.keyboard_keys[i+4]) || IsGamepadButtonDown(0, set.gamepad_keys[i+4]))
            joypad1.btn[i] = 0;
        else
            joypad1.btn[i] = 1;
    }

    if (IsKeyPressed(set.fast_forward_key))
        joypad1.fast_forward = 10;
    if (IsKeyReleased(set.fast_forward_key))
        joypad1.fast_forward = 1;

    for (int i = 0; i < 8; i++) {
        if (IsKeyPressed(set.keyboard_keys[i]))
            return true;
        if (IsGamepadButtonDown(0, set.gamepad_keys[i]))
            return true;
    }
    return false;
}


#if defined(PLATFORM_NX)
struct {
    HidSixAxisSensorHandle handles[4];
    PadState pad;
}switch_axis;
#endif

void init_axis() {
    #if defined(PLATFORM_NX)
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);

    // Initialize the default gamepad (which reads handheld mode inputs as well as the first connected controller)
    padInitializeDefault(&switch_axis.pad);

    // It's necessary to initialize these separately as they all have different handle values
    hidGetSixAxisSensorHandles(&switch_axis.handles[0], 1, HidNpadIdType_Handheld, HidNpadStyleTag_NpadHandheld);
    hidGetSixAxisSensorHandles(&switch_axis.handles[1], 1, HidNpadIdType_No1,      HidNpadStyleTag_NpadFullKey);
    hidGetSixAxisSensorHandles(&switch_axis.handles[2], 2, HidNpadIdType_No1,      HidNpadStyleTag_NpadJoyDual);
    hidStartSixAxisSensor(switch_axis.handles[0]);
    hidStartSixAxisSensor(switch_axis.handles[1]);
    hidStartSixAxisSensor(switch_axis.handles[2]);
    hidStartSixAxisSensor(switch_axis.handles[3]);
    #endif
}

int get_x_accel() {
    #if defined(PLATFORM_NX)
        padUpdate(&switch_axis.pad);
        HidSixAxisSensorState sixaxis = {0};
        u64 style_set = padGetStyleSet(&switch_axis.pad);
        if (style_set & HidNpadStyleTag_NpadHandheld)
            hidGetSixAxisSensorStates(switch_axis.handles[0], &sixaxis, 1);
        else if (style_set & HidNpadStyleTag_NpadFullKey)
            hidGetSixAxisSensorStates(switch_axis.handles[1], &sixaxis, 1);
        else if (style_set & HidNpadStyleTag_NpadJoyDual) {
            // For JoyDual, read from either the Left or Right Joy-Con depending on which is/are connected
            u64 attrib = padGetAttributes(&switch_axis.pad);
            if (attrib & HidNpadAttribute_IsLeftConnected)
                hidGetSixAxisSensorStates(switch_axis.handles[2], &sixaxis, 1);
            else if (attrib & HidNpadAttribute_IsRightConnected)
                hidGetSixAxisSensorStates(switch_axis.handles[3], &sixaxis, 1);
        }
        return (int)(sixaxis.acceleration.x * -128);
    #endif
    switch (set.motion_style) {
        case 0:
            return (int)(GetGamepadAxisMovement(0, 0) * -64);
        case 1:
            if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
                return (int)((GetMouseX() - (GetScreenWidth()/2)) * -1);
            else
                return 0;
        default:
            return 0;
    }
}

int get_y_accel() {
    #if defined(PLATFORM_NX)
    padUpdate(&switch_axis.pad);
        HidSixAxisSensorState sixaxis = {0};
        u64 style_set = padGetStyleSet(&switch_axis.pad);
        if (style_set & HidNpadStyleTag_NpadHandheld)
            hidGetSixAxisSensorStates(switch_axis.handles[0], &sixaxis, 1);
        else if (style_set & HidNpadStyleTag_NpadFullKey)
            hidGetSixAxisSensorStates(switch_axis.handles[1], &sixaxis, 1);
        else if (style_set & HidNpadStyleTag_NpadJoyDual) {
            // For JoyDual, read from either the Left or Right Joy-Con depending on which is/are connected
            u64 attrib = padGetAttributes(&switch_axis.pad);
            if (attrib & HidNpadAttribute_IsLeftConnected)
                hidGetSixAxisSensorStates(switch_axis.handles[2], &sixaxis, 1);
            else if (attrib & HidNpadAttribute_IsRightConnected)
                hidGetSixAxisSensorStates(switch_axis.handles[3], &sixaxis, 1);
        }
        return (int)(sixaxis.acceleration.y * 128);
    #endif
    switch (set.motion_style) {
        case 0:
            return (int)(GetGamepadAxisMovement(0, 1) * -64);
        case 1:
            if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
                return (int)((GetMouseY() - (GetScreenHeight()/2)) * -1);
            else
                return 0;
        default:
            return 0;
    }
}