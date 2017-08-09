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

/*static void toggle_active_layer() {
    Layer *layer = wrapper_layers[get_active_layer_index()];
    GRect original_bounds = layer_get_bounds(layer);
    GPoint original_point = original_bounds.origin;

    GPoint off_screen_bottom = GPoint(original_bounds.origin.x, original_bounds.origin.y + original_bounds.size.h);
    PropertyAnimation *prop_anim_a = property_animation_create_bounds_origin(layer, NULL, &off_screen_bottom);
    Animation *animation_a = property_animation_get_animation(prop_anim_a);
    animation_set_duration(animation_a, 200);

    layer = wrapper_layers[get_inactive_layer_index()];
    GPoint off_screen_top = GPoint(original_bounds.origin.x, original_bounds.origin.y - original_bounds.size.h);
    PropertyAnimation *prop_anim_b = property_animation_create_bounds_origin(layer, &off_screen_top, &original_point);
    Animation *animation_b = property_animation_get_animation(prop_anim_b);
    animation_set_duration(animation_b, 200);

    Animation *spawn = animation_spawn_create(animation_a, animation_b, NULL);
    animation_schedule(spawn);

    active_layer_index = get_inactive_layer_index();
}*/

static char *get_request_date_string(time_t date) {
    static char s_buffer[11];
    //struct tm *time = gmtime(&date);
    strftime(s_buffer, sizeof(s_buffer), "%F", gmtime(&date));
    return &s_buffer[0];
}

static void request_lunch(time_t date, AnimateDirection direction) {
    lunch_menu_item_layer_set_date(lunch_menu_item_layer, date);
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
        //current_menu_item->menu = malloc(menu_tuple->length);
        //strncpy(current_menu_item->menu, menu_tuple->value->cstring, menu_tuple->length);
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

    window = window_create();
    window_set_click_config_provider(window, click_config_provider);
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    lunch_menu_item_layer = lunch_menu_item_layer_create(bounds);
    layer_add_child(window_layer, lunch_menu_item_layer_get_layer(lunch_menu_item_layer));

    window_stack_push(window, true);
}

void handle_deinit(void) {
    lunch_menu_item_layer_destroy(lunch_menu_item_layer);
    window_destroy(window);
}

int main(void) {
    handle_init();
    app_event_loop();
    handle_deinit();
}
