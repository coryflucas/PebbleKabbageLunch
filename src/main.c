#include <pebble.h>

static Window *my_window;
static Layer *wrapper_layers[2];
static TextLayer *menu_text_layers[2];
static TextLayer *date_text_layers[2];
static char date_texts[2][12];
static char menu_texts[2][500];

static int active_layer_index = 0;
static time_t selected_date;

const uint32_t inbox_size = 512;
const uint32_t outbox_size = 32;

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

static char *get_date_string(time_t date) {
    static char s_buffer[11];
    struct tm *time = gmtime(&date);
    strftime(s_buffer, sizeof(s_buffer), "%F", time);
    return &s_buffer[0];
}

static int get_active_layer_index() {
    return active_layer_index;
}

static int get_inactive_layer_index() {
    return (active_layer_index + 1) % 2;
}

static void toggle_active_layer() {
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
}

void set_selected_date(time_t date) {
    selected_date = date;

    int i = get_active_layer_index();
    struct tm *time = gmtime(&date);
    strftime(date_texts[i], sizeof(date_texts[i]), "%a, %b %e", time);
    text_layer_set_text(date_text_layers[get_active_layer_index()], date_texts[i]);
}

void request_lunch(time_t date) {
    toggle_active_layer();
    set_selected_date(date);
    text_layer_set_text(menu_text_layers[get_active_layer_index()], "Loading...");

    DictionaryIterator *out_iter;
    AppMessageResult result = app_message_outbox_begin(&out_iter);
    if (result == APP_MSG_OK) {
        dict_write_cstring(out_iter, MESSAGE_KEY_LUNCH_DATE, get_date_string(date));
        result = app_message_outbox_send();
        if (result != APP_MSG_OK) {
            APP_LOG(APP_LOG_LEVEL_ERROR, "Error sending the outbox: %d", (int) result);
        }
    } else {
        APP_LOG(APP_LOG_LEVEL_ERROR, "Error preparing the outbox: %d", (int) result);
    }
}

static void inbox_received_handler(DictionaryIterator *iter, void *context) {
    Tuple *ready = dict_find(iter, MESSAGE_KEY_JS_READY);
    Tuple *menu = dict_find(iter, MESSAGE_KEY_LUNCH_MENU);
    if (ready) {
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Phone reports JS is ready");
        request_lunch(get_next_week_day(time_start_of_today()));
        return;
    } else if (menu) {
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Got lunch menu: %s", menu->value->cstring);
        int i = get_active_layer_index();
        strncpy(menu_texts[i], menu->value->cstring, sizeof(menu_texts[1]));
        text_layer_set_text(menu_text_layers[i], menu_texts[i]);
    }
}

static void inbox_dropped_handler(AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped. Reason: %d", (int) reason);
}

static void outbox_failed_handler(DictionaryIterator *iter, AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Message send failed. Reason: %d", (int) reason);
}

static void outbox_sent_handler(DictionaryIterator *iter, void *context) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Message sent, size: %d", iter->cursor->length);
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
    request_lunch(get_prev_week_day(selected_date - SECONDS_PER_DAY));
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
    request_lunch(get_next_week_day(selected_date + SECONDS_PER_DAY));
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
    app_message_register_outbox_sent(outbox_sent_handler);

    my_window = window_create();
    window_set_click_config_provider(my_window, click_config_provider);
    Layer *window_layer = window_get_root_layer(my_window);
    GRect bounds = layer_get_bounds(window_layer);

    BitmapLayer *bitmap_layer = bitmap_layer_create(bounds);
    bitmap_layer_set_background_color(bitmap_layer, GColorPurple);
    layer_add_child(window_layer, bitmap_layer_get_layer(bitmap_layer));

    for(int i = 0; i < 2; i++) {
        Layer *wrapper_layer = layer_create(bounds);
        layer_add_child(window_layer, wrapper_layer);
        wrapper_layers[i] = wrapper_layer;

        TextLayer *menu_text_layer = text_layer_create(GRect(0, 30, bounds.size.w, bounds.size.h - 30));
        text_layer_set_font(menu_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
        text_layer_set_text_alignment(menu_text_layer, GTextAlignmentCenter);
        layer_add_child(wrapper_layer, text_layer_get_layer(menu_text_layer));
        menu_text_layers[i] = menu_text_layer;

        TextLayer *date_text_layer = text_layer_create(GRect(0, 0, bounds.size.w, 30));
        text_layer_set_font(date_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
        text_layer_set_text_alignment(date_text_layer, GTextAlignmentCenter);
        layer_add_child(wrapper_layer, text_layer_get_layer(date_text_layer));
        date_text_layers[i] = date_text_layer;
    }

    text_layer_set_text(menu_text_layers[get_active_layer_index()], "Loading...");

    APP_LOG(APP_LOG_LEVEL_DEBUG, "active index: %d, inactive index: %d", get_active_layer_index(), get_inactive_layer_index());

    GRect off_screen_bounds = GRect(bounds.origin.x, bounds.origin.y - bounds.size.h, bounds.size.w, bounds.size.h);
    layer_set_bounds(wrapper_layers[get_inactive_layer_index()], off_screen_bounds);

    window_stack_push(my_window, true);
}

void handle_deinit(void) {
    for(int i = 0; i < 2; i++) {
        text_layer_destroy(date_text_layers[i]);
        text_layer_destroy(menu_text_layers[i]);
        layer_destroy(wrapper_layers[i]);
    }
    window_destroy(my_window);
}

int main(void) {
    handle_init();
    app_event_loop();
    handle_deinit();
}
