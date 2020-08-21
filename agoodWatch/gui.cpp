/*
 * Copyright (c) 2020 Alex Goodyear
 * Derived from ...
Copyright (c) 2019 lewis he
This is just a demonstration. Most of the functions are not implemented.
The main implementation is low-power standby. 
The off-screen standby (not deep sleep) current is about 4mA.
Select standard motherboard and standard backplane for testing.
Created by Lewis he on October 10, 2019.
*/

#include "config.h"
#include <Arduino.h>
#include <time.h>
#include "gui.h"
#include <WiFi.h>
#include "string.h"
#include <Ticker.h>

LV_FONT_DECLARE(Ubuntu);
LV_IMG_DECLARE(bg);
LV_IMG_DECLARE(bg1);
LV_IMG_DECLARE(bg2);
LV_IMG_DECLARE(bg3);
LV_IMG_DECLARE(WALLPAPER_1_IMG);
LV_IMG_DECLARE(WALLPAPER_2_IMG);
LV_IMG_DECLARE(WALLPAPER_3_IMG);
LV_IMG_DECLARE(step);
LV_IMG_DECLARE(menu);

LV_IMG_DECLARE(wifi);
LV_IMG_DECLARE(bluetooth);
LV_IMG_DECLARE(setting);
LV_IMG_DECLARE(on);
LV_IMG_DECLARE(off);
LV_IMG_DECLARE(level1);
LV_IMG_DECLARE(level2);
LV_IMG_DECLARE(level3);
LV_IMG_DECLARE(iexit);
LV_IMG_DECLARE(modules); // AKA About!

extern EventGroupHandle_t g_event_group;
extern QueueHandle_t g_event_queue_handle;

static lv_style_t settingStyle;
static lv_obj_t *mainBar = nullptr;
static lv_style_t mainStyle;
static lv_obj_t *timeLabel = nullptr;
static lv_obj_t *dateLabel = nullptr;
static lv_obj_t *menuBtn = nullptr;
static lv_obj_t *torchLabel = nullptr;

static uint8_t globalIndex = 0;

static void lv_update_task(struct _lv_task_t *);
static void lv_battery_task(struct _lv_task_t *);
static void view_event_handler(lv_obj_t *obj, lv_event_t event);

static void wifi_event_cb();
static void setting_event_cb();
static void bluetooth_event_cb();
static void about_event_cb();
static void wifi_destory();

#ifdef DEBUG_EVENTS
#define DEBUG_EVENTS
/*
 * This is for debugging the LVGL library, requires LV_USE_LOG setting in lv_conf.h
 */
void my_log_cb(lv_log_level_t level, const char * file, int line, const char * fn_name, const char * dsc)
{
  /*Send the logs via serial port*/
  if(level == LV_LOG_LEVEL_ERROR) Serial.print("ERROR: ");
  if(level == LV_LOG_LEVEL_WARN)  Serial.print("WARNING: ");
  if(level == LV_LOG_LEVEL_INFO)  Serial.print("INFO: ");
  if(level == LV_LOG_LEVEL_TRACE) Serial.print("TRACE: ");

  Serial.printf("%s:%d:%s: %s\n", file, line, fn_name, dsc);
}

static void my_test_event_cb(lv_obj_t * obj, lv_event_t event)
{
    Serial.printf ("my_test_event (%p, %d) : ", obj, event);

    switch(event) {
        case LV_EVENT_PRESSED:
            Serial.println("LV_EVENT_PRESSED");
            break;

        case LV_EVENT_PRESSING:
            Serial.println("LV_EVENT_PRESSING");
            break;

        case LV_EVENT_PRESS_LOST:
            Serial.println("LV_EVENT_PRESS_LOST");
            break;

        case LV_EVENT_SHORT_CLICKED:
            Serial.println("LV_EVENT_SHORT_CLICKED");
            break;

        case LV_EVENT_CLICKED:
            Serial.println("LV_EVENT_CLICKED");
            break;

        case LV_EVENT_LONG_PRESSED:
            Serial.println("LV_EVENT_LONG_PRESSED");
            break;

        case LV_EVENT_LONG_PRESSED_REPEAT:
            Serial.println("LV_EVENT_LONG_PRESSED_REPEAT");
            break;

        case LV_EVENT_RELEASED:
            Serial.println("LV_EVENT_RELEASED");
            break;

        case LV_EVENT_DRAG_BEGIN:
            Serial.println("LV_EVENT_DRAG_BEGIN");
            break;

        case LV_EVENT_DRAG_END:
            Serial.println("LV_EVENT_DRAG_END");
            break;

        case LV_EVENT_DRAG_THROW_BEGIN:
            Serial.println("LV_EVENT_DRAG_THROW_BEGIN");
            break;

        case LV_EVENT_GESTURE:
            Serial.printf("LV_EVENT_GESTURE dir=%d\n", (int)lv_indev_get_gesture_dir(lv_indev_get_act()));
            break;

        case LV_EVENT_KEY:
            Serial.println("LV_EVENT_KEY");
            break;
        case LV_EVENT_FOCUSED:
            Serial.println("LV_EVENT_FOCUSED");
            break;
        case LV_EVENT_DEFOCUSED:
            Serial.println("LV_EVENT_DEFOCUSED");
            break;
        case LV_EVENT_LEAVE:
            Serial.println("LV_EVENT_LEAVE");
            break;
    }
}
#endif

class StatusBar
{
    typedef struct {
        bool vaild;
        lv_obj_t *icon;
    } lv_status_bar_t;
public:
    StatusBar()
    {
        memset(_array, 0, sizeof(_array));
    }
    void createIcons(lv_obj_t *par)
    {
        _par = par;

        static lv_style_t barStyle;
        lv_style_init(&barStyle);
        lv_style_set_radius(&barStyle, LV_OBJ_PART_MAIN, 0);
        lv_style_set_bg_color(&barStyle, LV_OBJ_PART_MAIN, LV_COLOR_GRAY);
        lv_style_set_bg_opa(&barStyle, LV_OBJ_PART_MAIN, LV_OPA_20);
        lv_style_set_border_width(&barStyle, LV_OBJ_PART_MAIN, 0);
        lv_style_set_text_color(&barStyle, LV_OBJ_PART_MAIN, LV_COLOR_WHITE);
        lv_style_set_image_recolor(&barStyle, LV_OBJ_PART_MAIN, LV_COLOR_WHITE);

        _bar = lv_cont_create(_par, NULL);
        lv_obj_set_size(_bar,  LV_HOR_RES, _barHeight);
        lv_obj_add_style(_bar, LV_OBJ_PART_MAIN, &barStyle);

        _array[0].icon = lv_label_create(_bar, NULL);
        lv_label_set_text(_array[0].icon, "100%");

        _array[1].icon = lv_img_create(_bar, NULL);
        lv_img_set_src(_array[1].icon, LV_SYMBOL_BATTERY_FULL);

        _array[2].icon = lv_img_create(_bar, NULL);
        lv_img_set_src(_array[2].icon, LV_SYMBOL_WIFI);
        lv_obj_set_hidden(_array[2].icon, true);

        _array[3].icon = lv_img_create(_bar, NULL);
        lv_img_set_src(_array[3].icon, LV_SYMBOL_BLUETOOTH);
        lv_obj_set_hidden(_array[3].icon, true);

        //step counter
        _array[4].icon = lv_img_create(_bar, NULL);
        lv_img_set_src(_array[4].icon, &step);
        lv_obj_align(_array[4].icon, _bar, LV_ALIGN_IN_LEFT_MID, 10, 0);

        _array[5].icon = lv_label_create(_bar, NULL);
        lv_label_set_text(_array[5].icon, "00000");
        lv_obj_align(_array[5].icon, _array[4].icon, LV_ALIGN_OUT_RIGHT_MID, 5, 0);

        _array[6].icon = lv_label_create(_bar, NULL);
        lv_label_set_text(_array[6].icon, THIS_VERSION_STR);
        lv_obj_align(_array[6].icon, _array[5].icon, LV_ALIGN_OUT_RIGHT_MID, 5, 0);
        
        refresh();
    }

