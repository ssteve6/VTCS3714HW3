#include <pebble.h>
#include "png.h"
#include "upng.h"
#define DATA_LENGTH 0
#define DATA_PAYLOAD 1
#define DATA_INDEX 2
static Window *window;

static int dataLength=0;
static GBitmap *receivedImage;
static BitmapLayer *png_layer;
static TextLayer *text_layer;
static TextLayer *time_layer;
static uint32_t indexToReceive=0;
static uint8_t *pic_buffer;
static bool black=true;
static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
    if(black){
        text_layer_set_text_color(time_layer, GColorWhite);
        black=false;
    }else{
        text_layer_set_text_color(time_layer, GColorBlack);
        black=true;
    }
}

static void set_wallpaper(){
    if (receivedImage) {
        // TODO: DESTROY IMAGE THAT'S ALREADY BEEN SET
        gbitmap_destroy(receivedImage);
    }
    GBitmap *newImage=gbitmap_create_with_png_data(pic_buffer, dataLength);
    layer_set_hidden((Layer *)text_layer, true);
    
    // TODO: SET BITMAP LAYER
  
    bitmap_layer_set_bitmap(png_layer, newImage);
    
    receivedImage=newImage;
    
    // TODO: SEND SHORT PULSE
    vibes_short_pulse();
}
static void click_config_provider(void *context) {
    window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
}

static void update_time() {
    // Get a tm structure
    time_t temp = time(NULL);
    struct tm *tick_time = localtime(&temp);
    // Write the current hours and minutes into the buffer
    static char buffer[] = "00:00";
    if(clock_is_24h_style() == true) {
        // Use 24 hour format
        strftime(buffer, sizeof("00:00"), "%H:%M", tick_time);
    } else {
        // Use 12 hour format
        strftime(buffer, sizeof("00:00"), "%I:%M", tick_time);
    }
    // Display this time on the TextLayer
    text_layer_set_text(time_layer, buffer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
    update_time();
}

static void window_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);
    
    // TODO: CREATE UI
    png_layer = bitmap_layer_create(GRect(0,0,144,168));
    layer_add_Child(window_layer, bitmap_layer_get_layer(png_layer));
    
    //Register timem listener
    tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
    time_layer = text_layer_create(GRect(0, 0, 144, 168));
    text_layer_set_baground_color(time_layer, GColorClear);
    text_layer_set_text_alignment(time_layer, GTextAlignmentCenter);
    text_layer_set_font(time_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
    layer_add_child(window_layer, text_layer_get_layer(time_layer));
  
    text_layer = text_layer_create(GRect(0, 80, 144, 30));
    text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);
    text_layer_set_font(text_layer, FONT_KEY_BITHAM_42_BOLD);
    layer_add_child(window_layer, text_layer);
}

static void window_unload(Window *window) {
    // TODO: DESTORY TEXT LAYER
    
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
    static char str[20];
    static int pkgSize=124;
    // Read first item
    Tuple *t = dict_read_first(iterator);
    static int pkgNum=0;
    while(t != NULL) {
        // Which key was received?
        if(t->key==DATA_LENGTH){
            dataLength=(int)t->value->int32;
            pic_buffer=malloc(dataLength);
            pkgNum=dataLength/pkgSize+1;
            snprintf(str, sizeof(str),"%d", (int)pkgNum);
            text_layer_set_text(text_layer, str);
            layer_set_hidden((Layer *)text_layer, false);
        }else{
            indexToReceive=(t->key-1)*pkgSize;
            memcpy(&pic_buffer[indexToReceive],t->value->data,t->length);
            int percentage=((int)t->key)*100/pkgNum;
            snprintf(str, sizeof(str),"%d%%",percentage);
            text_layer_set_text(text_layer, str);
            if(percentage==100){
                set_wallpaper();
            }
        }
        // Look for next item
        t = dict_read_next(iterator);
    }
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

static void tap_handler(AccelAxisType axis, int32_t direction) {
    switch (axis) {
        case ACCEL_AXIS_X:
        case ACCEL_AXIS_Y:
        case ACCEL_AXIS_Z:
        if(black){
            text_layer_set_text_color(time_layer, GColorWhite);
            black=false;
        }else{
            text_layer_set_text_color(time_layer, GColorBlack);
            black=true;
        }
        break;
    }
}

static void init(void) {
    window = window_create();
    window_set_click_config_provider(window, click_config_provider);
    window_set_window_handlers(window, (WindowHandlers) {
        .load = window_load,
        .unload = window_unload,
    });
    // Register callbacks
    
    // Open AppMessage
    
    // Push the window
    
    // Subscribe to the accelerometer tap service
    accel_tap_service_subscribe(tap_handler);
}

static void deinit(void) {
    window_destroy(window);
    // TODO: destroy bitmap layer
    bitmap_layer_destroy(png_layer);
    // TODO: destroy bitmap if one was set
    gbitmap_destroy(receivedImage);
}

int main(void) {
    // TODO: INITALIZE APP
    init();
    // TODO: SET EVENT LOOP
    app_event_loop();
    // TODO: DEINITALIZE APP
    deinit();
}