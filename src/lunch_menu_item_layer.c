//
// Created by Cory Lucas on 8/8/17.
//

#include "lunch_menu_item_layer.h"

#define DATE_TEXT_LENGTH 12

struct LunchMenuItemLayer {
    Layer *wrapper_layer;
    TextLayer *date_text_layer;
    TextLayer *menu_text_layer;
    time_t date;
    char *date_text;
    char *menu_text;
};

#ifdef PBL_COLOR
static GBitmap *background_bitmap;
#endif

void lunch_menu_item_layer_initialize() {
#ifdef PBL_COLOR
    background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_BACKGROUND_LOGO);
#endif
}

void lunch_menu_item_layer_deinitialize() {
#ifdef PBL_COLOR
    gbitmap_destroy(background_bitmap);
#endif
}

static void update_date_text(LunchMenuItemLayer *lunch_menu_item_layer) {
    strftime(lunch_menu_item_layer->date_text, DATE_TEXT_LENGTH, "%a, %b %e", gmtime(&lunch_menu_item_layer->date));
}

static void wrapper_update_proc(Layer *layer, GContext *ctx) {
    GRect bounds = layer_get_bounds(layer);

#ifdef PBL_COLOR
    graphics_context_set_fill_color(ctx, GColorDarkGreen);
    graphics_context_set_stroke_color(ctx, GColorWhite);

    bounds.origin = GPoint(0, 0);
    graphics_fill_rect(ctx, bounds, 0, GCornerNone);
    graphics_context_set_compositing_mode(ctx, GCompOpSet);
    GRect bitmap_bounds = gbitmap_get_bounds(background_bitmap);
    bitmap_bounds.origin.x = bounds.size.w - bitmap_bounds.size.w - 2;
    bitmap_bounds.origin.y = bounds.size.h - bitmap_bounds.size.h - 2;
    graphics_draw_bitmap_in_rect(ctx, background_bitmap, bitmap_bounds);
#endif
    graphics_draw_line(ctx, GPoint(4, 23), GPoint(bounds.size.w - 4, 23));
}

LunchMenuItemLayer *lunch_menu_item_layer_create(GRect frame) {
    LunchMenuItemLayer *self = calloc(1, sizeof(LunchMenuItemLayer));

    self->date_text = calloc(DATE_TEXT_LENGTH, sizeof(char));

    self->wrapper_layer = layer_create(frame);
    layer_set_update_proc(self->wrapper_layer, wrapper_update_proc);

    GRect date_text_layer_bounds = frame;
    date_text_layer_bounds.size.h = 22;
    self->date_text_layer = text_layer_create(date_text_layer_bounds);
    text_layer_set_font(self->date_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
    text_layer_set_text_alignment(self->date_text_layer, GTextAlignmentCenter);
    text_layer_set_text(self->date_text_layer, self->date_text);
    text_layer_set_background_color(self->date_text_layer, GColorClear);
#ifdef PBL_COLOR
    text_layer_set_text_color(self->date_text_layer, GColorWhite);
#endif
    layer_add_child(self->wrapper_layer, text_layer_get_layer(self->date_text_layer));

    GRect menu_text_layer_bounds = frame;
    menu_text_layer_bounds.origin.x = 4;
    menu_text_layer_bounds.origin.y = 24;
    menu_text_layer_bounds.size.w -= 8;
    menu_text_layer_bounds.size.h -= 28;
    self->menu_text_layer = text_layer_create(menu_text_layer_bounds);
    text_layer_set_font(self->menu_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
    text_layer_set_background_color(self->menu_text_layer, GColorClear);
#ifdef PBL_COLOR
    text_layer_set_text_color(self->menu_text_layer, GColorWhite);
#endif
    layer_add_child(self->wrapper_layer, text_layer_get_layer(self->menu_text_layer));

    return self;
}

void lunch_menu_item_layer_destroy(LunchMenuItemLayer *lunch_menu_item_layer) {
    text_layer_destroy(lunch_menu_item_layer->menu_text_layer);
    text_layer_destroy(lunch_menu_item_layer->date_text_layer);
    layer_destroy(lunch_menu_item_layer->wrapper_layer);
    if (lunch_menu_item_layer->date_text) {
        free(lunch_menu_item_layer->date_text);
    }
    if (lunch_menu_item_layer->menu_text) {
        free(lunch_menu_item_layer->menu_text);
    }
    free(lunch_menu_item_layer);
}

Layer *lunch_menu_item_layer_get_layer(LunchMenuItemLayer *lunch_menu_item_layer) {
    return lunch_menu_item_layer->wrapper_layer;
}

time_t lunch_menu_item_layer_get_date(LunchMenuItemLayer *lunch_menu_item_layer) {
    return lunch_menu_item_layer->date;
}

void lunch_menu_item_layer_set_date(LunchMenuItemLayer *lunch_menu_item_layer, time_t date) {
    lunch_menu_item_layer->date = date;
    update_date_text(lunch_menu_item_layer);
    layer_mark_dirty(text_layer_get_layer(lunch_menu_item_layer->date_text_layer));
}

const char *lunch_menu_item_layer_get_menu(LunchMenuItemLayer *lunch_menu_item_layer) {
    return lunch_menu_item_layer->menu_text;
}

void lunch_menu_item_layer_set_menu(LunchMenuItemLayer *lunch_menu_item_layer, char *menu_text, uint16_t menu_length) {
    char *old_menu_text = lunch_menu_item_layer->menu_text;
    lunch_menu_item_layer->menu_text = malloc(menu_length);
    strncpy(lunch_menu_item_layer->menu_text, menu_text, menu_length);
    text_layer_set_text(lunch_menu_item_layer->menu_text_layer, lunch_menu_item_layer->menu_text);
    if (old_menu_text) {
        free(old_menu_text);
    }
}

void lunch_menu_item_layer_mark_dirty(LunchMenuItemLayer *lunch_menu_item_layer) {
    update_date_text(lunch_menu_item_layer);
    layer_mark_dirty(text_layer_get_layer(lunch_menu_item_layer->date_text_layer));
    layer_mark_dirty(text_layer_get_layer(lunch_menu_item_layer->menu_text_layer));
}