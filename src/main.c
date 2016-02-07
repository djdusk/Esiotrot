/*
 * main.c
 * Esitrot (Tortoise) original
 */

#include <pebble.h>

static Window *s_main_window;
static TextLayer *s_time_layer, *s_date_layer;
static Layer *s_battery_layer;

static GFont s_time_font, s_date_font;
static BitmapLayer *s_bitmap_layer, *s_bt_icon_layer;
static GBitmap *s_bitmap, *s_bt_icon_bitmap;

static int s_battery_level;

static void bluetooth_callback(bool connected) {
  // Show icon if disconnected
  layer_set_hidden(bitmap_layer_get_layer(s_bt_icon_layer), connected);
  
  if(!connected) {
    vibes_double_pulse();
  }
}

static void battery_callback(BatteryChargeState state) {
  //if (charge_state.is_charging) {
  //  
  //}
  
  // Record the new battery level
  s_battery_level = state.charge_percent;
  
  // Update meter
  layer_mark_dirty(s_battery_layer);
}

static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  // Create a long-lived buffer, and show the time
  static char s_buffer[] = "00:00";
  if(clock_is_24h_style()) {
    strftime(s_buffer, sizeof("00:00"), "%H:%M", tick_time);
  } else {
    strftime(s_buffer, sizeof("00:00"), "%I:%M %p", tick_time);
  }
  text_layer_set_text(s_time_layer, s_buffer);
  
  // Show the date
  static char date_buffer[16];
  strftime(date_buffer, sizeof(date_buffer), "%a %d %b", tick_time);
  text_layer_set_text(s_date_layer, date_buffer);
}

static void battery_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  
  // Find the width of the bar
  int width = (int)(float)(((float)s_battery_level / 100.0F) * 114.0F);
  
  // Draw the background
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, bounds, GCornerNone, 0);
  
  // Draw the bar
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, GRect(0, 0, width, bounds.size.h), GCornerNone, 0);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

static void main_window_load(Window *window) {
  // Get information about the Window
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  // Create GBitmap, then set to created BitmapLayer
  s_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_TURTOISE_STATIC);
  s_bitmap_layer = bitmap_layer_create(bounds);
  bitmap_layer_set_bitmap(s_bitmap_layer, s_bitmap);
  bitmap_layer_set_compositing_mode(s_bitmap_layer, GCompOpSet);
  layer_add_child(window_layer, bitmap_layer_get_layer(s_bitmap_layer));
  
  // Create GFonts
  s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_WALT_BOLD_28));
  s_date_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_HANG_REGULAR_26));
  
  // Create T I M E TextLayer
  s_time_layer = text_layer_create(
      GRect(0, PBL_IF_ROUND_ELSE(12, 60), bounds.size.w, 50));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorBlack);
  text_layer_set_text(s_time_layer, "00:00");
  text_layer_set_font(s_time_layer, s_time_font);
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
  
  // Create D A T E TextLayer
  s_date_layer = text_layer_create(
      GRect(0, PBL_IF_ROUND_ELSE(135, 52), bounds.size.w, 50));
  text_layer_set_text_color(s_date_layer, GColorBlack);
  text_layer_set_background_color(s_date_layer, GColorClear);
  text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
  text_layer_set_text(s_date_layer, "Sept 23");
  text_layer_set_font(s_date_layer, s_date_font);
  layer_add_child(window_layer, text_layer_get_layer(s_date_layer));
  
  // Create battery meter Layer
  s_battery_layer = layer_create(GRect(30, 134, 115, 2));
  layer_set_update_proc(s_battery_layer, battery_update_proc);
  layer_add_child(window_get_root_layer(window), s_battery_layer);
  
  // Create the Bluetooth icon GBitmap
  s_bt_icon_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BT_ICON);
  
  // Create the BitmapLayer to display the GBitmap
  s_bt_icon_layer = bitmap_layer_create(GRect(58, 77, 20, 20));
  bitmap_layer_set_bitmap(s_bt_icon_layer, s_bt_icon_bitmap);
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_bt_icon_layer));
  
  // Initialize the display
  update_time();
  battery_callback(battery_state_service_peek());

  bluetooth_callback(connection_service_peek_pebble_app_connection());
}

static void main_window_unload(Window *window) {
  fonts_unload_custom_font(s_time_font);
  fonts_unload_custom_font(s_date_font);
  
  gbitmap_destroy(s_bitmap);
  gbitmap_destroy(s_bt_icon_bitmap);
  
  bitmap_layer_destroy(s_bitmap_layer);
  bitmap_layer_destroy(s_bt_icon_layer);
  
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_date_layer);
  
  layer_destroy(s_battery_layer);
}


static void init() {
  s_main_window = window_create();
  window_set_background_color(s_main_window, COLOR_FALLBACK(GColorVividCerulean, GColorBlack));
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });
  window_stack_push(s_main_window, true);
  
  // Register with Event Services
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  battery_state_service_subscribe(battery_callback);

  connection_service_subscribe((ConnectionHandlers) {
    .pebble_app_connection_handler = bluetooth_callback
  });
  
  // Make sure the time is displayed from the start
  update_time();

  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
}

static void deinit() {
  window_destroy(s_main_window);
}

int main() {
  init();
  app_event_loop();
  deinit();
}