#include "oled_menu.h"
#include "oled.h"

#define MENU_INVALID_INDEX   (-1)
#define MENU_MAX_SHOW_ITEMS  6
#define MENU_HEADER_LINE     1
#define MENU_FIRST_ITEM_LINE 2

static int16_t Menu_GetParent(MenuManager_t *mgr, int16_t index)
{
    if ((mgr == 0) || (index < 0) || (index >= mgr->total_count)) {
        return MENU_INVALID_INDEX;
    }

    return mgr->items[index].parent_index;
}

static int16_t Menu_GetFirstChild(MenuManager_t *mgr, int16_t index)
{
    if ((mgr == 0) || (index < 0) || (index >= mgr->total_count)) {
        return MENU_INVALID_INDEX;
    }

    return mgr->items[index].child_index;
}

static int16_t Menu_GetNextSibling(MenuManager_t *mgr, int16_t index)
{
    if ((mgr == 0) || (index < 0) || (index >= mgr->total_count)) {
        return MENU_INVALID_INDEX;
    }

    return mgr->items[index].sibling_next_index;
}

static int16_t Menu_GetPrevSibling(MenuManager_t *mgr, int16_t index)
{
    int16_t parent;
    int16_t node;

    if ((mgr == 0) || (index < 0) || (index >= mgr->total_count)) {
        return MENU_INVALID_INDEX;
    }

    parent = Menu_GetParent(mgr, index);
    if (parent < 0) {
        return index;
    }

    node = Menu_GetFirstChild(mgr, parent);
    if (node == index) {
        return index;
    }

    while ((node != MENU_INVALID_INDEX) &&
           (Menu_GetNextSibling(mgr, node) != index)) {
        node = Menu_GetNextSibling(mgr, node);
    }

    return (node == MENU_INVALID_INDEX) ? index : node;
}

static uint8_t Menu_IsSiblingOfCurrentParent(MenuManager_t *mgr, int16_t index)
{
    int16_t node;

    if ((mgr == 0) || (index < 0) || (index >= mgr->total_count)) {
        return 0;
    }

    node = Menu_GetFirstChild(mgr, mgr->current_parent);
    while (node != MENU_INVALID_INDEX) {
        if (node == index) {
            return 1;
        }
        node = Menu_GetNextSibling(mgr, node);
    }

    return 0;
}

static void Menu_AdjustView(MenuManager_t *mgr)
{
    int16_t node;
    uint8_t count = 0;

    if ((mgr == 0) || (mgr->current_index == MENU_INVALID_INDEX)) {
        return;
    }

    if (!Menu_IsSiblingOfCurrentParent(mgr, mgr->view_start_index)) {
        mgr->view_start_index = Menu_GetFirstChild(mgr, mgr->current_parent);
    }

    node = mgr->view_start_index;
    while ((node != MENU_INVALID_INDEX) && (count < MENU_MAX_SHOW_ITEMS)) {
        if (node == mgr->current_index) {
            return;
        }
        node = Menu_GetNextSibling(mgr, node);
        count++;
    }

    mgr->view_start_index = mgr->current_index;
    for (count = 0; count < (MENU_MAX_SHOW_ITEMS - 1); count++) {
        node = Menu_GetPrevSibling(mgr, mgr->view_start_index);
        if (node == mgr->view_start_index) {
            break;
        }
        mgr->view_start_index = node;
    }
}

static void Menu_EnterCurrent(MenuManager_t *mgr)
{
    int16_t child;

    child = Menu_GetFirstChild(mgr, mgr->current_index);
    if (child != MENU_INVALID_INDEX) {
        mgr->current_parent = mgr->current_index;
        mgr->current_index = child;
        mgr->view_start_index = child;
        return;
    }

    if (mgr->items[mgr->current_index].on_enter != 0) {
        mgr->items[mgr->current_index].on_enter(
            mgr->items[mgr->current_index].user_data
        );
    }
}