    void setStepCounter(uint32_t counter)
    {
        lv_label_set_text(_array[5].icon, String(counter).c_str());
        lv_obj_align(_array[5].icon, _array[4].icon, LV_ALIGN_OUT_RIGHT_MID, 5, 0);
    }

    void updateLevel(int level)
    {
        lv_label_set_text(_array[0].icon, (String(level) + "%").c_str());
        refresh();
    }

    void updateBatteryIcon(lv_icon_battery_t icon)
    {
        const char *icons[6] = {LV_SYMBOL_BATTERY_EMPTY, LV_SYMBOL_BATTERY_1, LV_SYMBOL_BATTERY_2, LV_SYMBOL_BATTERY_3, LV_SYMBOL_BATTERY_FULL, LV_SYMBOL_CHARGE};
        lv_img_set_src(_array[1].icon, icons[icon]);
        refresh();
    }

    void show(lv_icon_status_bar_t icon)
    {
        lv_obj_set_hidden(_array[icon].icon, false);
        refresh();
    }

    void hidden(lv_icon_status_bar_t icon)
    {
        lv_obj_set_hidden(_array[icon].icon, true);
        refresh();
    }
    uint8_t height()
    {
        return _barHeight;
    }
    lv_obj_t *self()
    {
        return _bar;
    }
private:
    void refresh()
    {
        int prev;
        for (int i = 0; i < 4; i++) {
            if (!lv_obj_get_hidden(_array[i].icon)) {
                if (i == LV_STATUS_BAR_BATTERY_LEVEL) {
                    lv_obj_align(_array[i].icon, NULL, LV_ALIGN_IN_RIGHT_MID, 0, 0);
                } else {
                    lv_obj_align(_array[i].icon, _array[prev].icon, LV_ALIGN_OUT_LEFT_MID, iconOffset, 0);
                }
                prev = i;
            }
        }
    };
    lv_obj_t *_bar = nullptr;
    lv_obj_t *_par = nullptr;
    uint8_t _barHeight = 30;
    lv_status_bar_t _array[7];
    const int8_t iconOffset = -5;
};

class MenuBar
{
public:
    typedef struct {
        const char *name;
        void *img;
        void (*event_cb)();
    } lv_menu_config_t;

    MenuBar()
    {
        _cont = nullptr;
        _view = nullptr;
        _exit = nullptr;
        _obj = nullptr;
        _vp = nullptr;
    };
    ~MenuBar() {};

