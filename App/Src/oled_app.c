#include "oled_app.h"
#include "key.h"
#include "oled.h"
#include "oled_menu.h"
#include "motor_app.h"
#include <stdio.h>

QueueHandle_t xJobQueue = NULL;


typedef enum {
    OLED_PAGE_MENU = 0,
    OLED_PAGE_YAW,
    OLED_PAGE_VELOCITY,
    OLED_PAGE_DISTANCE,
    OLED_PAGE_GRAYSCALE,
} OLED_Page_t;

static MenuManager_t g_menu_mgr;
static volatile uint8_t g_auto_run_enabled = 1U;
static float g_pitch;
static float g_roll;
static float g_yaw;
static uint16_t g_sensor_values[8];
static OLED_Page_t g_current_page = OLED_PAGE_MENU;

static void MenuAction_SendJobQueue(void *data);

static Job_e g_job_0 = Job_0;
static Job_e g_job_1 = Job_1;
static Job_e g_job_2 = Job_2;
static Job_e g_job_3 = Job_3;

static void MenuAction_ShowYaw(void *data);
static void MenuAction_ShowVelocity(void *data);
static void MenuAction_ShowDistance(void *data);
static void MenuAction_ShowGrayscale(void *data);
static void OLED_AppDrawYawPageFrame(void);
static void OLED_AppDrawVelocityPageFrame(void);
static void OLED_AppDrawDistancePageFrame(void);
static void OLED_AppDrawGrayscalePageFrame(void);
static void OLED_AppUpdateYawPageData(void);
static void OLED_AppUpdateVelocityPageData(void);
static void OLED_AppUpdateDistancePageData(void);
static void OLED_AppUpdateGrayscalePageData(void);



static MenuItem_t g_menu_items[] = {
    {"Root", -1, 1, -1, 0, 0},
    {"Action", 0, 3, 2, 0, 0},
    {"Status", 0, 7, -1, 0, 0},

    {"Job_0", 1, -1, 4, MenuAction_SendJobQueue, &g_job_0},
    {"Job_1", 1, -1, 5, MenuAction_SendJobQueue, &g_job_1},
    {"Job_2", 1, -1, 6, MenuAction_SendJobQueue, &g_job_2},
    {"Job_3", 1, -1, -1, MenuAction_SendJobQueue, &g_job_3},

    {"Show Yaw",   2, -1, 8, MenuAction_ShowYaw, 0},
    {"Show Vel",   2, -1, 9, MenuAction_ShowVelocity, 0},
    {"Show Dist",  2, -1, 10, MenuAction_ShowDistance, 0},
    {"Show Gray",  2, -1, -1, MenuAction_ShowGrayscale, 0},

};

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

static void OLED_AppDrawGrayscalePageFrame(void)
{
    Menu_ShowTempPageTitle("Gray Sensor");
    OLED_ShowString(4, 2, "1 2 3 4 5 6 7 8", 1);
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

static void OLED_AppUpdateGrayscalePageData(void)
{
    char buf[17];
    for(uint8_t i = 0; i < 8 ; i ++){
      buf[i * 2] = g_sensor_values[i] ? '#' : '-';
      buf[i * 2 + 1] = ' ';
    }
    buf[16] = '\0';
    OLED_ShowString(6, 2, "                ", 1);
    OLED_ShowString(6, 2, buf, 1);
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

static void MenuAction_ShowGrayscale(void *data)
{
    (void)data;
    g_current_page = OLED_PAGE_GRAYSCALE;
    OLED_AppDrawGrayscalePageFrame();
    OLED_AppUpdateGrayscalePageData();
}

static void MenuAction_SendJobQueue(void *data)
{
    Job_e job;

    if (data == 0) {
        return;
    }

    job = *(Job_e *)data;
    xQueueSend(xJobQueue, &job, 0);
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

void OLED_AppSetSensorValues(uint16_t* sensor_values) {
    for(int i = 0; i < 8; i++) {
      g_sensor_values[i] = sensor_values[i];
    }
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

        case OLED_PAGE_GRAYSCALE:
            OLED_AppUpdateGrayscalePageData();
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
