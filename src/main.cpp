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
              *bat_img,
              *low_bat_img,
              *button_text,
              *charging_img,
              *button[3];
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
    
    LV_IMAGE_DECLARE(battery_icon);
    LV_IMAGE_DECLARE(low_battery);
    LV_IMAGE_DECLARE(charging);
    M5DisplayUI.bat_img = lv_img_create(M5DisplayUI.nav_screen);
    M5DisplayUI.low_bat_img = lv_img_create(M5DisplayUI.nav_screen);
    M5DisplayUI.charging_img = lv_img_create(M5DisplayUI.nav_screen);
    
    if(M5.Axp.GetAPSVoltage() > 4.8)
    {
      lv_image_set_src(M5DisplayUI.charging_img,&charging);
      lv_obj_align(M5DisplayUI.charging_img,LV_ALIGN_RIGHT_MID,0,0);
    }
    else if(battery_percentage < 30)
    {
      lv_image_set_src(M5DisplayUI.low_bat_img,&low_battery);
      lv_obj_align(M5DisplayUI.low_bat_img,LV_ALIGN_RIGHT_MID,0,0);   
    }
    else
    {
      lv_image_set_src(M5DisplayUI.bat_img,&battery_icon);
      lv_obj_align(M5DisplayUI.bat_img,LV_ALIGN_RIGHT_MID,0,0);
    }
    
    
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
void event_cb(lv_event_t *e){
    static uint32_t cnt = 1;
    lv_obj_t * btn = (lv_obj_t*)lv_event_get_target(e);
    lv_obj_t * label = lv_obj_get_child(btn, 0);
    lv_label_set_text_fmt(label, "%"LV_PRIu32, cnt);
    cnt++;
}
void drawUI(){
  
  lv_timer_t *timer = lv_timer_create(screen_update,1000,nullptr); 

  M5DisplayUI.main_screen = lv_obj_create(lv_screen_active()); 
  M5DisplayUI.nav_screen = lv_obj_create(M5DisplayUI.main_screen);
  M5DisplayUI.battery_label = lv_label_create(M5DisplayUI.nav_screen);
  M5DisplayUI.datetime_label = lv_label_create(M5DisplayUI.nav_screen);

  lv_obj_set_size(M5DisplayUI.main_screen,SCREEN_WIDTH,SCREEN_HEIGHT);
  lv_obj_center(M5DisplayUI.main_screen);
  lv_obj_set_flex_flow(M5DisplayUI.main_screen,LV_FLEX_FLOW_COLUMN);
  lv_obj_set_size(M5DisplayUI.nav_screen,SCREEN_WIDTH,30);
  lv_obj_align(M5DisplayUI.battery_label,LV_ALIGN_RIGHT_MID,-30,0);
  lv_obj_align(M5DisplayUI.datetime_label,LV_ALIGN_LEFT_MID,0,0);
    
  lv_obj_set_style_pad_all(M5DisplayUI.nav_screen,0,LV_PART_MAIN);
  lv_obj_set_style_pad_all(M5DisplayUI.main_screen,0,LV_PART_MAIN);
  lv_obj_set_style_margin_all(M5DisplayUI.nav_screen,0,LV_PART_MAIN);
  lv_obj_set_style_bg_color(M5DisplayUI.main_screen, lv_color_hex(0x98a3a2), LV_PART_MAIN);
  lv_obj_set_style_bg_color(M5DisplayUI.nav_screen, lv_color_hex(0x000000), LV_PART_MAIN);
  lv_obj_set_style_text_color(M5DisplayUI.nav_screen, lv_color_hex(0xffffff), LV_PART_MAIN);
    
  
for (int i =0; i < 3;i++){
    M5DisplayUI.button[i] = lv_button_create(M5DisplayUI.main_screen);
    M5DisplayUI.button_text = lv_label_create(M5DisplayUI.button[i]);
    lv_label_set_text(M5DisplayUI.button_text,"0");
    lv_obj_set_size(M5DisplayUI.button_text,80,40);
    
    lv_obj_add_event_cb(M5DisplayUI.button[i],event_cb,LV_EVENT_CLICKED,nullptr);
}
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
