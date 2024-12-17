#include <Arduino.h>
#include <WiFi.h>
#include <lvgl.h>
#include <M5Core2.h>
#include <vector>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "credentials.c"
#include <full_battery.h>
#include <bat_two_bars.h>
#include <low_bat.h>
#include <10d@2x.h>
#include <critical_low_bat.h>
#include <charging.h>
TaskHandle_t lvglTaskHandler, sensorTaskHandler, wifiTaskHandler;
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
unsigned long lastTickMillis = 0;


std::vector<String> wifi_names;
RTC_TimeTypeDef TimeStruct;

struct Weather {
   float temperature = 255.372; //kelvin temp
   int humidity = 0;
   float wind_speed = 0.0;
   std::string icon = "10d";
};

Weather *weather = new Weather;
char weather_buffer[7];
struct {
          M5Display tft;
          
          lv_obj_t 
              *main_screen,
              *nav_screen,
              *battery_label,
              *datetime_label,
              //*bat_bar[4],
              *bat_img,
              *low_bat_img,
              *button_text,
              *charging_img,
              *icons[6],
              *connection_status,
              *weather_conditions,
              *temperature_label,
              *wind_speed_label,
              *humidity_label;
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
    snprintf(weather_buffer,sizeof(weather_buffer), "%3.2f",(weather->temperature - 273.15)*9/5+32);
    lv_label_set_text_fmt(M5DisplayUI.temperature_label,"Temp: %sF",weather_buffer);
    lv_label_set_text_fmt(M5DisplayUI.humidity_label,"Hum: %d%%",weather->humidity);
    snprintf(weather_buffer,sizeof(weather_buffer), "%3.2f",weather->wind_speed);
    lv_label_set_text_fmt(M5DisplayUI.wind_speed_label,"Wind Speed: %s MPH", weather_buffer);
    lv_label_set_text_fmt(M5DisplayUI.battery_label,"%s%%", M5DisplayUI.bat);
    M5.Rtc.GetTime(&TimeStruct);
    lv_label_set_text_fmt(
      M5DisplayUI.datetime_label,"%02d : %02d : %02d %s", 
      TimeStruct.Hours>12?TimeStruct.Hours-12:TimeStruct.Hours, 
      TimeStruct.Minutes, 
      TimeStruct.Seconds,
      TimeStruct.Hours >= 12? "PM":"AM");  
    if(M5.Axp.GetAPSVoltage() > 4.7) {
      LV_IMAGE_DECLARE(charging);
      lv_image_set_src(M5DisplayUI.bat_img,&charging);
    }
    else{
      switch (battery_percentage)
      {
        case 80 ... 100:
          LV_IMAGE_DECLARE(full_battery);
          lv_image_set_src(M5DisplayUI.bat_img,&full_battery);
          break;
        case 40 ... 79:
          LV_IMAGE_DECLARE(bat_two_bars);
          lv_image_set_src(M5DisplayUI.bat_img,&bat_two_bars);
          break;
        case 15 ... 39:
          LV_IMAGE_DECLARE(low_bat);
          lv_image_set_src(M5DisplayUI.bat_img,&low_bat);
          break;
        case 0 ... 14:
          LV_IMAGE_DECLARE(critical_low_bat);
          lv_image_set_src(M5DisplayUI.bat_img,&critical_low_bat);
          break;
        default:
          break;
      } 
    }
}
void wifi_list_task(void *pvParams){
  //lv_label_set_text(M5DisplayUI.wifi_list,"wifi");
  while(1){
    
    
   
    vTaskDelay(3000);
  } 
    
}
void drawUI(){
  LV_IMAGE_DECLARE(a10d);
  
  M5DisplayUI.main_screen = lv_obj_create(lv_screen_active()); 
  M5DisplayUI.nav_screen = lv_obj_create(M5DisplayUI.main_screen);
  M5DisplayUI.battery_label = lv_label_create(M5DisplayUI.nav_screen);
  M5DisplayUI.datetime_label = lv_label_create(M5DisplayUI.nav_screen);
  M5DisplayUI.connection_status = lv_label_create(M5DisplayUI.main_screen); 
  M5DisplayUI.temperature_label = lv_label_create(M5DisplayUI.main_screen);
  M5DisplayUI.wind_speed_label = lv_label_create(M5DisplayUI.main_screen);
  M5DisplayUI.humidity_label = lv_label_create(M5DisplayUI.main_screen);
  M5DisplayUI.bat_img = lv_image_create(M5DisplayUI.nav_screen);
  M5DisplayUI.weather_conditions = lv_image_create(M5DisplayUI.main_screen);
  lv_image_set_src(M5DisplayUI.weather_conditions,&a10d);
  lv_obj_align(M5DisplayUI.bat_img, LV_ALIGN_RIGHT_MID,-10,0);

  lv_obj_center(M5DisplayUI.connection_status);
  
  
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
   
   
   while(1){
    
    if(WiFi.status() == WL_CONNECTED){
        HTTPClient http;
    String server_path = "https://api.openweathermap.org/data/2.5/weather?lat=41.3165&lon=-73.0932&appid=";
    http.begin(server_path+MY_SECRET_API_KEY);
    int httpCode = http.GET();
    if(httpCode > 0){
      String payload = http.getString();
      
      JsonDocument doc;
      deserializeJson(doc,payload);
      weather->temperature = doc["main"]["temp"];
      weather->humidity = doc["main"]["humidity"];
      weather->wind_speed = doc["wind"]["speed"];
      Serial.println(payload);
    }
    else{
      Serial.println(httpCode);
    }
    http.end();
    }
    
  
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
 
  xTaskCreatePinnedToCore(setupLVGL,"setupLVGL",1024*10,NULL,2,&lvglTaskHandler,0);
  xTaskCreatePinnedToCore(sensorsTask,"sensorsTask",1024*6,NULL,1,&wifiTaskHandler,1);
  WiFi.mode(WIFI_STA);
  WiFi.begin(MY_SECRET_SSID, MY_SECRET_PASSWORD);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
}

void loop() {}
