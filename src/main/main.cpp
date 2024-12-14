#include <Arduino.h>
#include <WiFi.h>
#include <lvgl.h>
#include <M5Core2.h>
#include <vector>

TaskHandle_t lvglTaskHandler, sensorTaskHandler, wifiTaskHandler;
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
unsigned long lastTickMillis = 0;

const char* ssid = "kingpin";
const char* password = "florafan87";
std::vector<String> wifi_names;
RTC_TimeTypeDef TimeStruct;

struct {
          M5Display tft;
          
          lv_obj_t 
              *main_screen,
              *nav_screen,
              *battery_label,
              *datetime_label,
              *bat_bar[4],
              *low_bat_img,
              *button_text,
              *charging_img,
              *icons[6],
              *connection_status;
          char bat[4];
} static M5DisplayUI;   
void my_touch_read(lv_indev_t *indev_driver,lv_indev_data_t *data)
{
   uint16_t touchX, touchY;
  
  bool touch = M5DisplayUI.tft.getTouch(&touchX, &touchY);
    
  if(!touch){
    data->state = LV_INDEV_STATE_RELEASED;
  }
  else{   
    data->point.x = touchX;
    data->point.y = touchY;
    data->state = LV_INDEV_STATE_PRESSED;
  } 
}


void my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *color_p)
{
  
  //uint16_t *color_array_16 = (uint16_t *)color_p;
  uint16_t w = (area->x2 - area->x1 + 1);
  uint16_t h = (area->y2 - area->y1 + 1);
  
  
  lv_draw_sw_rgb565_swap(color_p, w*h);
  M5DisplayUI.tft.pushImage(area->x1,area->y1,w,h,(uint16_t*)color_p);
  
  
  lv_disp_flush_ready(disp);
}


/**
 * update info on m5 core2
 */
void screen_update()
{
    //make time struct
    
    uint8_t battery_percentage = m5.Axp.GetBatteryLevel();
    
    snprintf(M5DisplayUI.bat, sizeof(M5DisplayUI.bat),"%d",battery_percentage);
    lv_label_set_text(M5DisplayUI.connection_status,WiFi.status() == WL_CONNECTED?"Connected..." : "Not Connected...");
    lv_label_set_text_fmt(M5DisplayUI.battery_label,"%s%%", M5DisplayUI.bat);
    M5.Rtc.GetTime(&TimeStruct);
    lv_label_set_text_fmt(
      M5DisplayUI.datetime_label,"%02d : %02d : %02d %s", 
      TimeStruct.Hours>12?TimeStruct.Hours-12:TimeStruct.Hours, 
      TimeStruct.Minutes, 
      TimeStruct.Seconds,
      TimeStruct.Hours >= 12? "PM":"AM");   
}
void wifi_list_task(void *pvParams){
  //lv_label_set_text(M5DisplayUI.wifi_list,"wifi");
  while(1){
    
    
   
    vTaskDelay(3000);
  } 
    
}
void drawUI(){
  M5DisplayUI.main_screen = lv_obj_create(lv_screen_active()); 
  M5DisplayUI.nav_screen = lv_obj_create(M5DisplayUI.main_screen);
  M5DisplayUI.battery_label = lv_label_create(M5DisplayUI.nav_screen);
  M5DisplayUI.datetime_label = lv_label_create(M5DisplayUI.nav_screen);
  M5DisplayUI.connection_status = lv_label_create(M5DisplayUI.main_screen); 
  lv_obj_center(M5DisplayUI.connection_status);
  /*for (u_int8_t i = 0; i < 4; i++){
    M5DisplayUI.bat_bar[i] = lv_obj_create(M5DisplayUI.nav_screen);
    lv_obj_align(M5DisplayUI.bat_bar[i],LV_ALIGN_RIGHT_MID,-40+i*10,0);
    lv_list_add_button(M5DisplayUI.bat_bar[i],LV_SYMBOL_BATTERY_1,"");
    lv_obj_set_size(M5DisplayUI.bat_bar[i],50,20);
    lv_obj_set_style_bg_color(M5DisplayUI.bat_bar[i],lv_color_hex(0x7175ab),0);
  }*/
  M5DisplayUI.bat_bar[0] = lv_obj_create(M5DisplayUI.nav_screen);
  lv_obj_align(M5DisplayUI.bat_bar[0],LV_ALIGN_RIGHT_MID,-10,0);
  lv_obj_set_size(M5DisplayUI.bat_bar[0],40,20);
  lv_list_add_button(M5DisplayUI.bat_bar[0],LV_SYMBOL_BATTERY_FULL,"");
  
  lv_obj_set_size(M5DisplayUI.main_screen,SCREEN_WIDTH,SCREEN_HEIGHT);
  lv_obj_center(M5DisplayUI.main_screen);
  lv_obj_set_flex_flow(M5DisplayUI.main_screen,LV_FLEX_FLOW_COLUMN);
  lv_obj_set_size(M5DisplayUI.nav_screen,SCREEN_WIDTH,30);
  lv_obj_align(M5DisplayUI.battery_label,LV_ALIGN_RIGHT_MID,-60,0);
  lv_obj_align(M5DisplayUI.datetime_label,LV_ALIGN_LEFT_MID,0,0);
  
  lv_obj_set_style_pad_all(M5DisplayUI.nav_screen,0,LV_PART_MAIN);
  lv_obj_set_style_pad_all(M5DisplayUI.main_screen,0,LV_PART_MAIN);
  lv_obj_set_style_margin_all(M5DisplayUI.nav_screen,0,LV_PART_MAIN);
  lv_obj_set_style_bg_color(M5DisplayUI.main_screen, lv_color_hex(0x98a3a2), LV_PART_MAIN);
  lv_obj_set_style_bg_color(M5DisplayUI.nav_screen, lv_color_hex(0x948d8d), LV_PART_MAIN);
  lv_obj_set_style_text_color(M5DisplayUI.nav_screen, lv_color_hex(0x000000), LV_PART_MAIN); 
 
}
void sensorsTask(void *pvParams){
   
   // WiFi.scanNetworks will return the number of networks found
   while(1){
    
  
  Serial.println("");
  vTaskDelay(10000);
   }
}

