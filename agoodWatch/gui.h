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

#define THIS_VERSION_ID  0.4
#define THIS_VERSION_STR "Ver " STR(THIS_VERSION_ID)

/*
 * Use a time-zone string from https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
 * Courtesy of Andreas Spiess NTP example https://github.com/SensorsIot and his YouTube channel
 * https://www.youtube.com/channel/UCu7_D0o48KbfhpEohoP7YSQ
 * 
 * Also note that the NTP update may not update the time the first time so ensure that the time
 * reported by the watch is correct before pressing "Ok" otherwise just press "Cancel" and try again.
 */
#define RTC_TIME_ZONE   "GMT0BST,M3.5.0/1,M10.5.0"  // Europe-London

/*
 * The number of milliseconds of inactivity before the watch goes to sleep. Every tap or swipe on the
 * screen will reset the internal timer.
 */
#define DEFAULT_SCREEN_TIMEOUT  7*1000    // Was 30* - Should reduce battery consumption.

#define CPU_FREQ_MIN     10
#define CPU_FREQ_NORM    80
#define CPU_FREQ_WIFI    80
#define CPU_FREQ_MEDIUM 160
#define CPU_FREQ_MAX    240

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
