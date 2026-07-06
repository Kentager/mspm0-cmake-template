#include "oled_app.h"
#include "oled.h"
#include "oled_menu.h"
#include "motor_app.h"
#include <stdio.h>

typedef struct {
    float left_m_s;
    float right_m_s;
} MenuSpeedCmd_t;

typedef struct {
    float target_yaw;
} MenuYawCmd_t;

typedef struct {
    float left_m;
    float right_m;
    float speed_m_s;
} MenuDriveCmd_t;

static MenuManager_t g_menu_mgr;
static volatile uint8_t g_auto_run_enabled = 1U;
static float g_pitch;
static float g_roll;
static float g_yaw;

static void MenuAction_Brake(void *data);
static void MenuAction_SetSpeed(void *data);
static void MenuAction_SetTargetYaw(void *data);
static void MenuAction_Drive(void *data);
static void MenuAction_ResetDistance(void *data);
static void MenuAction_ShowYaw(void *data);
static void MenuAction_ShowVelocity(void *data);
static void MenuAction_ShowDistance(void *data);
static void MenuAction_ToggleAutoRun(void *data);

static const MenuSpeedCmd_t g_menu_speed_forward = {0.2f, 0.2f};
static const MenuSpeedCmd_t g_menu_speed_backward = {-0.2f, -0.2f};
static const MenuYawCmd_t g_menu_yaw_0 = {0.0f};
static const MenuYawCmd_t g_menu_yaw_90 = {90.0f};
static const MenuYawCmd_t g_menu_yaw_n90 = {-90.0f};
static const MenuYawCmd_t g_menu_yaw_180 = {180.0f};
static const MenuDriveCmd_t g_menu_drive_02 = {0.2f, 0.2f, 0.15f};
static const MenuDriveCmd_t g_menu_drive_05 = {0.5f, 0.5f, 0.15f};

static MenuItem_t g_menu_items[] = {
    {"Root",     -1,  1, -1, 0, 0},
    {"Motion",    0,  5,  2, 0, 0},
    {"Distance",  0, 12,  3, 0, 0},
    {"Status",    0, 15,  4, 0, 0},
    {"Demo",      0, 18, -1, 0, 0},

    {"Brake",      1, -1,  6, MenuAction_Brake, 0},
    {"Forward",    1, -1,  7, MenuAction_SetSpeed, (void *)&g_menu_speed_forward},
    {"Backward",   1, -1,  8, MenuAction_SetSpeed, (void *)&g_menu_speed_backward},
    {"Turn 0",     1, -1,  9, MenuAction_SetTargetYaw, (void *)&g_menu_yaw_0},
    {"Turn 90",    1, -1, 10, MenuAction_SetTargetYaw, (void *)&g_menu_yaw_90},
    {"Turn -90",   1, -1, 11, MenuAction_SetTargetYaw, (void *)&g_menu_yaw_n90},
    {"Turn 180",   1, -1, -1, MenuAction_SetTargetYaw, (void *)&g_menu_yaw_180},

    {"Reset Odo",  2, -1, 13, MenuAction_ResetDistance, 0},
    {"Drive 0.2m", 2, -1, 14, MenuAction_Drive, (void *)&g_menu_drive_02},
    {"Drive 0.5m", 2, -1, -1, MenuAction_Drive, (void *)&g_menu_drive_05},

    {"Show Yaw",   3, -1, 16, MenuAction_ShowYaw, 0},
    {"Show Vel",   3, -1, 17, MenuAction_ShowVelocity, 0},
    {"Show Dist",  3, -1, -1, MenuAction_ShowDistance, 0},

    {"Auto Run",   4, -1, -1, MenuAction_ToggleAutoRun, 0},
};

static void Menu_DisableAutoRun(void)
{
    g_auto_run_enabled = 0U;
}

static void Menu_ShowTempPageTitle(const char *title)
{
    OLED_CLS();
    OLED_ShowString(1, 0, (char *)title, 1);
}

static void Menu_ShowTempPageHint(void)
{
    OLED_ShowString(8, 0, "Any key back", 1);
    g_menu_mgr.skip_render_once = 1U;
}

