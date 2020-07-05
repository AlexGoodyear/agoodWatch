/*
 * Copyright (c) 2020 Alex Goodyear
 * 
 * agoodWatch will always work with my fork of the LilyGo TTGO_TWatch_Library at
 * https://github.com/AlexGoodyear/TTGO_TWatch_Library
 * 
 * Derived from the SimpleWatch example in https://github.com/Xinyuan-LilyGO/TTGO_TWatch_Library
 * 
 * Original copyright below ...
Copyright (c) 2019 lewis he
This is just a demonstration. Most of the functions are not implemented.
The main implementation is low-power standby. 
The off-screen standby (not deep sleep) current is about 4mA.
Select standard motherboard and standard backplane for testing.
Created by Lewis he on October 10, 2019.
*/
 
#include "config.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/queue.h"
#include <soc/rtc.h>
#include "esp_wifi.h"
#include <WiFi.h>
#include "gui.h"

//#define DEBUG_SERIAL_OUTPUT  1

#ifdef DEBUG_SERIAL_OUTPUT
#define DSERIAL(_func, ...) Serial._func (__VA_ARGS__)
#else
#define DSERIAL(_func, ...)
#endif

#define G_EVENT_VBUS_PLUGIN         _BV(0)
#define G_EVENT_VBUS_REMOVE         _BV(1)
#define G_EVENT_CHARGE_DONE         _BV(2)

#define G_EVENT_WIFI_SCAN_START     _BV(3)
#define G_EVENT_WIFI_SCAN_DONE      _BV(4)
#define G_EVENT_WIFI_CONNECTED      _BV(5)
#define G_EVENT_WIFI_BEGIN          _BV(6)
#define G_EVENT_WIFI_OFF            _BV(7)

enum {
    Q_EVENT_WIFI_SCAN_DONE,
    Q_EVENT_WIFI_CONNECT,
    Q_EVENT_BMA_INT,
    Q_EVENT_AXP_INT,
} ;

#define WATCH_FLAG_SLEEP_MODE   _BV(1)
#define WATCH_FLAG_SLEEP_EXIT   _BV(2)
#define WATCH_FLAG_BMA_IRQ      _BV(3)
#define WATCH_FLAG_AXP_IRQ      _BV(4)

QueueHandle_t g_event_queue_handle = NULL;
EventGroupHandle_t g_event_group = NULL;
EventGroupHandle_t isr_group = NULL;
bool lenergy = false;
TTGOClass *ttgo;
lv_icon_battery_t batState = LV_ICON_CALCULATION;
unsigned int screenTimeout = DEFAULT_SCREEN_TIMEOUT;

void setupNetwork()
{
    WiFi.mode(WIFI_STA);
    WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
        xEventGroupClearBits(g_event_group, G_EVENT_WIFI_CONNECTED);
    }, WiFiEvent_t::SYSTEM_EVENT_STA_DISCONNECTED);

    WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
        uint8_t data = Q_EVENT_WIFI_SCAN_DONE;
        xQueueSend(g_event_queue_handle, &data, portMAX_DELAY);
    }, WiFiEvent_t::SYSTEM_EVENT_SCAN_DONE);

    WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
        xEventGroupSetBits(g_event_group, G_EVENT_WIFI_CONNECTED);
    }, WiFiEvent_t::SYSTEM_EVENT_STA_CONNECTED);

    WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
        wifi_connect_status(true);
    }, WiFiEvent_t::SYSTEM_EVENT_STA_GOT_IP);
}

void low_energy()
{
    if (ttgo->bl->isOn()) {
        DSERIAL(println, "low_energy() - BL is on");
        xEventGroupSetBits(isr_group, WATCH_FLAG_SLEEP_MODE);
        
        if (screenTimeout > DEFAULT_SCREEN_TIMEOUT)
        {
          torchOff();
        }

        ttgo->closeBL();
        ttgo->stopLvglTick();
        ttgo->bma->enableStepCountInterrupt(false);
        ttgo->displaySleep();

        if (!WiFi.isConnected()) {
            DSERIAL(println, "low_energy() - WiFi is off entering 2MHz mode");
            delay(250);
            lenergy = true;
            WiFi.mode(WIFI_OFF);
            rtc_clk_cpu_freq_set(RTC_CPU_FREQ_2M);
            gpio_wakeup_enable ((gpio_num_t)AXP202_INT, GPIO_INTR_LOW_LEVEL);
            gpio_wakeup_enable ((gpio_num_t)BMA423_INT1, GPIO_INTR_HIGH_LEVEL);
            esp_sleep_enable_gpio_wakeup ();
            esp_light_sleep_start();
        }
    } else {
        DSERIAL(println, "low_energy() - BL is off");
        ttgo->startLvglTick();
        ttgo->displayWakeup();
        ttgo->rtc->syncToSystem();
        updateStepCounter(ttgo->bma->getCounter());
        updateBatteryLevel();
        updateBatteryIcon(batState);
        updateTime();
        lv_disp_trig_activity(NULL);
        ttgo->openBL();
        ttgo->bma->enableStepCountInterrupt(true);
        screenTimeout = DEFAULT_SCREEN_TIMEOUT;

    }
}

