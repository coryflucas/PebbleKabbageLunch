#include <pebble.h>

Window *my_window;
TextLayer *text_layer;

void handle_init(void) {
  my_window = window_create();
  Layer *window_layer = window_get_root_layer(my_window);
  text_layer = text_layer_create(GRect(0, 0, 144, 144));
  text_layer_set_text(text_layer, "Kabbage lunches are now on your timeline.");
  layer_add_child(window_layer, text_layer_get_layer(text_layer));
  
  #if PBL_ROUND
  text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);
  text_layer_enable_screen_text_flow_and_paging(text_layer, 8);
  #endif
  
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
