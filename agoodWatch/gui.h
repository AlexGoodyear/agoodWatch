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

#ifndef __GUI_H
#define __GUI_H

#define STR(_s) XSTR(_s)
#define XSTR(_s) #_s

#define THIS_VERSION_ID  0.3
#define THIS_VERSION_STR "Ver " STR(THIS_VERSION_ID)

#define DEFAULT_SCREEN_TIMEOUT  6*1000    //Was 30* - Should reduce battery consumption.

typedef enum {
    LV_ICON_BAT_EMPTY,
    LV_ICON_BAT_1,
    LV_ICON_BAT_2,
    LV_ICON_BAT_3,
    LV_ICON_BAT_FULL,
    LV_ICON_CHARGE,
    LV_ICON_CALCULATION = LV_ICON_BAT_FULL
} lv_icon_battery_t;


typedef enum {
    LV_STATUS_BAR_BATTERY_LEVEL = 0,
    LV_STATUS_BAR_BATTERY_ICON = 1,
    LV_STATUS_BAR_WIFI = 2,
    LV_STATUS_BAR_BLUETOOTH = 3,
} lv_icon_status_bar_t;

void setupGui();
void updateStepCounter(uint32_t counter);
void updateBatteryIcon(lv_icon_battery_t index);
void wifi_list_add(const char *ssid);
void wifi_connect_status(bool result);
void updateBatteryLevel();
void updateTime();
void torchOn();
void torchOff();

#endif /*__GUI_H */