void Menu_Init(MenuManager_t *mgr, MenuItem_t *items, uint16_t count)
{
    int16_t first_child;

    if ((mgr == 0) || (items == 0) || (count == 0)) {
        return;
    }

    mgr->items = items;
    mgr->total_count = count;
    mgr->root_index = 0;
    mgr->skip_render_once = 0;

    first_child = Menu_GetFirstChild(mgr, mgr->root_index);
    if (first_child == MENU_INVALID_INDEX) {
        mgr->current_index = mgr->root_index;
        mgr->current_parent = MENU_INVALID_INDEX;
        mgr->view_start_index = mgr->root_index;
        return;
    }

    mgr->current_index = first_child;
    mgr->current_parent = mgr->root_index;
    mgr->view_start_index = first_child;
}

void Menu_ReturnToRoot(MenuManager_t *mgr)
{
    int16_t first_child;

    if ((mgr == 0) || (mgr->items == 0) || (mgr->total_count == 0)) {
        return;
    }

    first_child = Menu_GetFirstChild(mgr, mgr->root_index);
    mgr->current_parent = mgr->root_index;
    mgr->current_index = (first_child == MENU_INVALID_INDEX) ? mgr->root_index : first_child;
    mgr->view_start_index = mgr->current_index;
    mgr->skip_render_once = 0U;
    Menu_Render(mgr);
}

void Menu_Render(MenuManager_t *mgr)
{
    int16_t node;
    uint8_t line = MENU_FIRST_ITEM_LINE;
    const char *header = "MENU";

    if ((mgr == 0) || (mgr->items == 0) || (mgr->total_count == 0)) {
        return;
    }

    if ((mgr->current_parent >= 0) && (mgr->current_parent != mgr->root_index)) {
        header = mgr->items[mgr->current_parent].title;
    }

    Menu_AdjustView(mgr);

    OLED_CLS();
    OLED_ShowString(MENU_HEADER_LINE, 0, (char *)header, 1);

    node = mgr->view_start_index;
    while ((node != MENU_INVALID_INDEX) && (line < (MENU_FIRST_ITEM_LINE + MENU_MAX_SHOW_ITEMS))) {
        OLED_ShowString(line, 0, (node == mgr->current_index) ? ">" : " ", 1);
        OLED_ShowString(line, 2, (char *)mgr->items[node].title, 1);
        node = Menu_GetNextSibling(mgr, node);
        line++;
    }
}

void Menu_HandleKey(MenuManager_t *mgr, key_e key)
{
    int16_t next_index;
    int16_t prev_index;
    int16_t parent;

    if ((mgr == 0) || (mgr->items == 0) || (mgr->total_count == 0)) {
        return;
    }

    if (mgr->skip_render_once != 0) {
        mgr->skip_render_once = 0;
        Menu_Render(mgr);
        return;
    }

    switch (key) {
        case KEY_0:
            prev_index = Menu_GetPrevSibling(mgr, mgr->current_index);
            if (prev_index != MENU_INVALID_INDEX) {
                mgr->current_index = prev_index;
            }
            break;

        case KEY_1:
            next_index = Menu_GetNextSibling(mgr, mgr->current_index);
            if (next_index != MENU_INVALID_INDEX) {
                mgr->current_index = next_index;
            }
            break;

        case KEY_2:
            Menu_EnterCurrent(mgr);
            break;

        case KEY_3:
            parent = Menu_GetParent(mgr, mgr->current_index);
            if ((parent != MENU_INVALID_INDEX) && (parent != mgr->root_index)) {
                mgr->current_index = parent;
                mgr->current_parent = Menu_GetParent(mgr, parent);
                if (mgr->current_parent == MENU_INVALID_INDEX) {
                    mgr->current_parent = mgr->root_index;
                }
                mgr->view_start_index = mgr->current_index;
            }
            break;

        case KEY_NONE:
        default:
            break;
    }

    if (mgr->current_parent == MENU_INVALID_INDEX) {
        mgr->current_parent = mgr->root_index;
    }

    if ((mgr->current_index != MENU_INVALID_INDEX) &&
        (Menu_GetParent(mgr, mgr->current_index) >= 0)) {
        parent = Menu_GetParent(mgr, mgr->current_index);
        if (parent == mgr->root_index) {
            mgr->current_parent = mgr->root_index;
        } else {
            mgr->current_parent = parent;
        }
    }

    if (mgr->skip_render_once != 0) {
        return;
    }

    Menu_Render(mgr);
}
