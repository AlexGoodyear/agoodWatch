# agoodWatch
A TTGO-Twatch-2020 watch program

This is derived from the SimpleWatch example in the [Xinyuan LilyGO TTGO_TWatch GitHub repository](https://github.com/Xinyuan-LilyGO/TTGO_TWatch_Library).

I have had to make some changes to the above library code for the low level access routines. Where possible I will submit these changes to the original.

Version 0.1 has added ...
1. Add seconds to the time.
2. Add day and date.
3. NTP is now GMT.
4. Update display before enabling it after wakeup (otherwise is displays the old time for up to a second).
5. The time is BritishSummer Time aware.
6. The wrist tilt display activation works.
7. Double tapping the watch will also wake it.
8. Light sleep mode is used to extend the battery usage.
9. The backlight dims between 10pm and 8am (saving battery).
10. The backlight remembers it's setting.
