#include <pebble.h>
#include <math.h>

#define DEG_TO_RAD (3.14159265359 / 180.0f)
#define EARTH_RADIUS_M 6371000.0f

enum {
  KEY_LATITUDE = 10000,
  KEY_LONGITUDE = 10001,
};


enum {
  STATE_WAITING_FOR_GPS = 0,
  STATE_READY = 1,
  STATE_RUNNING = 2,
  STATE_PAUSED = 3
};

static int32_t runState = STATE_WAITING_FOR_GPS;
static uint32_t startTime;
static uint32_t elapsedTime = 0;
static uint32_t distanceMillimeters = 0;
static int16_t statusUpdateTimer = 0;

typedef struct {
  float latitude;
  float longitude;
  uint32_t time;
} Position;

typedef struct {
  uint32_t deltaTime;
  uint32_t distance;
  bool valid;
} SpeedBufferItem;

#define SPEED_BUFFER_SIZE 64
static int32_t speedBufferPosition = 0;
static SpeedBufferItem speedBuffer[SPEED_BUFFER_SIZE];

static Position startPosition;
static Position lastPosition;

static Window *s_window;
static TextLayer *s_text_layer;
static TextLayer *s_text_time;
static TextLayer *s_text_distance;
static TextLayer *s_text_pace;
// static TextLayer *s_text_time_unit;
static TextLayer *s_text_distance_unit;
static TextLayer *s_text_pace_unit;

static void initializeSpeedBuffer() {
  for (uint16_t i = 0; i < SPEED_BUFFER_SIZE; i++) {
    speedBuffer[i].valid = false;
  }
  speedBufferPosition = 0;
}

static void updateSpeedBuffer(uint32_t deltaTime, uint32_t distance) {
  speedBuffer[speedBufferPosition].deltaTime = deltaTime;
  speedBuffer[speedBufferPosition].distance = distance;
  speedBuffer[speedBufferPosition].valid = true;
  speedBufferPosition = (speedBufferPosition + 1) % SPEED_BUFFER_SIZE;
}

static uint32_t getPace() {
  uint32_t totalTime = 0;
  uint32_t totalDistance = 0;
  for (int16_t i = SPEED_BUFFER_SIZE - 1; i >= 0; i--) {
    uint16_t queryIndex = (i + SPEED_BUFFER_SIZE + speedBufferPosition) % SPEED_BUFFER_SIZE;
    if (!(speedBuffer[queryIndex].valid)) {
      break;
    }
    totalTime += speedBuffer[queryIndex].deltaTime;
    totalDistance += speedBuffer[queryIndex].distance;
    if (totalDistance > 1000000) {
      break;
    }
  }
  return 1000 * totalTime / totalDistance;
}

static uint32_t getCurrentTime(void) {
  time_t s;
  uint16_t ms;
  time_ms(&s, &ms);
  return ((int32_t)s * 1000) + ms;
}

static float fast_sqrtf(float x) {
  union {
    float f;
    uint32_t i;
  } u = { x };
  u.i = 0x1FBD1DF5 + (u.i >> 1);
  return u.f;
}

float euclidean_distance(float x, float y) {
  if (fabs(x) < 0.01) {
    return y;
  }
  if (fabs(y) < 0.01) {
    return x;
  }

  return fast_sqrtf(x * x + y * y);

  float theta = atan2f(y, x);
  if (theta > 1.5 || theta < -1.5) {
    return y / sinf(theta);
  } else {
    return x / cosf(theta);
  }
}

