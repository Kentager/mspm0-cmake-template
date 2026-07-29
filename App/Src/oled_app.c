#include "oled_app.h"
#include "key.h"
#include "oled.h"
#include "oled_menu.h"
#include "motor_app.h"
#include "task.h"
#include <stdio.h>

QueueHandle_t xJobQueue = NULL;

#define TIMER_DIGIT_WIDTH           18U
#define TIMER_DIGIT_HEIGHT          32U
#define TIMER_DIGIT_PAGES           (TIMER_DIGIT_HEIGHT / 8U)
#define TIMER_SEGMENT_THICKNESS     3U

#define TIMER_SEG_A  (1U << 0)
#define TIMER_SEG_B  (1U << 1)
#define TIMER_SEG_C  (1U << 2)
#define TIMER_SEG_D  (1U << 3)
#define TIMER_SEG_E  (1U << 4)
#define TIMER_SEG_F  (1U << 5)
#define TIMER_SEG_G  (1U << 6)

typedef enum {
    OLED_PAGE_MENU = 0,
    OLED_PAGE_YAW,
    OLED_PAGE_VELOCITY,
    OLED_PAGE_DISTANCE,
    OLED_PAGE_GRAYSCALE,
    OLED_PAGE_JOB_TIMER,
} OLED_Page_t;

static MenuManager_t g_menu_mgr;
static volatile uint8_t g_auto_run_enabled = 1U;
static float g_pitch;
static float g_roll;
static float g_yaw;
static uint16_t g_sensor_values[8];
static OLED_Page_t g_current_page = OLED_PAGE_MENU;
static TickType_t g_job_timer_start_tick;
static uint32_t g_job_timer_last_second;
static uint8_t g_job_timer_paused;
static volatile uint8_t g_job_timer_pause_requested;

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
static void OLED_AppDrawTimerPageFrame(Job_e job);
static void OLED_AppDrawTimer(uint32_t elapsed_seconds);
static void OLED_AppShowTimerPaused(void);
static void OLED_AppExitTimer(void);


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

static void OLED_AppTimerSetPixel(uint8_t *bitmap, uint8_t x, uint8_t y)
{
    if ((x >= TIMER_DIGIT_WIDTH) || (y >= TIMER_DIGIT_HEIGHT)) {
        return;
    }

    bitmap[(y / 8U) * TIMER_DIGIT_WIDTH + x] |= (uint8_t)(1U << (y % 8U));
}

static void OLED_AppTimerFillRect(uint8_t *bitmap, uint8_t x, uint8_t y,
                                  uint8_t width, uint8_t height)
{
    uint8_t row;
    uint8_t column;

    for (row = y; row < (uint8_t)(y + height); row++) {
        for (column = x; column < (uint8_t)(x + width); column++) {
            OLED_AppTimerSetPixel(bitmap, column, row);
        }
    }
}

static void OLED_AppDrawTimerDigit(uint8_t x, uint8_t digit)
{
    static const uint8_t segment_map[10] = {
        TIMER_SEG_A | TIMER_SEG_B | TIMER_SEG_C | TIMER_SEG_D | TIMER_SEG_E | TIMER_SEG_F,
        TIMER_SEG_B | TIMER_SEG_C,
        TIMER_SEG_A | TIMER_SEG_B | TIMER_SEG_D | TIMER_SEG_E | TIMER_SEG_G,
        TIMER_SEG_A | TIMER_SEG_B | TIMER_SEG_C | TIMER_SEG_D | TIMER_SEG_G,
        TIMER_SEG_B | TIMER_SEG_C | TIMER_SEG_F | TIMER_SEG_G,
        TIMER_SEG_A | TIMER_SEG_C | TIMER_SEG_D | TIMER_SEG_F | TIMER_SEG_G,
        TIMER_SEG_A | TIMER_SEG_C | TIMER_SEG_D | TIMER_SEG_E | TIMER_SEG_F | TIMER_SEG_G,
        TIMER_SEG_A | TIMER_SEG_B | TIMER_SEG_C,
        TIMER_SEG_A | TIMER_SEG_B | TIMER_SEG_C | TIMER_SEG_D | TIMER_SEG_E | TIMER_SEG_F | TIMER_SEG_G,
        TIMER_SEG_A | TIMER_SEG_B | TIMER_SEG_C | TIMER_SEG_D | TIMER_SEG_F | TIMER_SEG_G,
    };
    uint8_t bitmap[TIMER_DIGIT_WIDTH * TIMER_DIGIT_PAGES];
    uint8_t segments;
    uint8_t page;
    uint8_t column;

    for (column = 0; column < sizeof(bitmap); column++) {
        bitmap[column] = 0U;
    }

    segments = segment_map[digit % 10U];
    if ((segments & TIMER_SEG_A) != 0U) {
        OLED_AppTimerFillRect(bitmap, 3U, 0U, 12U, TIMER_SEGMENT_THICKNESS);
    }
    if ((segments & TIMER_SEG_B) != 0U) {
        OLED_AppTimerFillRect(bitmap, 15U, 3U, TIMER_SEGMENT_THICKNESS, 12U);
    }
    if ((segments & TIMER_SEG_C) != 0U) {
        OLED_AppTimerFillRect(bitmap, 15U, 17U, TIMER_SEGMENT_THICKNESS, 12U);
    }
    if ((segments & TIMER_SEG_D) != 0U) {
        OLED_AppTimerFillRect(bitmap, 3U, 29U, 12U, TIMER_SEGMENT_THICKNESS);
    }
    if ((segments & TIMER_SEG_E) != 0U) {
        OLED_AppTimerFillRect(bitmap, 0U, 17U, TIMER_SEGMENT_THICKNESS, 12U);
    }
    if ((segments & TIMER_SEG_F) != 0U) {
        OLED_AppTimerFillRect(bitmap, 0U, 3U, TIMER_SEGMENT_THICKNESS, 12U);
    }
    if ((segments & TIMER_SEG_G) != 0U) {
        OLED_AppTimerFillRect(bitmap, 3U, 14U, 12U, TIMER_SEGMENT_THICKNESS);
    }

    for (page = 0; page < TIMER_DIGIT_PAGES; page++) {
        OLED_SetPos(x, (uint8_t)(2U + page));
        for (column = 0; column < TIMER_DIGIT_WIDTH; column++) {
            WriteData(bitmap[page * TIMER_DIGIT_WIDTH + column]);
        }
    }
}

