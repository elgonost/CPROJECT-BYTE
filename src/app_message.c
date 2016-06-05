#include <pebble.h>

#define BUFFER_SIZE 900 
// declaration of the byte arrays we send
// X,Y,Z are 4 digit numbers so we only need 2 concecutive bytes
// to represent each one
#define XYZ_BYTE_ARRAY_SIZE BUFFER_SIZE*2
// Timestamp is considered 9 digit number for convenience so we use 5 concecutive bytes
// to represent each timestamp value.
#define TIME_BYTE_ARRAY_SIZE BUFFER_SIZE/NUM_SAMPLES*7
#define NUM_SAMPLES 25

#define NUM_MENU_SECTIONS 2
#define NUM_FIRST_MENU_ITEMS 1
#define NUM_SECOND_MENU_ITEMS 18

#define NUM_ADL_MENU_SECTIONS 1
#define NUM_DURATION_MENU_SECTIONS 1 
#define NUM_DURATION_MENU_ITEMS 4

#define ACTIVITIES_BUFFER_SIZE 10
#define ACTIVITIES_TIME_BUFFER_SIZE 7*ACTIVITIES_BUFFER_SIZE

static Window *s_window,*first_window,*third_window;	
static SimpleMenuLayer *s_simple_menu_layer,*duration_simple_menu_layer;

static SimpleMenuSection s_menu_sections[NUM_MENU_SECTIONS];
static SimpleMenuSection duration_menu_sections[NUM_DURATION_MENU_SECTIONS];

static SimpleMenuItem s_first_menu_items[NUM_FIRST_MENU_ITEMS];
static SimpleMenuItem s_second_menu_items[NUM_SECOND_MENU_ITEMS];
static SimpleMenuItem duration_menu_items[NUM_DURATION_MENU_ITEMS];

static int s_battery_level;
static TextLayer *s_battery_layer;

static TextLayer *s_time_layer;
DictionaryIterator *iter;
bool js_flag = false;
bool data_col = false;
bool activity_flag = false;

//Byte arrays
uint8_t XBytes[XYZ_BYTE_ARRAY_SIZE];
uint8_t YBytes[XYZ_BYTE_ARRAY_SIZE];
uint8_t ZBytes[XYZ_BYTE_ARRAY_SIZE];
uint8_t TimeBytes[TIME_BYTE_ARRAY_SIZE];	

uint8_t ActivityCode[ACTIVITIES_BUFFER_SIZE];

uint8_t StartTimeBytes[ACTIVITIES_TIME_BUFFER_SIZE];	
uint8_t EndTimeBytes[ACTIVITIES_TIME_BUFFER_SIZE];	

int counter = 0;
int timeCounter = 0;

int act_index = 0;
int select_counter = 0;
int number_of_activities = 0;
char* adl = "None";
char* duration = "None";
char* status = "Disabled";

// Keys for AppMessage Dictionary
// These should correspond to the values 
//you defined in appinfo.json/Settings
enum {
	STATUS_KEY = 0,	
	MESSAGE_KEY = 1,
  X_KEY = 2,
  Y_KEY = 3,
  Z_KEY = 4,
  TIME_KEY = 5,
  TIME_VALUE_START = 6,
  TIME_VALUE_END = 7,
  ACTIVITY = 8,
  ACTIVITY_NUMBER = 9
};

struct byteForm{
  uint8_t first_half;
  uint8_t second_half;
};

struct timeByteForm{
  uint8_t byte_0;
  uint8_t byte_1;
  uint8_t byte_2;
  uint8_t byte_3;
  uint8_t byte_4;
  
  uint8_t byte_5;
  uint8_t byte_6;
};

//Converts the accelerometer values to bytes
struct byteForm convert_xyz_to_bytes(int value){
  struct byteForm split;
  
  //OR operation with 128 is equal to making MSB = 1
  //which represents the negative sign
  if(value<0)
    split.first_half = (abs(value)/100) | 128;
  else
    split.first_half = abs(value)/100;
  split.second_half = abs(value)%100;
  //APP_LOG(APP_LOG_LEVEL_DEBUG, "Test value: %d. Converted to %d%d",value,split.first_half,split.second_half);
  return split;
}

struct timeByteForm convert_time_to_bytes(int value1,int value2){
  struct timeByteForm split;
  value2 = abs(value2);
  value1 = abs(value1);
  split.byte_0 = value2 % 100;
  split.byte_1 = (value2/100) % 100;
  split.byte_2 = (value2/10000) % 100;
  split.byte_3 = (value2/1000000);
  split.byte_4 = value1 % 100;
  split.byte_5 = (value1/100) % 100;
  split.byte_6 = (value1/10000);
  return split;
}