static uint32_t getDistance(Position* position1, Position* position2) {
  float dlat = (position2->latitude - position1->latitude) * 111000.0f; // meters
  float dlon = (position2->longitude - position1->longitude) * 111000.0f * cosf(position1->latitude * DEG_TO_RAD);

  float distance = euclidean_distance(dlat, dlon);
  return (uint32_t)(distance * 1000.0f);
}


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

  float latitude = intLatitude * 1e-6f;
  float longitude = intLongitude * 1e-6f;

  Position newPosition;
  newPosition.latitude = latitude;
  newPosition.longitude = longitude;
  newPosition.time = getCurrentTime();

  switch (runState) {
    case STATE_WAITING_FOR_GPS:
      runState = STATE_READY;
      break;
    case STATE_RUNNING:;
      uint32_t stepDistance = getDistance(&lastPosition, &newPosition);
      uint32_t displayDistance = distanceMillimeters + stepDistance;

      static char formatted_distance[10];      
      snprintf(formatted_distance, sizeof(formatted_distance), "%lu.%02lu", displayDistance / 1000000, (displayDistance / 10000) % 100);
      text_layer_set_text(s_text_distance, formatted_distance);

      if (stepDistance > 20000) {
        distanceMillimeters += stepDistance;
        uint32_t timeDifference = newPosition.time - lastPosition.time;
        updateSpeedBuffer(timeDifference, stepDistance);

        uint32_t pace = getPace();
        static char formatted_pace[10];
        snprintf(formatted_pace, sizeof(formatted_pace), "%lu:%02lu", pace / 60, pace % 60);
        text_layer_set_text(s_text_pace, formatted_pace);

        lastPosition = newPosition;
      }
      // Don't update last position if we're not updating 
      return;
  }

  lastPosition = newPosition;
}

static void prv_select_click_handler(ClickRecognizerRef recognizer, void *context) {
  switch (runState) {
    case STATE_WAITING_FOR_GPS:
      return;
    case STATE_READY:
      runState = STATE_RUNNING;
      distanceMillimeters = 0;
      elapsedTime = 0;
      startPosition = lastPosition;
      startTime = getCurrentTime();
      text_layer_set_text(s_text_layer, "Run started!");
      statusUpdateTimer = 4;
      initializeSpeedBuffer();
      return;
    case STATE_RUNNING:
      runState = STATE_PAUSED;
      elapsedTime += getCurrentTime() - startTime;
      text_layer_set_text(s_text_layer, "Run paused");
      statusUpdateTimer = 4;
      return;
    case STATE_PAUSED:
      runState = STATE_RUNNING;
      startTime = getCurrentTime();
      text_layer_set_text(s_text_layer, "Run resumed");
      statusUpdateTimer = 4;
      return;
  }
}

static void prv_up_click_handler(ClickRecognizerRef recognizer, void *context) {
  
}

static void prv_down_click_handler(ClickRecognizerRef recognizer, void *context) {
  
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
  int32_t base_height = 24;
  int32_t gap = 6;
  s_text_time = text_layer_create(GRect(8, base_height + row_height * 0, bounds.size.w - 16, row_height));
  text_layer_set_text(s_text_time, "00:00");
  format_text_layer(s_text_time);
  text_layer_set_text_alignment(s_text_time, GTextAlignmentCenter);
  // text_layer_set_font(s_text_time, fonts_get_system_font(FONT_KEY_BITHAM_42_LIGHT));
  layer_add_child(window_layer, text_layer_get_layer(s_text_time));  
  
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
  text_layer_set_text(s_text_pace, "0:00");
  format_text_layer(s_text_pace);
  text_layer_set_font(s_text_pace, fonts_get_system_font(FONT_KEY_BITHAM_42_LIGHT));
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
    if (statusUpdateTimer > 0) {
      statusUpdateTimer -= 1;
    } else{
      if (runState == STATE_WAITING_FOR_GPS) {
        text_layer_set_text(s_text_layer, "Waiting for GPS");
      } else {
        static char formatted_wall_time[10];
        snprintf(formatted_wall_time, sizeof(formatted_wall_time), "%d:%02d:%02d", tick_time->tm_hour, tick_time->tm_min, tick_time->tm_sec);
        text_layer_set_text(s_text_layer, formatted_wall_time);
      }
    }
    
    if (runState == STATE_RUNNING) {
      int16_t totalElapsedTime = (int16_t)((elapsedTime + (getCurrentTime() - startTime)) / 1000);
      static char formatted_time[10];
      
      snprintf(formatted_time, sizeof(formatted_time), "%02d:%02d", totalElapsedTime / 60, totalElapsedTime % 60);
      text_layer_set_text(s_text_time, formatted_time);
    }
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
  app_event_loop();
  prv_deinit();
}
