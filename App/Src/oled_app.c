#include "oled_app.h"
#include "oled.h"
#include "oled_menu.h"
#include "motor_app.h"
#include <stdio.h>

typedef struct {
    float left_m_s;
    float right_m_s;
} MenuSpeedCmd_t;

typedef enum {
    OLED_PAGE_MENU = 0,
    OLED_PAGE_YAW,
    OLED_PAGE_VELOCITY,
    OLED_PAGE_DISTANCE,
} OLED_Page_t;

static MenuManager_t g_menu_mgr;
static volatile uint8_t g_auto_run_enabled = 1U;
static float g_pitch;
static float g_roll;
static float g_yaw;
static OLED_Page_t g_current_page = OLED_PAGE_MENU;

static void MenuAction_SetSpeed(void *data);

static void MenuAction_ShowYaw(void *data);
static void MenuAction_ShowVelocity(void *data);
static void MenuAction_ShowDistance(void *data);
static void OLED_AppDrawYawPageFrame(void);
static void OLED_AppDrawVelocityPageFrame(void);
static void OLED_AppDrawDistancePageFrame(void);
static void OLED_AppUpdateYawPageData(void);
static void OLED_AppUpdateVelocityPageData(void);
static void OLED_AppUpdateDistancePageData(void);

static const MenuSpeedCmd_t g_menu_speed_forward = {0.2f, 0.2f};
static const MenuSpeedCmd_t g_menu_speed_backward = {-0.2f, -0.2f};


static MenuItem_t g_menu_items[] = {
    {"Root",     -1,  1, -1, 0, 0},
    {"Motion",    0,  3,  2, 0, 0},
    {"Status",    0,  5, -1, 0, 0},

    {"Forward",    1, -1,  4, MenuAction_SetSpeed, (void *)&g_menu_speed_forward},
    {"Backward",   1, -1,  -1, MenuAction_SetSpeed, (void *)&g_menu_speed_backward},

    {"Show Yaw",   2, -1, 6, MenuAction_ShowYaw, 0},
    {"Show Vel",   2, -1, 7, MenuAction_ShowVelocity, 0},
    {"Show Dist",  2, -1, -1, MenuAction_ShowDistance, 0},

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

static void Menu_FormatSignedFixed(char *buf, uint32_t size,
                                   const char *label, float value)
{
    int value_i = (int)(value * 100.0f);
    int value_abs = value_i < 0 ? -value_i : value_i;

    snprintf(buf, size, "%s%s%d.%02d",
             label,
             value_i < 0 ? "-" : "",
             value_abs / 100,
             value_abs % 100);
}

static void OLED_AppDrawYawPageFrame(void)
{
    Menu_ShowTempPageTitle("Yaw Status");
    OLED_ShowString(3, 0, "Yaw:", 1);
    OLED_ShowString(4, 0, "Pitch:", 1);
    OLED_ShowString(5, 0, "Roll:", 1);
    Menu_ShowTempPageHint();
}

static void OLED_AppDrawVelocityPageFrame(void)
{
    Menu_ShowTempPageTitle("Velocity");
    OLED_ShowString(3, 0, "VL:", 1);
    OLED_ShowString(4, 0, "VR:", 1);
    Menu_ShowTempPageHint();
}

static void OLED_AppDrawDistancePageFrame(void)
{
    Menu_ShowTempPageTitle("Distance");
    OLED_ShowString(3, 0, "DL:", 1);
    OLED_ShowString(4, 0, "DR:", 1);
    Menu_ShowTempPageHint();
}

static void OLED_AppUpdateYawPageData(void)
{
    char buf[24];

    Menu_FormatSignedFixed(buf, sizeof(buf), "", g_yaw);
    OLED_ShowString(3, 7, "        ", 1);
    OLED_ShowString(3, 7, buf, 1);
    Menu_FormatSignedFixed(buf, sizeof(buf), "", g_pitch);
    OLED_ShowString(4, 7, "        ", 1);
    OLED_ShowString(4, 7, buf, 1);
    Menu_FormatSignedFixed(buf, sizeof(buf), "", g_roll);
    OLED_ShowString(5, 7, "        ", 1);
    OLED_ShowString(5, 7, buf, 1);
}

static void OLED_AppUpdateVelocityPageData(void)
{
    char buf[24];

    Menu_FormatSignedFixed(buf, sizeof(buf), "", Motor_App_GetVelocityLeft());
    OLED_ShowString(3, 4, "        ", 1);
    OLED_ShowString(3, 4, buf, 1);
    Menu_FormatSignedFixed(buf, sizeof(buf), "", Motor_App_GetVelocityRight());
    OLED_ShowString(4, 4, "        ", 1);
    OLED_ShowString(4, 4, buf, 1);
}

static void OLED_AppUpdateDistancePageData(void)
{
    char buf[24];

    Menu_FormatSignedFixed(buf, sizeof(buf), "", Motor_App_GetDistanceLeft());
    OLED_ShowString(3, 4, "        ", 1);
    OLED_ShowString(3, 4, buf, 1);
    Menu_FormatSignedFixed(buf, sizeof(buf), "", Motor_App_GetDistanceRight());
    OLED_ShowString(4, 4, "        ", 1);
    OLED_ShowString(4, 4, buf, 1);
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


static void MenuAction_ShowYaw(void *data)
{
    (void)data;
    g_current_page = OLED_PAGE_YAW;
    OLED_AppDrawYawPageFrame();
    OLED_AppUpdateYawPageData();
}

static void MenuAction_ShowVelocity(void *data)
{
    (void)data;
    g_current_page = OLED_PAGE_VELOCITY;
    OLED_AppDrawVelocityPageFrame();
    OLED_AppUpdateVelocityPageData();
}

static void MenuAction_ShowDistance(void *data)
{
    (void)data;
    g_current_page = OLED_PAGE_DISTANCE;
    OLED_AppDrawDistancePageFrame();
    OLED_AppUpdateDistancePageData();
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
    if ((g_current_page != OLED_PAGE_MENU) && (g_menu_mgr.skip_render_once != 0U)) {
        g_current_page = OLED_PAGE_MENU;
    }

    Menu_HandleKey(&g_menu_mgr, key);
}

void OLED_AppSetAttitude(float pitch, float roll, float yaw)
{
    g_pitch = pitch;
    g_roll = roll;
    g_yaw = yaw;
}

void OLED_AppRefresh(void)
{
    if (g_menu_mgr.skip_render_once == 0U) {
        return;
    }

    switch (g_current_page) {
        case OLED_PAGE_YAW:
            OLED_AppUpdateYawPageData();
            break;

        case OLED_PAGE_VELOCITY:
            OLED_AppUpdateVelocityPageData();
            break;

        case OLED_PAGE_DISTANCE:
            OLED_AppUpdateDistancePageData();
            break;

        case OLED_PAGE_MENU:
        default:
            break;
    }
}

uint8_t OLED_AppIsAutoRunEnabled(void)
{
    return g_auto_run_enabled;
}