    void createMenu(lv_menu_config_t *config, int count, lv_event_cb_t event_cb, int direction = 1)
    {
        static lv_style_t menuStyle;
        lv_style_init(&menuStyle);
        lv_style_set_radius(&menuStyle, LV_OBJ_PART_MAIN, 0);
        lv_style_set_bg_color(&menuStyle, LV_OBJ_PART_MAIN, LV_COLOR_GRAY);
        lv_style_set_bg_opa(&menuStyle, LV_OBJ_PART_MAIN, LV_OPA_0);
        lv_style_set_border_width(&menuStyle, LV_OBJ_PART_MAIN, 0);
        lv_style_set_text_color(&menuStyle, LV_OBJ_PART_MAIN, LV_COLOR_WHITE);
        lv_style_set_image_recolor(&menuStyle, LV_OBJ_PART_MAIN, LV_COLOR_WHITE);

        _count = count;

        _vp = new lv_point_t [count];

        _obj = new lv_obj_t *[count];

        for (int i = 0; i < count; i++) {
            if (direction) {
                _vp[i].x = 0;
                _vp[i].y = i;
            } else {
                _vp[i].x = i;
                _vp[i].y = 0;
            }
        }

        _cont = lv_cont_create(lv_scr_act(), NULL);
        lv_obj_set_size(_cont, LV_HOR_RES, LV_VER_RES - 30);
        lv_obj_align(_cont, NULL, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
        lv_obj_add_style(_cont, LV_OBJ_PART_MAIN, &menuStyle);
        
        _view = lv_tileview_create(_cont, NULL);
        lv_tileview_set_valid_positions(_view, _vp, count );
        lv_tileview_set_edge_flash(_view, false);
        lv_obj_align(_view, NULL, LV_ALIGN_CENTER, 0, 0);
        lv_page_set_scrlbar_mode(_view, LV_SCRLBAR_MODE_OFF);
        lv_obj_add_style(_view, LV_OBJ_PART_MAIN, &menuStyle);

        lv_coord_t _w = lv_obj_get_width(_view) ;
        lv_coord_t _h = lv_obj_get_height(_view);

        for (int i = 0; i < count; i++) {
            _obj[i] = lv_cont_create(_view, _view);
            lv_obj_set_size(_obj[i], _w, _h);

            lv_obj_t *img = lv_img_create(_obj[i], NULL);
            lv_img_set_src(img, config[i].img);
            lv_obj_align(img, _obj[i], LV_ALIGN_CENTER, 0, 0);

            lv_obj_t *label = lv_label_create(_obj[i], NULL);
            lv_label_set_text(label, config[i].name);
            lv_obj_align(label, img, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);


            i == 0 ? lv_obj_align(_obj[i], NULL, LV_ALIGN_CENTER, 0, 0) : lv_obj_align(_obj[i], _obj[i - 1], direction ? LV_ALIGN_OUT_BOTTOM_MID : LV_ALIGN_OUT_RIGHT_MID, 0, 0);

            lv_tileview_add_element(_view, _obj[i]);
            lv_obj_set_click(_obj[i], true);
            lv_obj_set_event_cb(_obj[i], event_cb);
        }

        _exit  = lv_imgbtn_create(lv_scr_act(), NULL);
        lv_imgbtn_set_src(_exit, LV_BTN_STATE_RELEASED, &iexit);
        lv_imgbtn_set_src(_exit, LV_BTN_STATE_PRESSED, &iexit);
        lv_imgbtn_set_src(_exit, LV_BTN_STATE_CHECKED_PRESSED, &iexit);
        lv_imgbtn_set_src(_exit, LV_BTN_STATE_CHECKED_RELEASED, &iexit);
        lv_obj_align(_exit, NULL, LV_ALIGN_IN_BOTTOM_RIGHT, -20, -20);
        lv_obj_set_event_cb(_exit, event_cb);
        lv_obj_set_top(_exit, true);
    }
    lv_obj_t *exitBtn() const
    {
        return _exit;
    }
    lv_obj_t *self() const
    {
        return _cont;
    }
    void hidden(bool en = true)
    {
        lv_obj_set_hidden(_cont, en);
        lv_obj_set_hidden(_exit, en);
    }
    lv_obj_t *obj(int index) const
    {
        if (index > _count)return nullptr;
        return _obj[index];
    }
private:
    lv_obj_t *_cont, *_view, *_exit, * *_obj;
    lv_point_t *_vp ;
    int _count = 0;
};

MenuBar::lv_menu_config_t _cfg[4] = {
    {.name = "WiFi",  .img = (void *) &wifi, .event_cb = wifi_event_cb},
    {.name = "Bluetooth",  .img = (void *) &bluetooth, /*.event_cb = bluetooth_event_cb*/},
    {.name = "Settings",  .img = (void *) &setting, /*.event_cb = setting_event_cb */},
    {.name = "About",  .img = (void *) &modules, .event_cb = about_event_cb}
};


MenuBar menuBars;
StatusBar bar;

static void event_handler(lv_obj_t *obj, lv_event_t event)
{
    if (event == LV_EVENT_SHORT_CLICKED) {  //!  Event callback Is in here
        if (obj == menuBtn) {
            lv_obj_set_hidden(mainBar, true);
            if (menuBars.self() == nullptr) {
                menuBars.createMenu(_cfg, sizeof(_cfg) / sizeof(_cfg[0]), view_event_handler);
                lv_obj_align(menuBars.self(), bar.self(), LV_ALIGN_OUT_BOTTOM_MID, 0, 0);

            } else {
                menuBars.hidden(false);
            }
        }
    }
}

void setupGui()
{
    //lv_log_register_print_cb((lv_log_print_g_cb_t)my_log_cb);

    lv_obj_t *scr = lv_scr_act();
    
    lv_style_init(&settingStyle);
    lv_style_set_radius(&settingStyle, LV_OBJ_PART_MAIN, 0);
    lv_style_set_bg_color(&settingStyle, LV_OBJ_PART_MAIN, LV_COLOR_GRAY);
    lv_style_set_bg_opa(&settingStyle, LV_OBJ_PART_MAIN, LV_OPA_0);
    lv_style_set_border_width(&settingStyle, LV_OBJ_PART_MAIN, 0);
    lv_style_set_text_color(&settingStyle, LV_OBJ_PART_MAIN, LV_COLOR_WHITE);
    lv_style_set_image_recolor(&settingStyle, LV_OBJ_PART_MAIN, LV_COLOR_WHITE);
    
    /* 
     * Create the torch and ensure that it is at the back of all other widgets.
     * I tried creating a label and a container before finally stumbling upon
     * the positioning functionality I wanted with a button.
     */
    torchLabel = lv_btn_create (scr, NULL);
    static lv_style_t torchStyle;
    lv_style_copy(&torchStyle, &settingStyle);
    lv_style_set_bg_color(&torchStyle, LV_OBJ_PART_MAIN, LV_COLOR_WHITE);
    lv_style_set_bg_opa(&torchStyle, LV_OBJ_PART_MAIN, 255);
    lv_style_set_text_color(&torchStyle, LV_OBJ_PART_MAIN, LV_COLOR_RED);
    lv_obj_add_style(torchLabel, LV_OBJ_PART_MAIN, &torchStyle);
    lv_obj_set_size(torchLabel, LV_HOR_RES, LV_VER_RES - 30);
    lv_obj_move_background(torchLabel);
    lv_obj_set_pos(torchLabel, 0, 30);
    lv_obj_t *l = lv_label_create (torchLabel, NULL);

    /*
     * I have a suspicion that this scroll mode eats battery!
     */
    //lv_label_set_long_mode(l, LV_LABEL_LONG_SROLL_CIRC);    // LVGL spelling mistake!
    //lv_obj_set_width(l, 100);
    lv_label_set_text(l, "  TORCH MODE  ");
    lv_obj_align(l, NULL, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_user_data(torchLabel, l);
    /*
     * There seems to be an LVGL buglet that allows the scrolling label to bleed through
     * to the tile menu. Hiding it makes the ghost image disappear.
     */
    //lv_obj_set_hidden (l, true);

//Maybe I should just hide the torchLabel then I wouldn't have to jump through hoops?????

    //Create wallpaper
    void *images[] = {(void *) &bg, (void *) &bg1, (void *) &bg2, (void *) &bg3 };
    lv_obj_t *img_bin = lv_img_create(scr, NULL);  /*Create an image object*/
    srand((int)time(0));
    int r = rand() % 4;
    lv_img_set_src(img_bin, images[r]);
    lv_obj_align(img_bin, NULL, LV_ALIGN_CENTER, 0, 0);

    //! bar
    bar.createIcons(scr);
    updateBatteryLevel();
    lv_icon_battery_t icon = LV_ICON_CALCULATION;

    TTGOClass *ttgo = TTGOClass::getWatch();

    if (ttgo->power->isChargeing()) {
        icon = LV_ICON_CHARGE;
    }
    updateBatteryIcon(icon);

    //! main
    lv_style_init(&mainStyle);
    lv_style_set_radius(&mainStyle, LV_OBJ_PART_MAIN, 0);
    lv_style_set_bg_color(&mainStyle, LV_OBJ_PART_MAIN, LV_COLOR_GRAY);
    lv_style_set_bg_opa(&mainStyle, LV_OBJ_PART_MAIN, LV_OPA_0);
    lv_style_set_border_width(&mainStyle, LV_OBJ_PART_MAIN, 0);
    lv_style_set_text_color(&mainStyle, LV_OBJ_PART_MAIN, LV_COLOR_WHITE);
    lv_style_set_image_recolor(&mainStyle, LV_OBJ_PART_MAIN, LV_COLOR_WHITE);

    mainBar = lv_cont_create(scr, NULL);
    lv_obj_set_size(mainBar,  LV_HOR_RES, LV_VER_RES - bar.height());
    lv_obj_add_style(mainBar, LV_OBJ_PART_MAIN, &mainStyle);
    
    lv_obj_align(mainBar, bar.self(), LV_ALIGN_OUT_BOTTOM_MID, 0, 0);


    //! Time
    static lv_style_t timeStyle;
    static lv_style_t dateStyle;
    lv_style_copy(&timeStyle, &mainStyle);
    lv_style_set_text_font (&timeStyle, LV_STATE_DEFAULT, &Ubuntu);
    lv_style_copy(&dateStyle, &mainStyle);
    lv_style_set_text_letter_space (&timeStyle, LV_STATE_DEFAULT, -2);

    timeLabel = lv_label_create(mainBar, NULL);
    lv_obj_add_style(timeLabel, LV_OBJ_PART_MAIN, &timeStyle);
    dateLabel = lv_label_create(mainBar, NULL);
    lv_obj_add_style(dateLabel, LV_OBJ_PART_MAIN, &dateStyle);
    //lv_label_set_long_mode(dateLabel, LV_LABEL_LONG_SROLL_CIRC);
    updateTime();
    
#ifdef DEBUG_EVENTS    
    lv_obj_set_gesture_parent(mainBar,0);
    lv_obj_set_event_cb(mainBar, my_test_event_cb);
    //lv_obj_set_event_cb(lv_scr_act(), my_test_event_cb);
#endif

    //! menu
    static lv_style_t style_pr;
    lv_style_init(&style_pr);
    lv_style_set_image_recolor(&style_pr, LV_OBJ_PART_MAIN, LV_COLOR_BLACK);
    lv_style_set_text_color(&style_pr, LV_OBJ_PART_MAIN, lv_color_hex3(0xaaa));

    menuBtn = lv_imgbtn_create(mainBar, NULL);
    lv_imgbtn_set_src(menuBtn, LV_BTN_STATE_ACTIVE, &menu);
    lv_imgbtn_set_src(menuBtn, LV_BTN_STATE_RELEASED, &menu);
    lv_imgbtn_set_src(menuBtn, LV_BTN_STATE_PRESSED, &menu);
    lv_imgbtn_set_src(menuBtn, LV_BTN_STATE_CHECKED_RELEASED, &menu);
    lv_imgbtn_set_src(menuBtn, LV_BTN_STATE_CHECKED_PRESSED, &menu);
    lv_obj_add_style(menuBtn, LV_OBJ_PART_MAIN, &style_pr);
    
    lv_obj_align(menuBtn, mainBar, LV_ALIGN_OUT_BOTTOM_MID, 0, -70);
    lv_obj_set_event_cb(menuBtn, event_handler);

    lv_task_create(lv_update_task, 1000, LV_TASK_PRIO_LOWEST, NULL);
    lv_task_create(lv_battery_task, 30000, LV_TASK_PRIO_LOWEST, NULL);
}

void updateStepCounter(uint32_t counter)
{
    bar.setStepCounter(counter);
}

void updateTime()
{
    time_t now;
    struct tm  info;
    char buf[64];
    TTGOClass *ttgo = TTGOClass::getWatch();
    extern unsigned int screenTimeout;
    
    time(&now);
    localtime_r(&now, &info);
    strftime(buf, sizeof(buf), "%H:%M:%S", &info);
    lv_label_set_text(timeLabel, buf);
    lv_obj_align(timeLabel, NULL, LV_ALIGN_IN_TOP_MID, 0, 10);
    strftime(buf, sizeof(buf), "%a %d/%m/%Y", &info);
    lv_label_set_text (dateLabel, buf);
    lv_obj_align(dateLabel, NULL, LV_ALIGN_CENTER, 0, 0);

    if (screenTimeout == DEFAULT_SCREEN_TIMEOUT)
    {
      if ((info.tm_hour > 22) || (info.tm_hour < 8))
      {
        ttgo->setBrightness(8);
      }
      else
      {
        ttgo->setBrightness(64);
      }
    }
}

void torchOn ()
{
  //lv_obj_set_hidden ((lv_obj_t*)lv_obj_get_user_data (torchLabel), false);
  lv_obj_move_foreground(torchLabel);
}

void torchOff ()
{
  extern unsigned int screenTimeout;
  
  screenTimeout = DEFAULT_SCREEN_TIMEOUT;
  lv_obj_move_background(torchLabel);
  //lv_obj_set_hidden ((lv_obj_t*)lv_obj_get_user_data (torchLabel), true);
  updateTime ();
}

void updateBatteryLevel()
{
    TTGOClass *ttgo = TTGOClass::getWatch();
    int p = ttgo->power->getBattPercentage();
    bar.updateLevel(p);
}

void updateBatteryIcon(lv_icon_battery_t icon)
{
    if (icon <= LV_ICON_CALCULATION) {
        TTGOClass *ttgo = TTGOClass::getWatch();
        int level = ttgo->power->getBattPercentage();
        if (level > 95)icon = LV_ICON_BAT_FULL;
        else if (level > 65)icon = LV_ICON_BAT_3;
        else if (level > 40)icon = LV_ICON_BAT_2;
        else if (level > 10)icon = LV_ICON_BAT_1;
        else icon = LV_ICON_BAT_EMPTY;
    }
    bar.updateBatteryIcon(icon);
}


static void lv_update_task(struct _lv_task_t *data)
{
    updateTime();
}

static void lv_battery_task(struct _lv_task_t *data)
{
    updateBatteryLevel();
}

static void view_event_handler(lv_obj_t *obj, lv_event_t event)
{
    int size = sizeof(_cfg) / sizeof(_cfg[0]);
    if (event == LV_EVENT_SHORT_CLICKED) {
        if (obj == menuBars.exitBtn()) {
            menuBars.hidden();
            lv_obj_set_hidden(mainBar, false);
            return;
        }
        for (int i = 0; i < size; i++) {
            if (obj == menuBars.obj(i)) {
                if (_cfg[i].event_cb != nullptr) {
                    menuBars.hidden();
                    _cfg[i].event_cb();
                }
                return;
            }
        }
    }
}

/*****************************************************************
 *
 *          ! Keyboard Class
 *
 */

#define NEW_KBD

class Keyboard
{
public:
    typedef enum {
        KB_EVENT_OK,
        KB_EVENT_EXIT,
    } kb_event_t;

    typedef void (*kb_event_cb)(kb_event_t event);

    Keyboard()
    {
        _kbCont = nullptr;
    };

    ~Keyboard()
    {
        if (_kbCont)
            lv_obj_del(_kbCont);
        _kbCont = nullptr;
    };

    void create(lv_obj_t *parent =  nullptr)
    {
        static lv_style_t kbStyle;
        lv_style_init(&kbStyle);
        lv_style_set_radius(&kbStyle, LV_OBJ_PART_MAIN, 0);
        lv_style_set_bg_color(&kbStyle, LV_OBJ_PART_MAIN, LV_COLOR_GRAY);
        lv_style_set_bg_opa(&kbStyle, LV_OBJ_PART_MAIN, LV_OPA_0);
        lv_style_set_border_width(&kbStyle, LV_OBJ_PART_MAIN, 0);
        lv_style_set_text_color(&kbStyle, LV_OBJ_PART_MAIN, LV_COLOR_WHITE);
        lv_style_set_image_recolor(&kbStyle, LV_OBJ_PART_MAIN, LV_COLOR_WHITE);

        if (parent == nullptr) {
            parent = lv_scr_act();
        }

        _kbCont = lv_cont_create(parent, NULL);
        lv_obj_set_size(_kbCont, LV_HOR_RES, LV_VER_RES - 30);
#ifdef NEW_KBD
        lv_obj_set_pos(_kbCont, 0, 30);
#else
        lv_obj_align(_kbCont, NULL, LV_ALIGN_CENTER, 0, 0);
#endif  // NEW_KBD.
        lv_obj_add_style(_kbCont, LV_OBJ_PART_MAIN, &kbStyle);


#ifdef NEW_KBD
        _kbPage = lv_page_create(_kbCont, NULL);
        lv_page_set_scrlbar_mode(_kbPage, LV_SCROLLBAR_MODE_OFF);
        lv_obj_set_size (_kbPage, LV_HOR_RES, LV_VER_RES - 30 - 20);
        lv_obj_set_pos(_kbPage, 0, 20);
        lv_page_set_scrl_width(_kbPage,480);
        lv_page_set_scrl_height(_kbPage,190);
#endif  // NEW_KBD.

        lv_obj_t *ta = lv_textarea_create(_kbCont, NULL);
#ifdef NEW_KBD
        lv_obj_set_height(ta, 20);
        lv_obj_set_pos(ta, 0, 0);
#else
        lv_obj_set_height(ta, 40);
#endif // NEW_KDB.

        lv_textarea_set_one_line(ta, true);
        lv_textarea_set_pwd_mode(ta, false);
        lv_textarea_set_text(ta, "");

#ifdef NEW_KBD
        lv_obj_t *kb = lv_keyboard_create(_kbPage, NULL);
        lv_obj_set_pos(ta, 0, 0);
        lv_obj_set_height(kb, LV_VER_RES - 30 - 20);
        lv_obj_set_width(kb, 480);
        lv_obj_move_foreground (_kbCont);
	      lv_obj_set_pos (kb, 0, 0);
#else
        lv_obj_t *kb = lv_keyboard_create(_kbCont, NULL);
        lv_obj_align(ta, _kbCont, LV_ALIGN_IN_TOP_MID, 0, 10);
        lv_keyboard_set_map(kb, LV_KEYBOARD_MODE_TEXT_LOWER, btnm_mapplus[0]);
        lv_obj_set_height(kb, LV_VER_RES / 3 * 2);
        lv_obj_set_width(kb, 240);
        lv_obj_align(kb, NULL, LV_ALIGN_IN_BOTTOM_MID, 0, 0);
#endif  //NEW_KBD.

        lv_keyboard_set_textarea(kb, ta);

#ifdef NEW_KBD
        lv_obj_add_style(kb, LV_OBJ_PART_MAIN, &kbStyle);
#else
        lv_obj_add_style(ta, LV_OBJ_PART_MAIN, &kbStyle);
#endif  //NEW_KBD.
        lv_obj_set_x(lv_page_get_scrllable(_kbPage), 0);
        lv_obj_set_event_cb(kb, __kb_event_cb);

        _kb = this;
    }

    void align(const lv_obj_t *base, lv_align_t align, lv_coord_t x = 0, lv_coord_t y = 0)
    {
        lv_obj_align(_kbCont, base, align, x, y);
    }

    static void __kb_event_cb(lv_obj_t *kb, lv_event_t event)
    {
        if (event != LV_EVENT_VALUE_CHANGED && event != LV_EVENT_LONG_PRESSED_REPEAT) return;
        lv_keyboard_ext_t *ext = (lv_keyboard_ext_t *)lv_obj_get_ext_attr(kb);
        const char *txt = lv_btnmatrix_get_active_btn_text(kb);
        if (txt == NULL) return;
        static int index = 0;
        if ((strcmp(txt, LV_SYMBOL_OK) == 0) || (strcmp(txt, "Enter") == 0) || (strcmp(txt, LV_SYMBOL_NEW_LINE) == 0)){
            strcpy(__buf, lv_textarea_get_text(ext->ta));
            if (_kb->_cb != nullptr) {
                _kb->_cb(KB_EVENT_OK);
            }
            return;
        } else if ((LV_EVENT_CANCEL == event) || (strcmp(txt, LV_SYMBOL_CLOSE) == 0)) {
            if (_kb->_cb != nullptr) {
                _kb->_cb(KB_EVENT_EXIT);
            }
            return;
#ifdef NEW_KBD
        } else if (strcmp(txt, LV_SYMBOL_LEFT) == 0) {
            log_i("LV_SYMBOL_LEFT before=%d",lv_obj_get_x(lv_page_get_scrllable(_kb->_kbPage)));
            lv_page_scroll_hor(_kb->_kbPage, lv_obj_get_x(lv_page_get_scrllable(_kb->_kbPage)) - 240);
            delay(250);
            log_i ("LV_SYMBOL_LEFT after=%d", lv_obj_get_x(lv_page_get_scrllable(_kb->_kbPage)));
        } else if (strcmp(txt, LV_SYMBOL_RIGHT) == 0) {
            log_i("LV_SYMBOL_RIGHT before=%d", lv_obj_get_x(lv_page_get_scrllable(_kb->_kbPage)));
            lv_page_scroll_hor(_kb->_kbPage, lv_obj_get_x(lv_page_get_scrllable(_kb->_kbPage)) *-1);
            delay(250);
            log_i("LV_SYMBOL_RIGHT after=%d", lv_obj_get_x(lv_page_get_scrllable(_kb->_kbPage)));
        } else {
            lv_keyboard_def_event_cb(kb, event);
#else
        } else if (strcmp(txt, LV_SYMBOL_RIGHT) == 0) {
            index = index + 1 >= sizeof(btnm_mapplus) / sizeof(btnm_mapplus[0]) ? 0 : index + 1;
            lv_keyboard_set_map(kb, LV_KEYBOARD_MODE_TEXT_LOWER, btnm_mapplus[index]);
            return;
        } else if (strcmp(txt, "Del") == 0) {
            lv_textarea_del_char(ext->ta);
        } else {
            lv_textarea_add_text(ext->ta, txt);
#endif  // NEW_KBD.
        }
    }

    void setKeyboardEvent(kb_event_cb cb)
    {
        _cb = cb;
    }

    const char *getText()
    {
        return (const char *)__buf;
    }

    void hidden(bool en = true)
    {
        lv_obj_set_hidden(_kbCont, en);
    }

private:
#ifdef NEW_KBD
    lv_obj_t *_kbPage = nullptr;
#endif  // NEW_KBD.
    lv_obj_t *_kbCont = nullptr;
    kb_event_cb _cb = nullptr;
    static const char *btnm_mapplus[10][23];
    static Keyboard *_kb;
    static char __buf[128];
};
char Keyboard::__buf[128];
Keyboard *Keyboard::_kb = nullptr;
const char *Keyboard::btnm_mapplus[10][23] = {
    {
        "a", "b", "c",   "\n",
        "d", "e", "f",   "\n",
        "g", "h", "i",   "\n",
        LV_SYMBOL_OK, "Del", "Exit", LV_SYMBOL_RIGHT, ""
    },
    {
        "j", "k", "l", "\n",
        "n", "m", "o",  "\n",
        "p", "q", "r",  "\n",
        LV_SYMBOL_OK, "Del", "Exit", LV_SYMBOL_RIGHT, ""
    },
    {
        "s", "t", "u",   "\n",
        "v", "w", "x", "\n",
        "y", "z", " ", "\n",
        LV_SYMBOL_OK, "Del", "Exit", LV_SYMBOL_RIGHT, ""
    },
    {
        "A", "B", "C",  "\n",
        "D", "E", "F",   "\n",
        "G", "H", "I",  "\n",
        LV_SYMBOL_OK, "Del", "Exit", LV_SYMBOL_RIGHT, ""
    },
    {
        "J", "K", "L", "\n",
        "N", "M", "O",  "\n",
        "P", "Q", "R", "\n",
        LV_SYMBOL_OK, "Del", "Exit", LV_SYMBOL_RIGHT, ""
    },
    {
        "S", "T", "U",   "\n",
        "V", "W", "X",   "\n",
        "Y", "Z", " ", "\n",
        LV_SYMBOL_OK, "Del", "Exit", LV_SYMBOL_RIGHT, ""
    },
    {
        "1", "2", "3",  "\n",
        "4", "5", "6",  "\n",
        "7", "8", "9",  "\n",
        LV_SYMBOL_OK, "Del", "Exit", LV_SYMBOL_RIGHT, ""
    },
    {
        "0", "+", "-",  "\n",
        "/", "*", "=",  "\n",
        "!", "?", "#",  "\n",
        LV_SYMBOL_OK, "Del", "Exit", LV_SYMBOL_RIGHT, ""
    },
    {
        "<", ">", "@",  "\n",
        "%", "$", "(",  "\n",
        ")", "{", "}",  "\n",
        LV_SYMBOL_OK, "Del", "Exit", LV_SYMBOL_RIGHT, ""
    },
    {
        "[", "]", ";",  "\n",
        "\"", "'", ".", "\n",
        ",", ":",  " ", "\n",
        LV_SYMBOL_OK, "Del", "Exit", LV_SYMBOL_RIGHT, ""
    }
};


/*****************************************************************
 *
 *          ! Switch Class
 *
 */
class Switch
{
public:
    typedef struct {
        const char *name;
        void (*cb)(uint8_t, bool);
    } switch_cfg_t;

    typedef void (*exit_cb)();
    Switch()
    {
        _swCont = nullptr;
    }
    ~Switch()
    {
        if (_swCont)
            lv_obj_del(_swCont);
        _swCont = nullptr;
    }

    void create(switch_cfg_t *cfg, uint8_t count, exit_cb cb, lv_obj_t *parent = nullptr)
    {
        static lv_style_t swlStyle;
        lv_style_init(&swlStyle);
        lv_style_set_radius(&swlStyle, LV_OBJ_PART_MAIN, 0);
        lv_style_set_bg_color(&swlStyle, LV_OBJ_PART_MAIN, LV_COLOR_GRAY);
        lv_style_set_bg_opa(&swlStyle, LV_OBJ_PART_MAIN, LV_OPA_0);
        lv_style_set_border_width(&swlStyle, LV_OBJ_PART_MAIN, 0);
        lv_style_set_border_opa(&swlStyle, LV_OBJ_PART_MAIN, LV_OPA_50);
        lv_style_set_text_color(&swlStyle, LV_OBJ_PART_MAIN, LV_COLOR_WHITE);
        lv_style_set_image_recolor(&swlStyle, LV_OBJ_PART_MAIN, LV_COLOR_WHITE);

        if (parent == nullptr) {
            parent = lv_scr_act();
        }
        _exit_cb = cb;

        _swCont = lv_cont_create(parent, NULL);
        lv_obj_set_size(_swCont, LV_HOR_RES, LV_VER_RES - 30);
        lv_obj_align(_swCont, NULL, LV_ALIGN_CENTER, 0, 0);
        lv_obj_add_style(_swCont, LV_OBJ_PART_MAIN, &swlStyle);

        _count = count;
        _sw = new lv_obj_t *[count];
        _cfg = new switch_cfg_t [count];

        memcpy(_cfg, cfg, sizeof(switch_cfg_t) * count);

        lv_obj_t *prev = nullptr;
        for (int i = 0; i < count; i++) {
            lv_obj_t *la1 = lv_label_create(_swCont, NULL);
            lv_label_set_text(la1, cfg[i].name);
            i == 0 ? lv_obj_align(la1, NULL, LV_ALIGN_IN_TOP_LEFT, 30, 20) : lv_obj_align(la1, prev, LV_ALIGN_OUT_BOTTOM_MID, 0, 20);
            _sw[i] = lv_imgbtn_create(_swCont, NULL);
            lv_imgbtn_set_src(_sw[i], LV_BTN_STATE_ACTIVE, &off);
            lv_imgbtn_set_src(_sw[i], LV_BTN_STATE_RELEASED, &off);
            lv_imgbtn_set_src(_sw[i], LV_BTN_STATE_PRESSED, &off);
            lv_imgbtn_set_src(_sw[i], LV_BTN_STATE_CHECKED_RELEASED, &off);
            lv_imgbtn_set_src(_sw[i], LV_BTN_STATE_CHECKED_PRESSED, &off);
            lv_obj_set_click(_sw[i], true);
            lv_obj_align(_sw[i], la1, LV_ALIGN_OUT_RIGHT_MID, 80, 0);
            lv_obj_set_event_cb(_sw[i], __switch_event_cb);
            prev = la1;
        }

        _exitBtn = lv_imgbtn_create(_swCont, NULL);
        lv_imgbtn_set_src(_exitBtn, LV_BTN_STATE_ACTIVE, &iexit);
        lv_imgbtn_set_src(_exitBtn, LV_BTN_STATE_RELEASED, &iexit);
        lv_imgbtn_set_src(_exitBtn, LV_BTN_STATE_PRESSED, &iexit);
        lv_imgbtn_set_src(_exitBtn, LV_BTN_STATE_CHECKED_RELEASED, &iexit);
        lv_imgbtn_set_src(_exitBtn, LV_BTN_STATE_CHECKED_PRESSED, &iexit);
        lv_obj_set_click(_exitBtn, true);
        lv_obj_align(_exitBtn, _swCont, LV_ALIGN_IN_BOTTOM_MID, 0, -5);
        lv_obj_set_event_cb(_exitBtn, __switch_event_cb);

        _switch = this;
    }

    void align(const lv_obj_t *base, lv_align_t align, lv_coord_t x = 0, lv_coord_t y = 0)
    {
        lv_obj_align(_swCont, base, align, x, y);
    }

    void hidden(bool en = true)
    {
        lv_obj_set_hidden(_swCont, en);
    }

    static void __switch_event_cb(lv_obj_t *obj, lv_event_t event)
    {
        if (event == LV_EVENT_SHORT_CLICKED) {
            if (obj == _switch->_exitBtn) {
                if ( _switch->_exit_cb != nullptr) {
                    _switch->_exit_cb();
                    return;
                }
            }
        }

        if (event == LV_EVENT_SHORT_CLICKED) {
            for (int i = 0; i < _switch->_count ; i++) {
                lv_obj_t *sw = _switch->_sw[i];
                if (obj == sw) {
                    const void *src =  lv_imgbtn_get_src(sw, LV_BTN_STATE_RELEASED);
                    const void *dst = src == &off ? &on : &off;
                    bool en = src == &off;
                    lv_imgbtn_set_src(sw, LV_BTN_STATE_ACTIVE, dst);
                    lv_imgbtn_set_src(sw, LV_BTN_STATE_RELEASED, dst);
                    lv_imgbtn_set_src(sw, LV_BTN_STATE_PRESSED, dst);
                    lv_imgbtn_set_src(sw, LV_BTN_STATE_CHECKED_RELEASED, dst);
                    lv_imgbtn_set_src(sw, LV_BTN_STATE_CHECKED_PRESSED, dst);
                    if (_switch->_cfg[i].cb != nullptr) {
                        _switch->_cfg[i].cb(i, en);
                    }
                    return;
                }
            }
        }
    }

    void setStatus(uint8_t index, bool en)
    {
        if (index > _count)return;
        lv_obj_t *sw = _sw[index];
        const void *dst =  en ? &on : &off;
        lv_imgbtn_set_src(sw, LV_BTN_STATE_ACTIVE, dst);
        lv_imgbtn_set_src(sw, LV_BTN_STATE_RELEASED, dst);
        lv_imgbtn_set_src(sw, LV_BTN_STATE_PRESSED, dst);
        lv_imgbtn_set_src(sw, LV_BTN_STATE_CHECKED_RELEASED, dst);
        lv_imgbtn_set_src(sw, LV_BTN_STATE_CHECKED_PRESSED, dst);
    }

private:
    static Switch *_switch;
    lv_obj_t *_swCont = nullptr;
    uint8_t _count;
    lv_obj_t **_sw = nullptr;
    switch_cfg_t *_cfg = nullptr;
    lv_obj_t *_exitBtn = nullptr;
    exit_cb _exit_cb = nullptr;
};

Switch *Switch::_switch = nullptr;


/*****************************************************************
 *
 *          ! Preload Class
 *
 */
class Preload
{
public:
    Preload()
    {
        _preloadCont = nullptr;
    }
    ~Preload()
    {
        if (_preloadCont == nullptr) return;
        lv_obj_del(_preloadCont);
        _preloadCont = nullptr;
    }
    void create(lv_obj_t *parent = nullptr)
    {
        if (parent == nullptr) {
            parent = lv_scr_act();
        }
        if (_preloadCont == nullptr) {
            static lv_style_t plStyle;
            lv_style_init(&plStyle);
            lv_style_set_radius(&plStyle, LV_OBJ_PART_MAIN, 0);
            lv_style_set_bg_color(&plStyle, LV_OBJ_PART_MAIN, LV_COLOR_GRAY);
            lv_style_set_bg_opa(&plStyle, LV_OBJ_PART_MAIN, LV_OPA_0);
            lv_style_set_border_width(&plStyle, LV_OBJ_PART_MAIN, 0);
            lv_style_set_text_color(&plStyle, LV_OBJ_PART_MAIN, LV_COLOR_WHITE);
            lv_style_set_image_recolor(&plStyle, LV_OBJ_PART_MAIN, LV_COLOR_WHITE);


            static lv_style_t style;
            lv_style_init(&style);
            lv_style_set_radius(&style, LV_OBJ_PART_MAIN, 0);
            lv_style_set_bg_color(&style, LV_OBJ_PART_MAIN, LV_COLOR_GRAY);
            lv_style_set_bg_opa(&style, LV_OBJ_PART_MAIN, LV_OPA_0);
            lv_style_set_border_width(&style, LV_OBJ_PART_MAIN, 0);
            lv_style_set_text_color(&style, LV_OBJ_PART_MAIN, LV_COLOR_WHITE);
            lv_style_set_image_recolor(&style, LV_OBJ_PART_MAIN, LV_COLOR_WHITE);

            _preloadCont = lv_cont_create(parent, NULL);
            lv_obj_set_size(_preloadCont, LV_HOR_RES, LV_VER_RES - 30);
            lv_obj_align(_preloadCont, NULL, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
            lv_obj_add_style(_preloadCont, LV_OBJ_PART_MAIN, &plStyle);

            lv_obj_t *preload = lv_spinner_create(_preloadCont, NULL);
            lv_obj_set_size(preload, lv_obj_get_width(_preloadCont) / 2, lv_obj_get_height(_preloadCont) / 2);
            lv_obj_add_style(preload, LV_OBJ_PART_MAIN, &style);
            lv_obj_align(preload, _preloadCont, LV_ALIGN_CENTER, 0, 0);
        }
    }
    void align(const lv_obj_t *base, lv_align_t align, lv_coord_t x = 0, lv_coord_t y = 0)
    {
        lv_obj_align(_preloadCont, base, align, x, y);
    }

    void hidden(bool en = true)
    {
        lv_obj_set_hidden(_preloadCont, en);
    }

private:
    lv_obj_t *_preloadCont = nullptr;
};


/*****************************************************************
 *
 *          ! List Class
 *
 */

class List
{
public:
    typedef void(*list_event_cb)(const char *);
    List()
    {
    }
    ~List()
    {
        if (_listCont == nullptr) return;
        lv_obj_del(_listCont);
        _listCont = nullptr;
    }
    void create(lv_obj_t *parent = nullptr)
    {
        if (parent == nullptr) {
            parent = lv_scr_act();
        }
        if (_listCont == nullptr) {
            static lv_style_t listStyle;
            lv_style_init(&listStyle);
            lv_style_set_radius(&listStyle, LV_OBJ_PART_MAIN, 0);
            lv_style_set_bg_color(&listStyle, LV_OBJ_PART_MAIN, LV_COLOR_GRAY);
            lv_style_set_bg_opa(&listStyle, LV_OBJ_PART_MAIN, LV_OPA_0);
            lv_style_set_border_width(&listStyle, LV_OBJ_PART_MAIN, 0);
            lv_style_set_text_color(&listStyle, LV_OBJ_PART_MAIN, LV_COLOR_WHITE);
            lv_style_set_image_recolor(&listStyle, LV_OBJ_PART_MAIN, LV_COLOR_WHITE);

            _listCont = lv_list_create(lv_scr_act(), NULL);
            lv_list_set_scrollbar_mode(_listCont, LV_SCROLLBAR_MODE_OFF);
            lv_obj_set_size(_listCont, LV_HOR_RES, LV_VER_RES - 30);
            lv_obj_add_style(_listCont, LV_OBJ_PART_MAIN, &listStyle);
            lv_obj_align(_listCont, NULL, LV_ALIGN_CENTER, 0, 0);
        }
        _list = this;
    }

    void add(const char *txt, void *imgsrc = (void *)LV_SYMBOL_WIFI)
    {
        lv_obj_t *btn = lv_list_add_btn(_listCont, imgsrc, txt);
        lv_obj_set_event_cb(btn, __list_event_cb);
    }

    void align(const lv_obj_t *base, lv_align_t align, lv_coord_t x = 0, lv_coord_t y = 0)
    {
        lv_obj_align(_listCont, base, align, x, y);
    }

    void hidden(bool en = true)
    {
        lv_obj_set_hidden(_listCont, en);
    }

    static void __list_event_cb(lv_obj_t *obj, lv_event_t event)
    {
        if (event == LV_EVENT_SHORT_CLICKED) {
            const char *txt = lv_list_get_btn_text(obj);
            if (_list->_cb != nullptr) {
                _list->_cb(txt);
            }
        }
    }
    void setListCb(list_event_cb cb)
    {
        _cb = cb;
    }
private:
    lv_obj_t *_listCont = nullptr;
    static List *_list ;
    list_event_cb _cb = nullptr;
};
List *List::_list = nullptr;

/*****************************************************************
 *
 *          ! Task Class
 *
 */
class Task
{
public:
    Task()
    {
        _handler = nullptr;
        _cb = nullptr;
    }
    ~Task()
    {
        if ( _handler == nullptr)return;
        Serial.println("Free Task Func");
        lv_task_del(_handler);
        _handler = nullptr;
        _cb = nullptr;
    }

    void create(lv_task_cb_t cb, uint32_t period = 1000, lv_task_prio_t prio = LV_TASK_PRIO_LOW)
    {
        _handler = lv_task_create(cb,  period,  prio, NULL);
    };

private:
    lv_task_t *_handler = nullptr;
    lv_task_cb_t _cb = nullptr;
};

/*****************************************************************
 *
 *          ! MesBox Class
 *
 */

class MBox
{
public:
    MBox()
    {
        _mbox = nullptr;
    }
    ~MBox()
    {
        if (_mbox == nullptr)return;
        lv_obj_del(_mbox);
        _mbox = nullptr;
    }

    void create(const char *text, lv_event_cb_t event_cb, const char **btns = nullptr, lv_obj_t *par = nullptr)
    {
        if (_mbox != nullptr)return;
        lv_obj_t *p = par == nullptr ? lv_scr_act() : par;
        _mbox = lv_msgbox_create(p, NULL);
        lv_msgbox_set_text(_mbox, text);
        if (btns == nullptr) {
            static const char *defBtns[] = {"Ok", ""};
            lv_msgbox_add_btns(_mbox, defBtns);
        } else {
            lv_msgbox_add_btns(_mbox, btns);
        }
        lv_obj_set_width(_mbox, LV_HOR_RES - 40);
        lv_obj_set_event_cb(_mbox, event_cb);
        lv_obj_align(_mbox, NULL, LV_ALIGN_CENTER, 0, 0);
    }

    void setData(void *data)
    {
        lv_obj_set_user_data(_mbox, data);
    }

    void *getData()
    {
        return lv_obj_get_user_data(_mbox);
    }

    void setBtn(const char **btns)
    {
        lv_msgbox_add_btns(_mbox, btns);
    }

private:
    lv_obj_t *_mbox = nullptr;
};




/*****************************************************************
 *
 *          ! GLOBAL VALUE
 *
 */
static Keyboard *kb = nullptr;
static Switch *sw = nullptr;
static Preload *pl = nullptr;
static List *list = nullptr;
static Task *task = nullptr;
static Ticker *gTicker = nullptr;
static MBox *mbox = nullptr;

static char ssid[64], password[64];

/*****************************************************************
 *
 *          !WIFI EVENT
 *
 */
void wifi_connect_status(bool result)
{
    if (gTicker != nullptr) {
        delete gTicker;
        gTicker = nullptr;
    }
    if (kb != nullptr) {
        delete kb;
        kb = nullptr;
    }
    if (sw != nullptr) {
        delete sw;
        sw = nullptr;
    }
    if (pl != nullptr) {
        delete pl;
        pl = nullptr;
    }
    if (result) {
        bar.show(LV_STATUS_BAR_WIFI);
    } else {
        bar.hidden(LV_STATUS_BAR_WIFI);
    }
    menuBars.hidden(false);
}


void wifi_kb_event_cb(Keyboard::kb_event_t event)
{
    if (event == 0) {
        kb->hidden();
        Serial.println(kb->getText());
        strlcpy(password, kb->getText(), sizeof(password));
        pl->hidden(false);
        WiFi.mode(WIFI_STA);
        WiFi.disconnect();
        WiFi.begin(ssid, password);
        gTicker = new Ticker;
        gTicker->once_ms(5 * 1000, []() {
            wifi_connect_status(false);
        });
    } else if (event == 1) {
        delete kb;
        delete sw;
        delete pl;
        pl = nullptr;
        kb = nullptr;
        sw = nullptr;
        menuBars.hidden(false);
    }
}

static void wifi_sync_mbox_cb(lv_task_t *t)
{
    static  struct tm timeinfo;
    bool ret = false;
    static int retry = 0;
    configTzTime(RTC_TIME_ZONE, "pool.ntp.org");
    while (1) {
        ret = getLocalTime(&timeinfo);
        if (!ret) {
            Serial.printf("get ntp fail,retry : %d \n", retry++);
        } else {
            //! del preload
            delete pl;
            pl = nullptr;

            char format[256];
            snprintf(format, sizeof(format), "Time acquisition is:%d-%d-%d/%d:%d:%d, Whether to synchronize?", timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
            Serial.println(format);
            delete task;
            task = nullptr;

            //! mbox
            static const char *btns[] = {"Ok", "Cancel", ""};
            mbox = new MBox;
            mbox->create(format, [](lv_obj_t *obj, lv_event_t event) {
                if (event == LV_EVENT_VALUE_CHANGED) {
                    const char *txt =  lv_msgbox_get_active_btn_text(obj);
                    if (!strcmp(txt, "Ok")) {

                        //!sync to rtc
                        struct tm *info =  (struct tm *)mbox->getData();
                        Serial.printf("read use data = %d:%d:%d - %d:%d:%d \n", info->tm_year + 1900, info->tm_mon + 1, info->tm_mday, info->tm_hour, info->tm_min, info->tm_sec);

                        TTGOClass *ttgo = TTGOClass::getWatch();
                        ttgo->rtc->setDateTime(info->tm_year + 1900, info->tm_mon + 1, info->tm_mday, info->tm_hour, info->tm_min, info->tm_sec);
                    } else if (!strcmp(txt, "Cancel")) {
                        //!cancel
                        // Serial.println("Cancel press");
                    }
                    delete mbox;
                    mbox = nullptr;
                    sw->hidden(false);
                }
            });
            mbox->setBtn(btns);
            mbox->setData(&timeinfo);
            return;
        }
    }
}

void wifi_sw_event_cb(uint8_t index, bool en)
{
    switch (index) {
    case 0:
        if (en) {
            setCpuFrequencyMhz(CPU_FREQ_WIFI);
            WiFi.begin();
        } else {
            WiFi.disconnect();
            bar.hidden(LV_STATUS_BAR_WIFI);
        }
        break;
    case 1:
        sw->hidden();
        pl = new Preload;
        pl->create();
        pl->align(bar.self(), LV_ALIGN_OUT_BOTTOM_MID);
        WiFi.disconnect();
        WiFi.scanNetworks(true);
        break;
    case 2:
        if (!WiFi.isConnected()) {
            //TODO pop-up window
            Serial.println("WiFi is not connected");
            return;
        } else {
            if (task != nullptr) {
                Serial.println("task is running ...");
                return;
            }
            task = new Task;
            task->create(wifi_sync_mbox_cb);
            sw->hidden();
            pl = new Preload;
            pl->create();
            pl->align(bar.self(), LV_ALIGN_OUT_BOTTOM_MID);
        }
        break;
    default:
        break;
    }
}

void wifi_list_cb(const char *txt)
{
    strlcpy(ssid, txt, sizeof(ssid));
    delete list;
    list = nullptr;
    kb = new Keyboard;
    kb->create();
    kb->align(bar.self(), LV_ALIGN_OUT_BOTTOM_MID);
    kb->setKeyboardEvent(wifi_kb_event_cb);
}

void wifi_list_add(const char *ssid)
{
    if (list == nullptr) {
        pl->hidden();
        list = new List;
        list->create();
        list->align(bar.self(), LV_ALIGN_OUT_BOTTOM_MID);
        list->setListCb(wifi_list_cb);
    }
    list->add(ssid);
}


static void wifi_event_cb()
{
    Switch::switch_cfg_t cfg[3] = {{"Switch", wifi_sw_event_cb}, {"Scan", wifi_sw_event_cb}, {"NTP Sync", wifi_sw_event_cb}};
    sw = new Switch;
    sw->create(cfg, 3, []() {
        delete sw;
        sw = nullptr;
        menuBars.hidden(false);
    });
    sw->align(bar.self(), LV_ALIGN_OUT_BOTTOM_MID);
    sw->setStatus(0, WiFi.isConnected());
}


static void wifi_destory()
{
    Serial.printf("globalIndex:%d\n", globalIndex);
    switch (globalIndex) {
    //! wifi management main
    case 0:
        menuBars.hidden(false);
        delete sw;
        sw = nullptr;
        break;
    //! wifi ap list
    case 1:
        if (list != nullptr) {
            delete list;
            list = nullptr;
        }
        if (gTicker != nullptr) {
            delete gTicker;
            gTicker = nullptr;
        }
        if (kb != nullptr) {
            delete kb;
            kb = nullptr;
        }
        if (pl != nullptr) {
            delete pl;
            pl = nullptr;
        }
        sw->hidden(false);
        break;
    //! wifi keyboard
    case 2:
        if (gTicker != nullptr) {
            delete gTicker;
            gTicker = nullptr;
        }
        if (kb != nullptr) {
            delete kb;
            kb = nullptr;
        }
        if (pl != nullptr) {
            delete pl;
            pl = nullptr;
        }
        sw->hidden(false);
        break;
    case 3:
        break;
    default:
        break;
    }
    globalIndex--;
}


/*****************************************************************
 *
 *          !SETTING EVENT
 *
 */
static void setting_event_cb()
{


}

/*****************************************************************
 *
 *          ! LIGHT EVENT
 *

static void light_sw_event_cb(uint8_t index, bool en)
{
    //Add lights that need to be controlled
}

static void light_event_cb()
{
    const uint8_t cfg_count = 4;
    Switch::switch_cfg_t cfg[cfg_count] = {
        {"light1", light_sw_event_cb},
        {"light2", light_sw_event_cb},
        {"light3", light_sw_event_cb},
        {"light4", light_sw_event_cb},
    };
    sw = new Switch;
    sw->create(cfg, cfg_count, []() {
        delete sw;
        sw = nullptr;
        menuBars.hidden(false);
    });

    sw->align(bar.self(), LV_ALIGN_OUT_BOTTOM_MID);

    //Initialize switch status
    for (int i = 0; i < cfg_count; i++) {
        sw->setStatus(i, 0);
    }
}
*/

/*****************************************************************
 *
 *          ! MBOX EVENT
 *
 */
static lv_obj_t *mbox1 = nullptr;

static void create_mbox(const char *txt, lv_event_cb_t event_cb)
{
    if (mbox1 != nullptr)return;
    static const char *btns[] = {"Ok", ""};
    mbox1 = lv_msgbox_create(lv_scr_act(), NULL);
    lv_msgbox_set_text(mbox1, txt);
    lv_msgbox_add_btns(mbox1, btns);
    lv_obj_set_width(mbox1, LV_HOR_RES - 40);
    lv_obj_set_event_cb(mbox1, event_cb);
    lv_obj_align(mbox1, NULL, LV_ALIGN_CENTER, 0, 0);
}

static void destory_mbox()
{
    if (pl != nullptr) {
        delete pl;
        pl = nullptr;
    }
    if (list != nullptr) {
        delete list;
        list = nullptr;
    }
    if (mbox1 != nullptr) {
        lv_obj_del(mbox1);
        mbox1 = nullptr;
    }
}

/*****************************************************************
 *
 *          About EVENT
 *
 * This is an experiment trying to use the table widget - it isn't really suitable
 * Need to develop a more generic widget container that combines List, Table and 
 * ButtonMatrix widgets - probably based upon the Switch and List classes in this
 * gui.cpp file.
 */

static lv_obj_t *about = nullptr;

static void exit_about(lv_obj_t *obj, lv_event_t event)
{
  if (event == LV_EVENT_SHORT_CLICKED)
  {
    lv_obj_set_hidden(about, true);

    menuBars.hidden(false);
  }
}

static void about_event_cb()
{ 
  static lv_obj_t *_label = nullptr;

  if (about == nullptr)
  {
    lv_obj_t *_exitBtn = nullptr;
    static lv_style_t barStyle;
    
    lv_style_init(&barStyle);
    lv_style_set_radius(&barStyle, LV_OBJ_PART_MAIN, 0);
    lv_style_set_bg_color(&barStyle, LV_OBJ_PART_MAIN, LV_COLOR_GRAY);
    lv_style_set_bg_opa(&barStyle, LV_OBJ_PART_MAIN, LV_OPA_20);
    lv_style_set_border_width(&barStyle, LV_OBJ_PART_MAIN, 0);
    lv_style_set_text_color(&barStyle, LV_OBJ_PART_MAIN, LV_COLOR_WHITE);
    lv_style_set_image_recolor(&barStyle, LV_OBJ_PART_MAIN, LV_COLOR_WHITE);
     
    about = lv_cont_create (lv_scr_act(), NULL);
    lv_obj_set_size (about, LV_HOR_RES, LV_VER_RES - 30);
    lv_obj_set_pos (about, 0, 30);
    lv_obj_add_style (about, LV_OBJ_PART_MAIN, &barStyle);

    _exitBtn = lv_imgbtn_create (about, NULL);
    lv_imgbtn_set_src(_exitBtn, LV_BTN_STATE_ACTIVE, &iexit);
    lv_imgbtn_set_src(_exitBtn, LV_BTN_STATE_RELEASED, &iexit);
    lv_imgbtn_set_src(_exitBtn, LV_BTN_STATE_PRESSED, &iexit);
    lv_imgbtn_set_src(_exitBtn, LV_BTN_STATE_CHECKED_RELEASED, &iexit);
    lv_imgbtn_set_src(_exitBtn, LV_BTN_STATE_CHECKED_PRESSED, &iexit);
    lv_obj_set_click(_exitBtn, true);
    lv_obj_align(_exitBtn, about, LV_ALIGN_IN_BOTTOM_MID, 0, 0);
    lv_obj_set_event_cb(_exitBtn, exit_about);

    _label = lv_label_create (about, NULL);
    lv_obj_add_style(_label, LV_OBJ_PART_MAIN, &barStyle);
  }
  else
  {
    lv_obj_set_hidden(about, false);
  }
  
  lv_label_set_text_fmt (_label, "\nagoodWatch %s\n(C) copyright Alex Goodyear\n%s\n\nCPU speed=%dMHz\nFree mem=%d",
                           THIS_VERSION_STR, __DATE__, getCpuFrequencyMhz(), esp_get_free_heap_size());
  lv_obj_align(_label, about, LV_ALIGN_IN_TOP_MID, 0, 0);
}
