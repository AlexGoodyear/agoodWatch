# agoodWatch	<a href="https://www.buymeacoffee.com/alexgoodyear" target="_blank"><img src="https://img.shields.io/badge/Buy%20me%20a%20coffee-%E2%82%AC5-orange?style=for-the-badge&logo=buy-me-a-coffee" /></a>

## A TTGO-T-Watch-2020 Arduino sketch

<img src="https://github.com/AlexGoodyear/agoodWatch/blob/master/images/agoodWatchV02.jpg" width="300" />
<img src="https://github.com/AlexGoodyear/agoodWatch/blob/master/images/agoodWatchV06.jpg" width="300" />

### Installation.
First ensure that you have installed the [TTGO_TWatch_Library](https://github.com/AlexGoodyear/TTGO_TWatch_Library) into your Arduino/libraries directory. Next put the agoodWatch directory from this repository into your Arduino sketch area.

This code is derived from the SimpleWatch example in the [Xinyuan LilyGO TTGO_TWatch GitHub repository](https://github.com/Xinyuan-LilyGO/TTGO_TWatch_Library).

### How to use.
Wake the watch by ...
1. raising your wrist to be horizontal in front of your face with the watch screen facing up, then rotate your wrist about 25 degrees so you can see the display better - the screen should wake up.
2. press the bezel button.
3. double tap the watch (this is quite a fast double tap!).

If you cannont see the display because of bright sunlight, then double tap the watch to go to full brightness.

If you double tap it again it will go into torch mode which is a white screen without the time displayed. This screen will stay active for 5 minutes or until you cancel with the bezel button.

Use the bezel button at anytime to put the watch back into sleep mode (this will also cancel any temporary brightness or tourch mode).

All navigation is now done using swipes. I find the best way to perform a swipe is to start on the metal bezel and then swipe acros the screen to the opposite metal bezel quite slowly - it should take you about 1 whole second. If you swipe too fast the watch will not recognise it - slow down.

The watch faces (currently 2) are in the top row, so to go from the default digital display, put your finger on the right hand side and swipe slowly left to get to the next watch face. To make this watch face the new default, perform a long press in the centre of the screen until you feel the watch react. After a new face has been selected you will be at the left most watch face again.

To get to the WiFi and About menus you must start from the left most watch face and swipe up. You can swipe left from each new menu icon. To go back to the current watch face, swipe right until you get back to the menu icon and then down to return to the watch faces.

This is a "map" of the current screens ...

[Face 1]---[Face 2]
   |
   |
[Wifi icon]---[Wifi config]    <-- the WiFi list and keyboard password entry
   |				   are popups and not part of the swipe system
   |
[About icon]---[About text]

The WiFi and ntp time settings are still taken directly from the original simpleWatch code - please don't raise issues about these functions, they obviously need work and it is on my todo list. I find the ntp times can vary a bit so just cancel an obviously incorrect time and try again.

The new keyboard is now split over 2 screens that are side by side. Press the arrow button at the base of the keyboard to shift to the other screen. Use the "ABC"/"abc" button to move between lower and upper case characters. Use the "1#" button to change the keyboard over to numbers and punctuation characters. The "abc" button will return to the normal alphabet characters. The cross ("X") in the lower left will abandon any input and the carriage return and tick characters on the right of the keyboard will enter the typed text.

### Known Issues.
1. All settings are hard coded - configurable settings are on my todo list.
2. Customise your time zone setting using RTC_TIME_ZONE defined in gui.h - please read the comment above this definition and find a city in your time zone in the spreadsheet list and use the time zone code for that city.

### Currently working on.
1. Configuration settings. (50% done)
2. More "real" and fun watch faces.

### Version 0.6  14/09/2020
1. New navigation strategy using swipes/gestures - no more wasteful navigation buttons!
2. Multiple (OK just 2 so far) watch faces - digital and analogue (with animations).

### Version 0.5  21/08/2020
1. Update to latest Xinyuan-LilyGo TTGO_TWatch_Library including LVGL v7.3.1
2. New keyboard design - this was much more work than the code changes reflect!

### Version 0.4  12/08/2020
1. Tweak torch mode to still display status bar (for battery monitoring).
2. Time is now based upon time zone strings.
3. Improve the About screen.

### Version 0.3
1. Forgot to put an exit button on the empty "About" screen - doh!
2. Populate the "About" screen.
3. Improved torch mode to now make the whole screen white for extra light.

### Version 0.2
1. Fix button event detection difference in the latest version of LVGL.
2. Add better comments - this will be an ongoing effort for each version!
3. Add max screen brightness mode for sunny days - just double tap whilst the screen is on and it will go to full brightness.
4. Add a torch mode for when the kids get you up in the middle of the night - first wake the watch using either the bezel button, double tap or tilt. Then double tap the watch to switch to full sun mode and then double tap again. The screen will stay on for 5 minutes or until the bezel button is used to put the watch back to sleep.

### Version 0.1 has added ...
1. Add seconds to the time.
2. Add day and date.
3. NTP is now GMT.
4. Update display before enabling it after wakeup (otherwise is displays the old time for up to a second).
5. The time is British Summer Time aware.
6. The wrist tilt display activation works.
7. Double tapping the watch will also wake it.
8. ESP32 light sleep mode is used to extend the battery usage.
9. The backlight dims between 10pm and 8am (saving battery).
10. The backlight remembers it's setting.
