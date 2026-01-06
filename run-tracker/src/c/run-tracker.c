#include <pebble.h>

enum {
  KEY_LATITUDE = 10000,
  KEY_LONGITUDE = 10001,
};

static Window *s_window;
static TextLayer *s_text_layer;
static TextLayer *s_text_time;
static TextLayer *s_text_distance;
static TextLayer *s_text_pace;
static TextLayer *s_text_time_unit;
static TextLayer *s_text_distance_unit;
static TextLayer *s_text_pace_unit;

static void inbox_received_handler(DictionaryIterator *iter, void *context) {
  Tuple *latitude_kvp = dict_find(iter, KEY_LATITUDE);
  Tuple *longitude_kvp = dict_find(iter, KEY_LONGITUDE);

  Tuple *t = dict_read_first(iter);
  while (t) {
    APP_LOG(APP_LOG_LEVEL_INFO,
            "Key %lu, type %d, length %u",
            (unsigned long)t->key,
            t->type,
            t->length);
    t = dict_read_next(iter);
  }

  if (!latitude_kvp || !longitude_kvp) {
    return;
  }

  int32_t intLatitude = latitude_kvp->value->int32;
  int32_t intLongitude = longitude_kvp->value->int32;

  // float latitude = intLatitude * 1e-6f;
  // float longitude = intLongitude * 1e-6f;
  
  /*static char formatted_position[32];
  
  snprintf(formatted_position, sizeof(formatted_position), "GPS: %ld.%06ld, %ld.%06ld",
         intLatitude / 1000000, labs(intLatitude % 1000000),
         intLongitude / 1000000, labs(intLongitude % 1000000));
  text_layer_set_text(s_text_layer, formatted_position);*/
}

static void prv_select_click_handler(ClickRecognizerRef recognizer, void *context) {
  text_layer_set_text(s_text_layer, "Select");
}

static void prv_up_click_handler(ClickRecognizerRef recognizer, void *context) {
  text_layer_set_text(s_text_layer, "Up");
}

static void prv_down_click_handler(ClickRecognizerRef recognizer, void *context) {
  text_layer_set_text(s_text_layer, "Down");
}

static void prv_click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, prv_select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, prv_up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, prv_down_click_handler);
}

static void format_text_layer(TextLayer* layer) {
  text_layer_set_text_alignment(layer, GTextAlignmentRight);
  text_layer_set_font(layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  text_layer_set_text_color(layer, GColorWhite);
  text_layer_set_background_color(layer, GColorBlack);
}

static void format_text_layer_unit(TextLayer* layer) {
  text_layer_set_text_alignment(layer, GTextAlignmentLeft);
  text_layer_set_font(layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
  text_layer_set_text_color(layer, GColorWhite);
  text_layer_set_background_color(layer, GColorBlack);
}

static void format_text_layer_hint(TextLayer* layer) {
  text_layer_set_text_alignment(layer, GTextAlignmentLeft);
  text_layer_set_font(layer, fonts_get_system_font(FONT_KEY_GOTHIC_18));
  text_layer_set_text_color(layer, GColorWhite);
  text_layer_set_background_color(layer, GColorBlack);
}

static void prv_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_text_layer = text_layer_create(GRect(0, 0, bounds.size.w, 20));
  text_layer_set_text(s_text_layer, "00:00:00");
  format_text_layer(s_text_layer);
  text_layer_set_font(s_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text_alignment(s_text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_text_layer));

  int32_t width = 106;
  int32_t row_height = 44;
  int32_t base_height = 18;
  int32_t gap = 6;
  s_text_time = text_layer_create(GRect(0, base_height + row_height * 0, width, row_height));
  text_layer_set_text(s_text_time, "00:00");
  format_text_layer(s_text_time);
  layer_add_child(window_layer, text_layer_get_layer(s_text_time));
  
  s_text_time_unit = text_layer_create(GRect(width + gap, base_height + row_height * 0 + 6, bounds.size.w - width - gap, 20));
  text_layer_set_text(s_text_time_unit, "TIME");
  format_text_layer_hint(s_text_time_unit);
  layer_add_child(window_layer, text_layer_get_layer(s_text_time_unit));

  s_text_distance = text_layer_create(GRect(0, base_height + row_height * 1, width, row_height));
  text_layer_set_text(s_text_distance, "0.00");
  format_text_layer(s_text_distance);
  text_layer_set_font(s_text_distance, fonts_get_system_font(FONT_KEY_BITHAM_42_LIGHT));
  layer_add_child(window_layer, text_layer_get_layer(s_text_distance));

  s_text_distance_unit = text_layer_create(GRect(width + gap, base_height + row_height * 1 + 18, bounds.size.w - width - gap, 40));
  text_layer_set_text(s_text_distance_unit, "km");
  format_text_layer_unit(s_text_distance_unit);
  layer_add_child(window_layer, text_layer_get_layer(s_text_distance_unit));

  s_text_pace = text_layer_create(GRect(0, base_height + row_height * 2, width, row_height));
  text_layer_set_text(s_text_pace, "0:67");
  format_text_layer(s_text_pace);
  layer_add_child(window_layer, text_layer_get_layer(s_text_pace));

  s_text_pace_unit = text_layer_create(GRect(width + gap, base_height + row_height * 2 + 6, bounds.size.w - width - gap, 20));
  text_layer_set_text(s_text_pace_unit, "PACE");
  format_text_layer_hint(s_text_pace_unit);
  layer_add_child(window_layer, text_layer_get_layer(s_text_pace_unit));
}

static void prv_window_unload(Window *window) {
  text_layer_destroy(s_text_layer);
}

static void time_update_handler(struct tm *tick_time, TimeUnits units_changed)
{
    static char formatted_time[10];
    
    snprintf(formatted_time, sizeof(formatted_time), "%d:%02d:%02d", tick_time->tm_hour, tick_time->tm_min, tick_time->tm_sec);
    text_layer_set_text(s_text_layer, formatted_time);
}

static void prv_init(void) {
  s_window = window_create();
  window_set_background_color(s_window, GColorBlack);
  window_set_click_config_provider(s_window, prv_click_config_provider);
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = prv_window_load,
    .unload = prv_window_unload,
  });
  app_message_register_inbox_received(inbox_received_handler);
  app_message_open(128, 128); // inbox, outbox sizes
  const bool animated = true;
  window_stack_push(s_window, animated);
  tick_timer_service_subscribe(SECOND_UNIT, time_update_handler);
}

static void prv_deinit(void) {
  window_destroy(s_window);
}

int main(void) {
  prv_init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", s_window);



  app_event_loop();
  prv_deinit();
}
