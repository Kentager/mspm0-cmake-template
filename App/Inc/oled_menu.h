#ifndef _OLED_APP_H
#define _OLED_APP_H

#include <stdint.h>
#include "key.h"

typedef void (*MenuAction_Func)(void *data);

typedef struct MenuItem {
    const char *title;           
    int16_t parent_index;        
    int16_t child_index;         
    int16_t sibling_next_index;

    MenuAction_Func on_enter;

    void *user_data;
} MenuItem_t;

typedef struct {
    MenuItem_t *items;
    uint16_t total_count;
    int16_t root_index;
    int16_t current_index;
    int16_t current_parent;
    int16_t view_start_index;
    uint8_t skip_render_once;
} MenuManager_t;

void Menu_Init(MenuManager_t *mgr , MenuItem_t *items, uint16_t count);
void Menu_Render(MenuManager_t *mgr);
void Menu_HandleKey(MenuManager_t *mgr, key_e key);
void Menu_ReturnToRoot(MenuManager_t *mgr);

#endif