static void MenuAction_Brake(void *data)
{
    (void)data;
    Menu_DisableAutoRun();
    Motor_App_Brake();
}

static void MenuAction_SetSpeed(void *data)
{
    const MenuSpeedCmd_t *cmd = (const MenuSpeedCmd_t *)data;

    if (cmd == 0) {
        return;
    }

    Menu_DisableAutoRun();
    Motor_App_SetSpeed(cmd->left_m_s, cmd->right_m_s);
}

static void MenuAction_SetTargetYaw(void *data)
{
    const MenuYawCmd_t *cmd = (const MenuYawCmd_t *)data;

    if (cmd == 0) {
        return;
    }

    Menu_DisableAutoRun();
    Motor_App_SetTargetYaw(cmd->target_yaw);
}

static void MenuAction_Drive(void *data)
{
    const MenuDriveCmd_t *cmd = (const MenuDriveCmd_t *)data;

    if (cmd == 0) {
        return;
    }

    Menu_DisableAutoRun();
    Motor_App_Drive(cmd->left_m, cmd->right_m, cmd->speed_m_s);
}

static void MenuAction_ResetDistance(void *data)
{
    (void)data;
    Menu_DisableAutoRun();
    Motor_App_ResetDistance();
}

static void MenuAction_ShowYaw(void *data)
{
    char buf[24];

    (void)data;
    Menu_ShowTempPageTitle("Yaw Status");
    snprintf(buf, sizeof(buf), "Yaw:%+.2f", g_yaw);
    OLED_ShowString(3, 0, buf, 1);
    snprintf(buf, sizeof(buf), "Pitch:%+.2f", g_pitch);
    OLED_ShowString(4, 0, buf, 1);
    snprintf(buf, sizeof(buf), "Roll:%+.2f", g_roll);
    OLED_ShowString(5, 0, buf, 1);
    Menu_ShowTempPageHint();
}

static void MenuAction_ShowVelocity(void *data)
{
    char buf[24];

    (void)data;
    Menu_ShowTempPageTitle("Velocity");
    snprintf(buf, sizeof(buf), "VL:%+.2f", Motor_App_GetVelocityLeft());
    OLED_ShowString(3, 0, buf, 1);
    snprintf(buf, sizeof(buf), "VR:%+.2f", Motor_App_GetVelocityRight());
    OLED_ShowString(4, 0, buf, 1);
    Menu_ShowTempPageHint();
}

static void MenuAction_ShowDistance(void *data)
{
    char buf[24];

    (void)data;
    Menu_ShowTempPageTitle("Distance");
    snprintf(buf, sizeof(buf), "DL:%+.2f", Motor_App_GetDistanceLeft());
    OLED_ShowString(3, 0, buf, 1);
    snprintf(buf, sizeof(buf), "DR:%+.2f", Motor_App_GetDistanceRight());
    OLED_ShowString(4, 0, buf, 1);
    Menu_ShowTempPageHint();
}

static void MenuAction_ToggleAutoRun(void *data)
{
    (void)data;

    if (g_auto_run_enabled != 0U) {
        g_auto_run_enabled = 0U;
        Motor_App_Brake();
    } else {
        g_auto_run_enabled = 1U;
    }

    Menu_ShowTempPageTitle("Demo");
    OLED_ShowString(3, 0, g_auto_run_enabled != 0U ? "Auto Run ON" : "Auto Run OFF", 1);
    Menu_ShowTempPageHint();
}

void OLED_AppInit(void)
{
    OLED_Init();
    Menu_Init(&g_menu_mgr, g_menu_items,
              sizeof(g_menu_items) / sizeof(g_menu_items[0]));
    Menu_Render(&g_menu_mgr);
}

void OLED_AppHandleKey(key_e key)
{
    Menu_HandleKey(&g_menu_mgr, key);
}

void OLED_AppSetAttitude(float pitch, float roll, float yaw)
{
    g_pitch = pitch;
    g_roll = roll;
    g_yaw = yaw;
}

uint8_t OLED_AppIsAutoRunEnabled(void)
{
    return g_auto_run_enabled;
}
