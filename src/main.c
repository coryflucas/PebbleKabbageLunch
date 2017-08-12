#include <pebble.h>
#include "lunch_menu_item_layer.h"

const uint32_t inbox_size = 512;
const uint32_t outbox_size = 32;

typedef enum {
    AnimateDirectionNone,
    AnimateDirectionForward,
    AnimateDirectionBack
} AnimateDirection;

static Window *window;
static LunchMenuItemLayer *lunch_menu_item_layer;

static time_t get_next_week_day(time_t date) {
    tm *time = gmtime(&date);
    time_t result = date;
    switch (time->tm_wday) {
        case 0:
            result = date + SECONDS_PER_DAY;
            break;
        case 6:
            result = date + SECONDS_PER_DAY * 2;
            break;
    }
    return result;
}

static time_t get_prev_week_day(time_t date) {
    tm *time = gmtime(&date);
    time_t result = date;
    switch (time->tm_wday) {
        case 0:
            result = date - SECONDS_PER_DAY * 2;
            break;
        case 6:
            result = date - SECONDS_PER_DAY;
            break;
    }
    return result;
}

static char *get_request_date_string(time_t date) {
    static char s_buffer[11];
    strftime(s_buffer, sizeof(s_buffer), "%F", gmtime(&date));
    return &s_buffer[0];
}

static void new_menu_item_anim_started_handler(Animation *animation, void *context) {
    // once the animation starts, add to the window. Adding before would make it flash on top of the old menu item
    LunchMenuItemLayer *new_lunch_menu_item_layer = context;
    layer_add_child(window_get_root_layer(window), lunch_menu_item_layer_get_layer(new_lunch_menu_item_layer));
}

static void old_menu_item_anim_stopped_handler(Animation *animation, bool finished, void *context) {
    // after animation complete, destroy old menu item
    if(finished) {
        LunchMenuItemLayer *old_lunch_menu_item_layer = context;
        layer_remove_from_parent(lunch_menu_item_layer_get_layer(old_lunch_menu_item_layer));
        lunch_menu_item_layer_destroy(old_lunch_menu_item_layer);
    }
}

static void request_lunch(time_t date, AnimateDirection direction) {
    LunchMenuItemLayer *old_lunch_menu_item_layer = lunch_menu_item_layer;

    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    LunchMenuItemLayer *new_lunch_menu_item_layer = lunch_menu_item_layer_create(bounds);
    lunch_menu_item_layer_set_date(new_lunch_menu_item_layer, date);
    lunch_menu_item_layer_set_menu(new_lunch_menu_item_layer, "loading", 8);
    lunch_menu_item_layer = new_lunch_menu_item_layer;

    if (direction != AnimateDirectionNone) {
        int slide_in_offset = direction == AnimateDirectionForward ? bounds.size.h : -bounds.size.h;

        // animate new item slide in
        GPoint end = bounds.origin;
        GPoint start = GPoint(bounds.origin.x, bounds.origin.y + slide_in_offset);
        PropertyAnimation *new_prop_animation = property_animation_create_bounds_origin(
                lunch_menu_item_layer_get_layer(new_lunch_menu_item_layer),
                &start,
                &end);
        Animation *new_animation = property_animation_get_animation(new_prop_animation);
        animation_set_duration(new_animation, 250);
        animation_set_handlers(new_animation, (AnimationHandlers) {
                .started = new_menu_item_anim_started_handler
        }, new_lunch_menu_item_layer);

        // animate old item slide out
        start = bounds.origin;
        end = GPoint(bounds.origin.x, bounds.origin.y - slide_in_offset);
        PropertyAnimation *old_prop_animation = property_animation_create_bounds_origin(
                lunch_menu_item_layer_get_layer(old_lunch_menu_item_layer),
                &start,
                &end);
        Animation *old_animation = property_animation_get_animation(old_prop_animation);
        animation_set_duration(old_animation, 250);
        animation_set_handlers(old_animation, (AnimationHandlers) {
                .stopped = old_menu_item_anim_stopped_handler
        }, old_lunch_menu_item_layer);

        animation_schedule(animation_spawn_create(new_animation, old_animation, NULL));
    } else {
        // this is handled in animation callbacks when animated
        layer_add_child(window_get_root_layer(window), lunch_menu_item_layer_get_layer(new_lunch_menu_item_layer));
        if (old_lunch_menu_item_layer) {
            layer_remove_from_parent(lunch_menu_item_layer_get_layer(old_lunch_menu_item_layer));
            lunch_menu_item_layer_destroy(old_lunch_menu_item_layer);
        }
    }

    DictionaryIterator *out_iter;
    AppMessageResult result = app_message_outbox_begin(&out_iter);
    if (result == APP_MSG_OK) {
        dict_write_cstring(out_iter, MESSAGE_KEY_LUNCH_DATE, get_request_date_string(date));
        result = app_message_outbox_send();
        if (result != APP_MSG_OK) {
            APP_LOG(APP_LOG_LEVEL_ERROR, "Error sending the outbox: %d", (int) result);
        }
    } else {
        APP_LOG(APP_LOG_LEVEL_ERROR, "Error preparing the outbox: %d", (int) result);
    }
}

static void inbox_received_handler(DictionaryIterator *iter, void *context) {
    Tuple *ready_tuple = dict_find(iter, MESSAGE_KEY_JS_READY);
    Tuple *menu_tuple = dict_find(iter, MESSAGE_KEY_LUNCH_MENU);
    if (ready_tuple) {
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Phone reports JS is ready");
        request_lunch(get_next_week_day(time_start_of_today()), AnimateDirectionNone);
        return;
    } else if (menu_tuple) {
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Got lunch menu: %s", menu_tuple->value->cstring);
        lunch_menu_item_layer_set_menu(lunch_menu_item_layer, menu_tuple->value->cstring, menu_tuple->length);
    }
}

static void inbox_dropped_handler(AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped. Reason: %d", (int) reason);
}

static void outbox_failed_handler(DictionaryIterator *iter, AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Message send failed. Reason: %d", (int) reason);
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
    time_t current_date = lunch_menu_item_layer_get_date(lunch_menu_item_layer);
    request_lunch(get_prev_week_day(current_date - SECONDS_PER_DAY), AnimateDirectionBack);
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
    time_t current_date = lunch_menu_item_layer_get_date(lunch_menu_item_layer);
    request_lunch(get_next_week_day(current_date + SECONDS_PER_DAY), AnimateDirectionForward);
}

static void click_config_provider(void *context) {
    window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
    window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}

void handle_init(void) {
    app_message_open(inbox_size, outbox_size);
    app_message_register_inbox_received(inbox_received_handler);
    app_message_register_inbox_dropped(inbox_dropped_handler);
    app_message_register_outbox_failed(outbox_failed_handler);

    lunch_menu_item_layer_initialize();

    window = window_create();
    window_set_click_config_provider(window, click_config_provider);

    window_stack_push(window, true);
}

void handle_deinit(void) {
    lunch_menu_item_layer_destroy(lunch_menu_item_layer);
    window_destroy(window);
    lunch_menu_item_layer_deinitialize();
}

int main(void) {
    handle_init();
    app_event_loop();
    handle_deinit();
}