void setup()
{
    Serial.begin (115200);

    Serial.println (THIS_VERSION_STR);
    
    //Create a program that allows the required message objects and group flags
    g_event_queue_handle = xQueueCreate(20, sizeof(uint8_t));
    g_event_group = xEventGroupCreate();
    isr_group = xEventGroupCreate();

    ttgo = TTGOClass::getWatch();

    //Initialize TWatch
    ttgo->begin();

    // Turn on the IRQ used
    ttgo->power->adc1Enable(AXP202_BATT_VOL_ADC1 | AXP202_BATT_CUR_ADC1 | AXP202_VBUS_VOL_ADC1 | AXP202_VBUS_CUR_ADC1, AXP202_ON);
    ttgo->power->enableIRQ(AXP202_VBUS_REMOVED_IRQ | AXP202_VBUS_CONNECT_IRQ | AXP202_CHARGING_FINISHED_IRQ, AXP202_ON);
    ttgo->power->clearIRQ();

    // Turn off unused power
    ttgo->power->setPowerOutPut(AXP202_EXTEN, AXP202_OFF);
    ttgo->power->setPowerOutPut(AXP202_DCDC2, AXP202_OFF);
    ttgo->power->setPowerOutPut(AXP202_LDO3, AXP202_OFF);
    ttgo->power->setPowerOutPut(AXP202_LDO4, AXP202_OFF);

    //Initialize lvgl
    ttgo->lvgl_begin();

    //Initialize bma423
    ttgo->bma->begin();

    //Enable BMA423 interrupt
    ttgo->bma->attachInterrupt();

    //Connection interrupted to the specified pin
    pinMode(BMA423_INT1, INPUT);
    attachInterrupt(BMA423_INT1, [] {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        EventBits_t  bits = xEventGroupGetBitsFromISR(isr_group);
        if (bits & WATCH_FLAG_SLEEP_MODE)
        {
            // Use an XEvent when waking from low energy sleep mode.
            xEventGroupSetBitsFromISR(isr_group, WATCH_FLAG_SLEEP_EXIT | WATCH_FLAG_BMA_IRQ, &xHigherPriorityTaskWoken);
        } else
        {
            // Use the XQueue mechanism when we are already awake.
            uint8_t data = Q_EVENT_BMA_INT;
            xQueueSendFromISR(g_event_queue_handle, &data, &xHigherPriorityTaskWoken);
        }

        if (xHigherPriorityTaskWoken)
        {
            portYIELD_FROM_ISR ();
        }
    }, RISING);

    // Connection interrupted to the specified pin
    pinMode(AXP202_INT, INPUT);
    attachInterrupt(AXP202_INT, [] {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        EventBits_t  bits = xEventGroupGetBitsFromISR(isr_group);
        if (bits & WATCH_FLAG_SLEEP_MODE)
        {
            // Use an XEvent when waking from low energy sleep mode.
            xEventGroupSetBitsFromISR(isr_group, WATCH_FLAG_SLEEP_EXIT | WATCH_FLAG_AXP_IRQ, &xHigherPriorityTaskWoken);
        } else
        {
            // Use the XQueue mechanism when we are already awake.
            uint8_t data = Q_EVENT_AXP_INT;
            xQueueSendFromISR(g_event_queue_handle, &data, &xHigherPriorityTaskWoken);
        }
        if (xHigherPriorityTaskWoken)
        {
            portYIELD_FROM_ISR ();
        }
    }, FALLING);

    //Check if the RTC clock matches, if not, use compile time
    ttgo->rtc->check();

    //Synchronize time to system time
    ttgo->rtc->syncToSystem();

    //Setting up the network
    setupNetwork();

    //Execute your own GUI interface
    setupGui();

    //Clear lvgl counter
    lv_disp_trig_activity(NULL);

    //When the initialization is complete, turn on the backlight
    ttgo->openBL();

    /*
     * Setup the axes for the TWatch-2020-V1 for the tilt detection.
     */
    struct bma423_axes_remap remap_data;

    remap_data.x_axis = 0;
    remap_data.x_axis_sign = 1;
    remap_data.y_axis = 1;
    remap_data.y_axis_sign = 0;
    remap_data.z_axis  = 2;
    remap_data.z_axis_sign  = 1;

    ttgo->bma->set_remap_axes(&remap_data);

    /*
     * Enable the double tap wakeup.
     */
    ttgo->bma->enableWakeupInterrupt (true);
}