struct byteForm temp;
struct timeByteForm temp2,referenceTimestamp,choiceTimestamp;

static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  // Write the current hours and minutes into a buffer
  static char s_buffer[8];
  strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ?
                                          "%H:%M" : "%I:%M", tick_time);

  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, s_buffer);
  
}

static void battery_callback(BatteryChargeState state) {
  APP_LOG(APP_LOG_LEVEL_DEBUG,"battery level is %d ",s_battery_level);
  // Record the new battery level
  s_battery_level = state.charge_percent;
  // Update meter
  //layer_mark_dirty(s_battery_layer);
  static char s_buffer[128];
   
  // Compose string of all data
  snprintf(s_buffer, sizeof(s_buffer),"Battery: %d%%",s_battery_level);
  text_layer_set_text(s_battery_layer, s_buffer);
}

/*static void battery_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  // Find the width of the bar
  int width = (int)(float)(((float)s_battery_level / 100.0F) * 114.0F);

  // Draw the background
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  // Draw the bar
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_rect(ctx, GRect(0, 0, width, bounds.size.h), 0, GCornerNone);
  
  
  
}*/

void up_single_click_handler(ClickRecognizerRef recognizer, void *context) {
  //Window *window = (Window *)context;
  window_stack_push(first_window,true);
  
}

void config_provider(Window *window) {
 // single click / repeat-on-hold config:
  window_single_click_subscribe(BUTTON_ID_UP, up_single_click_handler);
}

static void menu_select_callback(int index, void *ctx) {
  select_counter++;
  APP_LOG(APP_LOG_LEVEL_DEBUG,"Select counter is %d",select_counter);
  SimpleMenuItem *menu_item = &s_first_menu_items[index];
  if(select_counter % 2 == 0){
    status="Disabled";
    menu_item->subtitle=status;
    data_col = false;
  }
  else{
    status="Enabled";
    menu_item->subtitle=status;
    data_col = true;
  }
  layer_mark_dirty(simple_menu_layer_get_layer(s_simple_menu_layer));
}

static void adl_select_callback(int index, void *ctx){
  act_index = index;
  APP_LOG(APP_LOG_LEVEL_DEBUG,"Choice is %d",index);
  window_stack_push(third_window, true);
  layer_mark_dirty(simple_menu_layer_get_layer(s_simple_menu_layer));
}

static void duration_select_callback(int index, void *ctx){
    
  time_t sec;
  uint16_t ms;
  time_ms(&sec,&ms);
  int vl1,vl2;
  
  vl1 = (sec%100000)*1000 + ms ;
  vl2 = sec / 100000;
  //long long int me = vl2*100000000 + vl1;
  //APP_LOG(APP_LOG_LEVEL_DEBUG,"long int is %lli",me);
  //APP_LOG(APP_LOG_LEVEL_DEBUG,"seconds are %ld and ms are %d",sec,ms);
  //APP_LOG(APP_LOG_LEVEL_DEBUG,"seconds are %d and ms are %d",vl2,vl1);
  referenceTimestamp = convert_time_to_bytes(vl2,vl1);
  
  if(index==0){
    duration = "1 minutes";
    APP_LOG(APP_LOG_LEVEL_DEBUG,"Choice is %s",duration);
    sec = sec - 60;
  } 
  else if (index==1){
    duration = "2 minutes";
    APP_LOG(APP_LOG_LEVEL_DEBUG,"Choice is %s",duration);
    sec = sec - 120;    
  } 
  else if (index==2){
    duration = "5 minutes";
    APP_LOG(APP_LOG_LEVEL_DEBUG,"Choice is %s",duration);
    sec = sec - 300;  
  }
  else if (index==3){
    duration = "20 minutes";
    APP_LOG(APP_LOG_LEVEL_DEBUG,"Choice is %s",duration);
    sec = sec - 1200;   
  } 
  vl1 = (sec%100000)*1000 + ms ;
  vl2 = sec / 100000;
  choiceTimestamp = convert_time_to_bytes(vl2,vl1);
  
  
  window_stack_remove(third_window,true);
  //window_stack_remove(second_window,true);
  window_stack_remove(first_window,true);
  layer_mark_dirty(simple_menu_layer_get_layer(duration_simple_menu_layer));
  
  StartTimeBytes[7*number_of_activities] = choiceTimestamp.byte_6;
  StartTimeBytes[7*number_of_activities+1] = choiceTimestamp.byte_5;
  StartTimeBytes[7*number_of_activities+2] = choiceTimestamp.byte_4;
  StartTimeBytes[7*number_of_activities+3] = choiceTimestamp.byte_3;
  StartTimeBytes[7*number_of_activities+4] = choiceTimestamp.byte_2;
  StartTimeBytes[7*number_of_activities+5] = choiceTimestamp.byte_1;
  StartTimeBytes[7*number_of_activities+6] = choiceTimestamp.byte_0;
  
  
  EndTimeBytes[7*number_of_activities] = referenceTimestamp.byte_6;
  EndTimeBytes[7*number_of_activities+1] = referenceTimestamp.byte_5;
  EndTimeBytes[7*number_of_activities+2] = referenceTimestamp.byte_4;
  EndTimeBytes[7*number_of_activities+3] = referenceTimestamp.byte_3;
  EndTimeBytes[7*number_of_activities+4] = referenceTimestamp.byte_2;
  EndTimeBytes[7*number_of_activities+5] = referenceTimestamp.byte_1;
  EndTimeBytes[7*number_of_activities+6] = referenceTimestamp.byte_0;
  
  ActivityCode[number_of_activities]=act_index;
  number_of_activities++;

  if(number_of_activities>9)
    number_of_activities = 0;
  
  //enables sending activity data
  activity_flag = true;
}


