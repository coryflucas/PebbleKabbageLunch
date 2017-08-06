#include <pebble.h>

static Window *my_window;
static TextLayer *text_layer;
static time_t selected_date;

const uint32_t inbox_size = 512;
const uint32_t outbox_size = 32;

time_t get_next_week_day(time_t date) {
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

time_t get_prev_week_day(time_t date) {
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

void request_lunch(time_t date) {
    selected_date = date;
    text_layer_set_text(text_layer, "Loading...");

    char s_buffer[11];
    struct tm *time = gmtime(&date);
    strftime(s_buffer, sizeof(s_buffer), "%F", time);

    DictionaryIterator *out_iter;
    AppMessageResult result = app_message_outbox_begin(&out_iter);
    if (result == APP_MSG_OK) {
        dict_write_cstring(out_iter, MESSAGE_KEY_LUNCH_DATE, &s_buffer[0]);
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
        text_layer_set_text(text_layer, menu->value->cstring);
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

    text_layer = text_layer_create(GRect(0, 30, bounds.size.w, 114));
    text_layer_set_font(text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
    layer_add_child(window_layer, text_layer_get_layer(text_layer));
    text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);
    text_layer_set_text(text_layer, "Loading...");

    window_stack_push(my_window, true);
}

void handle_deinit(void) {
    text_layer_destroy(text_layer);
    window_destroy(my_window);
}

int main(void) {
    handle_init();
    app_event_loop();
    handle_deinit();
}