void loop()
{
    bool  rlst;
    uint8_t data;
    static uint32_t start = 0;

    // An XEvent signifies that there has been a wakeup interrupt, bring the CPU out of low energy mode
    EventBits_t  bits = xEventGroupGetBits(isr_group);
    if (bits & WATCH_FLAG_SLEEP_EXIT) {
        if (lenergy) {
            lenergy = false;
            rtc_clk_cpu_freq_set(RTC_CPU_FREQ_160M);
        }

        low_energy();

        if (bits & WATCH_FLAG_BMA_IRQ) {
          DSERIAL(println, "WATCH_FLAG_BMA_IRQ");
            do {
                rlst =  ttgo->bma->readInterrupt();
            } while (!rlst);
            xEventGroupClearBits(isr_group, WATCH_FLAG_BMA_IRQ);
        }
        if (bits & WATCH_FLAG_AXP_IRQ) {
          DSERIAL(println, "WATCH_FLAG_AXP_IRQ");
            ttgo->power->readIRQ();
            ttgo->power->clearIRQ();
            xEventGroupClearBits(isr_group, WATCH_FLAG_AXP_IRQ);
        }
        xEventGroupClearBits(isr_group, WATCH_FLAG_SLEEP_EXIT);
        xEventGroupClearBits(isr_group, WATCH_FLAG_SLEEP_MODE);
    }
    if ((bits & WATCH_FLAG_SLEEP_MODE)) {
        //! No event processing after entering the information screen
        return;
    }

    //! Normal polling
    if (xQueueReceive(g_event_queue_handle, &data, 5 / portTICK_RATE_MS) == pdPASS) {
        switch (data) {
        case Q_EVENT_BMA_INT:
          DSERIAL(println, "Q_EVENT_BMA_IRQ");

            do {
                rlst =  ttgo->bma->readInterrupt();
            } while (!rlst);

            if (ttgo->bma->isDoubleClick()) {
              if (screenTimeout == DEFAULT_SCREEN_TIMEOUT)
              {
                screenTimeout--;
                ttgo->setBrightness(255);
              }
              else
              {
                 screenTimeout = 5 * 60 * 1000;
                 torchOn();
              }
            }
            
            //! setp counter
            if (ttgo->bma->isStepCounter()) {
                updateStepCounter(ttgo->bma->getCounter());
            }
            break;
        case Q_EVENT_AXP_INT:
          DSERIAL(println, "Q_EVENT_AXP_INT");

            ttgo->power->readIRQ();
            if (ttgo->power->isVbusPlugInIRQ()) {
                batState = LV_ICON_CHARGE;
                updateBatteryIcon(LV_ICON_CHARGE);
            }
            if (ttgo->power->isVbusRemoveIRQ()) {
                batState = LV_ICON_CALCULATION;
                updateBatteryIcon(LV_ICON_CALCULATION);
            }
            if (ttgo->power->isChargingDoneIRQ()) {
                batState = LV_ICON_CALCULATION;
                updateBatteryIcon(LV_ICON_CALCULATION);
            }
            if (ttgo->power->isPEKShortPressIRQ()) {
                ttgo->power->clearIRQ();
                low_energy();
                return;
            }
            ttgo->power->clearIRQ();
            break;
        case Q_EVENT_WIFI_SCAN_DONE: {
            int16_t len =  WiFi.scanComplete();
            for (int i = 0; i < len; ++i) {
                wifi_list_add(WiFi.SSID(i).c_str());
            }
            break;
        }
        default:
            break;
        }

    }
    if (lv_disp_get_inactive_time(NULL) < screenTimeout) {
        lv_task_handler();
    } else {
        low_energy();
    }
}
