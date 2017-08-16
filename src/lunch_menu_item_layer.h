//
// Created by Cory Lucas on 8/8/17.
//

#include <pebble.h>

#ifndef PEBBLEKABBAGELUNCH_LUNCH_MENU_ITEM_LAYER_H
#define PEBBLEKABBAGELUNCH_LUNCH_MENU_ITEM_LAYER_H

struct LunchMenuItemLayer;
typedef struct LunchMenuItemLayer LunchMenuItemLayer;

// Initialize shared resources. Must be called before using any LunchMenuItemLayers.
void lunch_menu_item_layer_initialize();

// Release shared resources.
void lunch_menu_item_layer_deinitialize();

LunchMenuItemLayer *lunch_menu_item_layer_create(GRect frame);

void lunch_menu_item_layer_destroy(LunchMenuItemLayer *lunch_menu_item_layer);

Layer *lunch_menu_item_layer_get_layer(LunchMenuItemLayer *lunch_menu_item_layer);

time_t lunch_menu_item_layer_get_date(LunchMenuItemLayer *lunch_menu_item_layer);

void lunch_menu_item_layer_set_date(LunchMenuItemLayer *lunch_menu_item_layer, time_t date);

void lunch_menu_item_layer_set_menu(LunchMenuItemLayer *lunch_menu_item_layer, char *menu_text, uint16_t menu_length);

#endif //PEBBLEKABBAGELUNCH_LUNCH_MENU_ITEM_LAYER_H
