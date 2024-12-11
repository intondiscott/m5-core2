#include <Arduino.h>
#include <lvgl.h>
#include <M5Core2.h>
#include <battery_icon.h>
#include <low_battery.h>
#include <charging.h>

M5Display tft;

const int SCREEN_WIDTH = 320;
const int SCREEN_HEIGHT = 240;
unsigned long lastTickMillis = 0;

 
struct {
          lv_obj_t 
              *main_screen,
              *nav_screen,
              *battery_label,
              *datetime_label,
              *bat_bar[4],
              *low_bat_img,
              *button_text,
              *charging_img,
              *icons[6];
          char bat[4];
} M5DisplayUI;   
void my_touch_read(lv_indev_t *indev_driver,lv_indev_data_t *data)
{
  uint16_t touchX, touchY;
  
  bool touch = tft.getTouch(&touchX, &touchY);
    
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
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);
  
  
  lv_draw_sw_rgb565_swap(color_p, w*h);
  tft.pushImage(area->x1,area->y1,w,h,(uint16_t*)color_p);
  
  
  lv_disp_flush_ready(disp);
}


/**
 * update info on m5 core2
 */
void screen_update(lv_timer_t *timer)
{
    //make time struct
    RTC_TimeTypeDef time_t;
    uint8_t battery_percentage = m5.Axp.GetBatteryLevel();
    
    snprintf(M5DisplayUI.bat, sizeof(M5DisplayUI.bat),"%d",battery_percentage);
    
    lv_label_set_text_fmt(M5DisplayUI.battery_label,"%s%%", M5DisplayUI.bat);
    M5.Rtc.GetTime(&time_t);
    lv_label_set_text_fmt(
      M5DisplayUI.datetime_label,"%02d : %02d : %02d %s", 
      time_t.Hours>12?time_t.Hours-12:time_t.Hours, 
      time_t.Minutes, 
      time_t.Seconds,
      time_t.Hours >= 12? "PM":"AM");   
}

void drawUI(){
  
  lv_timer_t *timer = lv_timer_create(screen_update,1000,nullptr); 
  
  M5DisplayUI.main_screen = lv_obj_create(lv_screen_active()); 
  M5DisplayUI.nav_screen = lv_obj_create(M5DisplayUI.main_screen);
  M5DisplayUI.battery_label = lv_label_create(M5DisplayUI.nav_screen);
  M5DisplayUI.datetime_label = lv_label_create(M5DisplayUI.nav_screen);
  for (int i = 0; i < 4; i++){
    M5DisplayUI.bat_bar[i] = lv_obj_create(M5DisplayUI.nav_screen);
    lv_obj_align(M5DisplayUI.bat_bar[i],LV_ALIGN_RIGHT_MID,-40+i*10,0);
    
    lv_obj_set_size(M5DisplayUI.bat_bar[i],10,20);
    lv_obj_set_style_bg_color(M5DisplayUI.bat_bar[i],lv_color_hex(0x7175ab),0);
  }
 
  
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

void setupLVGL(){
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
  

}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  M5.begin();
 
  tft.setRotation(1);
  
  tft.setBrightness(200);
  
  setupLVGL();

}

void loop() {
  //uint16_t touchX,touchY;
  // put your main code here, to run repeatedly:
  unsigned int tickPeriod = millis() - lastTickMillis;
  lv_tick_inc(tickPeriod);
  lastTickMillis = millis();
  lv_timer_handler();
  
}