static void OLED_AppDrawTimerColon(void)
{
    uint8_t page;
    uint8_t column;
    uint8_t data;

    for (page = 0; page < TIMER_DIGIT_PAGES; page++) {
        data = ((page == 1U) || (page == 2U)) ? 0x18U : 0x00U;
        OLED_SetPos(58U, (uint8_t)(2U + page));
        for (column = 0; column < 4U; column++) {
            WriteData(data);
        }
    }
}

static void OLED_AppDrawTimerPageFrame(Job_e job)
{
    char title[] = "JOB 0 TIMER";

    title[4] = (char)('0' + (uint8_t)job);
    OLED_CLS();
    OLED_ShowString(1, 5, title, 1);
    OLED_ShowString(8, 4, "ANY KEY STOP", 1);
    OLED_AppDrawTimerColon();
}

static void OLED_AppDrawTimer(uint32_t elapsed_seconds)
{
    uint8_t minutes = (uint8_t)((elapsed_seconds / 60U) % 100U);
    uint8_t seconds = (uint8_t)(elapsed_seconds % 60U);

    OLED_AppDrawTimerDigit(9U, minutes / 10U);
    OLED_AppDrawTimerDigit(31U, minutes % 10U);
    OLED_AppDrawTimerDigit(68U, seconds / 10U);
    OLED_AppDrawTimerDigit(90U, seconds % 10U);
}

static void OLED_AppShowTimerPaused(void)
{
    OLED_ShowString(1, 5, "   PAUSED  ", 1);
}

static void OLED_AppExitTimer(void)
{
    Job_e stop_job = Job_Stop;

    if (xQueueSend(xJobQueue, &stop_job, pdMS_TO_TICKS(200U)) != pdPASS) {
        return;
    }

    g_job_timer_paused = 0U;
    g_job_timer_pause_requested = 0U;
    g_current_page = OLED_PAGE_MENU;
    Menu_ReturnToRoot(&g_menu_mgr);
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
    if (xQueueSend(xJobQueue, &job, 0) != pdPASS) {
        return;
    }

    g_current_page = OLED_PAGE_JOB_TIMER;
    g_menu_mgr.skip_render_once = 1U;
    g_job_timer_last_second = 0U;
    g_job_timer_paused = 0U;
    g_job_timer_pause_requested = 0U;
    g_job_timer_start_tick = xTaskGetTickCount();
    OLED_AppDrawTimerPageFrame(job);
    OLED_AppDrawTimer(0U);
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
    if (g_current_page == OLED_PAGE_JOB_TIMER) {
        OLED_AppExitTimer();
        return;
    }

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

void OLED_AppPauseTimer(void)
{
    g_job_timer_pause_requested = 1U;
}

void OLED_AppRefresh(void)
{
    uint32_t elapsed_seconds;

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

        case OLED_PAGE_JOB_TIMER:
            if ((g_job_timer_pause_requested != 0U) &&
                (g_job_timer_paused == 0U)) {
                g_job_timer_paused = 1U;
                OLED_AppShowTimerPaused();
            }

            if (g_job_timer_paused != 0U) {
                break;
            }

            elapsed_seconds = (uint32_t)((xTaskGetTickCount() - g_job_timer_start_tick) /
                                         pdMS_TO_TICKS(1000U));
            if (elapsed_seconds != g_job_timer_last_second) {
                g_job_timer_last_second = elapsed_seconds;
                OLED_AppDrawTimer(g_job_timer_last_second);
            }
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