static void first_window_load(Window *window) {

  int num_a_items = 0;
  int num_b_items = 0;
  s_first_menu_items[num_a_items++] = (SimpleMenuItem) {
    .title = "Collect and transmit",
    .subtitle = status,
    .callback = menu_select_callback,
  };
  s_second_menu_items[num_b_items++]= (SimpleMenuItem) {
    .title = "Mild",
    .callback = adl_select_callback,
  };
  s_second_menu_items[num_b_items++]= (SimpleMenuItem) {
    .title = "Moderate",
    .callback = adl_select_callback,
  };
  s_second_menu_items[num_b_items++]= (SimpleMenuItem) {
    .title = "Intense",
    .callback = adl_select_callback,
  };
  s_second_menu_items[num_b_items++]= (SimpleMenuItem) {
    .title = "Cycling",
    .callback = adl_select_callback,
  };
  s_second_menu_items[num_b_items++]= (SimpleMenuItem) {
    .title = "Dressing",
    .callback = adl_select_callback,
  };
  s_second_menu_items[num_b_items++]= (SimpleMenuItem) {
    .title = "Driving",
    .callback = adl_select_callback,
  };
  s_second_menu_items[num_b_items++]= (SimpleMenuItem) {
    .title = "Fall",
    .callback = adl_select_callback,
  };
  s_second_menu_items[num_b_items++]= (SimpleMenuItem) {
    .title = "Mobile",
    .callback = adl_select_callback,
  };
  s_second_menu_items[num_b_items++]= (SimpleMenuItem) {
    .title = "Running",
    .callback = adl_select_callback,
  };
  s_second_menu_items[num_b_items++]= (SimpleMenuItem) {
    .title = "Sitting",
    .callback = adl_select_callback,
  };
  s_second_menu_items[num_b_items++]= (SimpleMenuItem) {
    .title = "Sleeping early day",
    .callback = adl_select_callback,
  };
  s_second_menu_items[num_b_items++]= (SimpleMenuItem) {
    .title = "Sleeping mid day",
    .callback = adl_select_callback,
  };
  s_second_menu_items[num_b_items++]= (SimpleMenuItem) {
    .title = "Sleeping late day",
    .callback = adl_select_callback,
  };
  s_second_menu_items[num_b_items++]= (SimpleMenuItem) {
    .title = "Standing",
    .callback = adl_select_callback,
  };
  s_second_menu_items[num_b_items++]= (SimpleMenuItem) {
    .title = "Non Fall Spike",
    .callback = adl_select_callback,
  };
  s_second_menu_items[num_b_items++]= (SimpleMenuItem) {
    .title = "Walking",
    .callback = adl_select_callback,
  };
  s_second_menu_items[num_b_items++]= (SimpleMenuItem) {
    .title = "Washing in front of mirror",
    .callback = adl_select_callback,
  };
  s_second_menu_items[num_b_items++]= (SimpleMenuItem) {
    .title = "Watching TV",
    .callback = adl_select_callback,
  };  
  s_menu_sections[0] = (SimpleMenuSection) {
    .title = "Data",
    .num_items = NUM_FIRST_MENU_ITEMS,
    .items = s_first_menu_items,
  };
  s_menu_sections[1] = (SimpleMenuSection) {
    .title = "Tag activity",
    .num_items = NUM_SECOND_MENU_ITEMS,
    .items = s_second_menu_items,
  };
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_frame(window_layer);
  s_simple_menu_layer = simple_menu_layer_create(bounds, window, s_menu_sections, NUM_MENU_SECTIONS, NULL);
  layer_add_child(window_layer, simple_menu_layer_get_layer(s_simple_menu_layer));
}