void setupLVGL(void *pvParams){
  M5.begin();
  M5DisplayUI.tft.setBrightness(200);
  M5DisplayUI.tft.setRotation(1);
  lv_init();
  
  static lv_color_t buf[SCREEN_WIDTH * 10];
  lv_display_t *display = lv_display_create(SCREEN_WIDTH, SCREEN_HEIGHT);
  /*Declare a buffer for 1/10 screen size*/
  #define BYTE_PER_PIXEL (LV_COLOR_FORMAT_GET_SIZE(LV_COLOR_FORMAT_RGB565)) /*will be 2 for RGB565 */
  static uint8_t buf1[SCREEN_WIDTH * SCREEN_HEIGHT / 10 * BYTE_PER_PIXEL];

  lv_display_set_buffers(display, buf1, NULL, sizeof(buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);  /*Initialize the display buffer.*/
  lv_display_set_flush_cb(display,my_disp_flush);
  
  lv_indev_t * indev = lv_indev_create();           /*Create an input device*/
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);  /*Touch pad is a pointer-like device*/
  lv_indev_set_read_cb(indev, my_touch_read);
  
  drawUI(); 
  while(1){
    screen_update();
    u_int8_t tickPeriod = millis() - lastTickMillis;
  lv_tick_inc(tickPeriod);
  lastTickMillis = millis();
  lv_timer_handler();
  vTaskDelay(1);
  } 
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
 
  xTaskCreatePinnedToCore(setupLVGL,"setupLVGL",1024*10,NULL,3,&lvglTaskHandler,0);
  xTaskCreatePinnedToCore(wifi_list_task,"wifi_list_task",1024*3,NULL,2,&wifiTaskHandler,1);
  xTaskCreatePinnedToCore(sensorsTask,"sensorsTask",1024*3,NULL,1,&sensorTaskHandler,1);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
  
}

void loop() {}