void first_window_unload(Window *window) {
  simple_menu_layer_destroy(s_simple_menu_layer);  
}

void third_window_load(Window *window){
  int num_c_items = 0;
  
  duration_menu_items[num_c_items++]= (SimpleMenuItem) {
    .title = "1 minute",
    .callback = duration_select_callback,
  };
  duration_menu_items[num_c_items++]= (SimpleMenuItem) {
    .title = "2 minutes",
    .callback = duration_select_callback,
  };
  duration_menu_items[num_c_items++]= (SimpleMenuItem) {
    .title = "5 minutes",
    .callback = duration_select_callback,
  };
  duration_menu_items[num_c_items++]= (SimpleMenuItem) {
    .title = "20 minutes",
    .callback = duration_select_callback,
  };
  
  duration_menu_sections[0] = (SimpleMenuSection) {
    .num_items = NUM_DURATION_MENU_ITEMS,
    .title = "Select duration window",
    .items = duration_menu_items,
  };
  
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_frame(window_layer);
  duration_simple_menu_layer = simple_menu_layer_create(bounds, window, duration_menu_sections, NUM_DURATION_MENU_SECTIONS, NULL);
  layer_add_child(window_layer, simple_menu_layer_get_layer(duration_simple_menu_layer)); 
  
}

void third_window_unload(Window *window){
  simple_menu_layer_destroy(duration_simple_menu_layer);
}

static void data_handler(AccelData *data, uint32_t num_samples) {
    
  if((js_flag)&&(data_col)){
    //javascript is ready and we can start gathering data
    if(counter<BUFFER_SIZE){
      //keep gathering data
      for(int i=0;i<NUM_SAMPLES;i++){
        temp = convert_xyz_to_bytes(data[i].x);
        XBytes[2*counter] = temp.first_half;  
        XBytes[2*counter+1] = temp.second_half;
        
        temp = convert_xyz_to_bytes(data[i].y);
        YBytes[2*counter] = temp.first_half;  
        YBytes[2*counter+1] = temp.second_half;
        
        temp = convert_xyz_to_bytes(data[i].z);
        ZBytes[2*counter] = temp.first_half;  
        ZBytes[2*counter+1] = temp.second_half;
        
        if(i==0){
  
          temp2 = convert_time_to_bytes(data[i].timestamp / 100000000,data[i].timestamp % 100000000);
          
          TimeBytes[timeCounter] = temp2.byte_6;
          TimeBytes[timeCounter+1] = temp2.byte_5;
          TimeBytes[timeCounter+2] = temp2.byte_4;
          TimeBytes[timeCounter+3] = temp2.byte_3;
          TimeBytes[timeCounter+4] = temp2.byte_2;
          TimeBytes[timeCounter+5] = temp2.byte_1;
          TimeBytes[timeCounter+6] = temp2.byte_0;
          
          timeCounter+=7;
        }
        counter++;
      }
      //for debugging
      if(counter==250)
        APP_LOG(APP_LOG_LEVEL_DEBUG,"Counter is 250");
      if(counter==500)
        APP_LOG(APP_LOG_LEVEL_DEBUG,"Counter is 500");
      if(counter==750)
        APP_LOG(APP_LOG_LEVEL_DEBUG,"Counter is 750");
      if(counter==1000)
        APP_LOG(APP_LOG_LEVEL_DEBUG,"Counter is 1000");
      
    }
    else{
      //When we get here our Buffers are full. We need to send data
      //to PebbleKitJS.
      app_message_outbox_begin(&iter);
      dict_write_data(iter,X_KEY,&XBytes[0],sizeof(XBytes));
      dict_write_data(iter,Y_KEY,&YBytes[0],sizeof(YBytes));
      dict_write_data(iter,Z_KEY,&ZBytes[0],sizeof(ZBytes));
      dict_write_data(iter,TIME_KEY,&TimeBytes[0],sizeof(TimeBytes));
      dict_write_end(iter);
      app_message_outbox_send();      
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Message was sent to PebbleKitJS"); 
      counter = 0;
      timeCounter = 0;
    }
  }
}
// Called when a message is received from PebbleKitJS
static void in_received_handler(DictionaryIterator *received, void *context) {
	Tuple *tuple;
	
	tuple = dict_find(received, STATUS_KEY);
	if(tuple) {
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Received Status: %d", (int)tuple->value->uint32); 
	}
	
	tuple = dict_find(received, MESSAGE_KEY);
	if(tuple) {
		APP_LOG(APP_LOG_LEVEL_DEBUG, "Received Message: %s", tuple->value->cstring);
	}
  js_flag = true;
  APP_LOG(APP_LOG_LEVEL_DEBUG, "JS_FLAG is set true");
}

// Called when an incoming message from PebbleKitJS is dropped
static void in_dropped_handler(AppMessageResult reason, void *context) {	
}

// Called when PebbleKitJS does not acknowledge receipt of a message
static void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
}

// Called when PebbleKitJS acknowledges receipt of a message
static void out_sent_handler(DictionaryIterator *iter, void *context) {
  
  if (activity_flag){
    
    app_message_outbox_begin(&iter);
    dict_write_data(iter,TIME_VALUE_START,&StartTimeBytes[0],sizeof(StartTimeBytes)/10*number_of_activities);
    dict_write_data(iter,TIME_VALUE_END,&EndTimeBytes[0],sizeof(EndTimeBytes)/10*number_of_activities);
    //dict_write_cstring(iter, ACTIVITY, adl);
    dict_write_int(iter, ACTIVITY_NUMBER, &number_of_activities, sizeof(int), true);
    dict_write_data(iter,ACTIVITY,&ActivityCode[0],sizeof(ActivityCode)/10*number_of_activities);
    dict_write_end(iter);
    app_message_outbox_send();
    activity_flag = false;
    number_of_activities = 0;
    
    //reseting arrays!!
    /*memset(ActivityCode,0,sizeof(ActivityCode));
    memset(StartTimeBytes,0,sizeof(StartTimeBytes));
    memset(EndTimeBytes,0,sizeof(EndTimeBytes));*/
    
    //APP_LOG(APP_LOG_LEVEL_DEBUG, "Activity message was sent to PebbleKitJS"); 
    APP_LOG(APP_LOG_LEVEL_DEBUG, "OUT_SENT_HANDLER HERE");     
  } 
}
static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect window_bounds = layer_get_bounds(window_layer);

  // Create output TextLayer
  s_time_layer = text_layer_create(GRect(0, PBL_IF_ROUND_ELSE(58, 45), window_bounds.size.w, 50));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorBlack);
  text_layer_set_text(s_time_layer, "00:00");
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
  
 
  s_battery_layer = text_layer_create(GRect(0, 90, window_bounds.size.w - 10, window_bounds.size.h-140));
  text_layer_set_font(s_battery_layer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD));
  text_layer_set_text(s_battery_layer, "No data yet.");
  text_layer_set_overflow_mode(s_battery_layer, GTextOverflowModeWordWrap);
  text_layer_set_text_alignment(s_battery_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_battery_layer));
  
  
  
}

static void main_window_unload(Window *window) {
  // Destroy output TextLayer
  text_layer_destroy(s_time_layer);
}

static void init(void) {
	s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });  
  window_set_click_config_provider(s_window, (ClickConfigProvider) config_provider);
	window_stack_push(s_window, true);
  
  first_window = window_create();
  window_set_window_handlers(first_window, (WindowHandlers) {
    .load = first_window_load,
    .unload = first_window_unload,
  });
  
  third_window = window_create();
  window_set_window_handlers(third_window, (WindowHandlers) {
    .load = third_window_load,
    .unload = third_window_unload,
  });
    
  update_time();
  battery_state_service_subscribe(battery_callback);
  battery_callback(battery_state_service_peek());
	
	// Register AppMessage handlers
	app_message_register_inbox_received(in_received_handler); 
	app_message_register_inbox_dropped(in_dropped_handler); 
	app_message_register_outbox_failed(out_failed_handler);
  app_message_register_outbox_sent(out_sent_handler);

  accel_data_service_subscribe(NUM_SAMPLES, data_handler);
  accel_service_set_sampling_rate(ACCEL_SAMPLING_25HZ);
  
  // Initialize AppMessage inbox and outbox buffers with a suitable size
  const int inbox_size = 128;
  const int outbox_size = 6000;
  app_message_open(inbox_size, outbox_size);
  
  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

}

static void deinit(void) {
	app_message_deregister_callbacks();
  window_destroy(third_window);
  window_destroy(first_window);
	window_destroy(s_window);
}

int main( void ) {
	init();
	app_event_loop();
	deinit();
}