Various bits of software related to different components of [another day](https://kevinstadler.github.io/#anotherday)

## another day (pt. 1)

Time-delayed LED playback of web-retrieved environmental light data on an ESP8266.

Data between the two design iterations is technically interchangeable, but interpretation is slightly different:

* Mk1: uses a heuristic to determine how long to sleep before taking next light measurement. Only stores new data points when they are noticably different, but might miss noticably different datapoints in between, i.e. no guarantees for the timespans in between. For playback, data points are interpreted as keyframes, with linear interpolation between them, leading to smooth replay.
* Mk2: continuous busy reads by the sensor guarantee that any noticably different new data point (at least in terms of luminance) will be recorded, so data points actually represent periods of stable readings (+- less than the just noticable difference). Playback can therefore forego linear interpolation between datapoints and just instantly apply all (less-than-noticable) changes.

### Mk1

#### LightLogger

Wemos D1 Mini taking continuous environmental light measurements from a TCS34725 sensor, regularly uploading them via WiFi. Powered from a `300mAh` LiPo battery, charged from a TP4056 with charging current set to 130mA (Rprog = `9.5k`), input current from a `150mA @ 6V` solar panel.

First approach at continuous environmental light logging using a Wemos D1 Mini, based on heuristic-driven waking up from timed deep sleep to take measurements from a TCS34725. Data is accumulated in RTC user memory, regularly persisted to SPIFFS and from there uploaded to the web in bulk.

* TCS34725 with SCL on `D2`, SDA on `D1` (is this right??)
* AM2302 temperature/humidity sensor, only measuring occasionally (`D7` for supplying power, `D6` for data)
* battery voltage (`A0` -- can't remember the resistor value but it was so that 1V would produce a reading of 200 on the pin's `[0,1023]` scale)

##### RTC user memory

Since Arduino 2.3.0, RTC memory can be addressed directly (heed 4-byte alignment!), but after upgrading from Arduino 2.7.4 to 3.0.2 the two extra pointers for more convenient access to inner structures that worked under 2.7.4 now only for reading, but writes to them do not seem to be persisted outside the scope of the write:

```cpp
static nv_s* nv = (nv_s*) RTC_USER_MEM; // user RTC RAM area

// since Arduino 3.*, THESE TWO STRUCTURES WORK FOR READING ONLY
static RtcData *data = &nv->rtcData;
static LogRecord *cache = nv->cache;
```

From the [3.* changelog](https://github.com/esp8266/Arduino/releases/tag/3.0.2) it's not clear to me if (or why) this would be the case, either way I just had to replace all shorthand `data->foo = ...` assignments with `nv->rtcData.foo = ...` ones and all worked again.

##### Bogus `0xff` writes to SPIFFS after upgrade to 3.0.2

Also after the upgrade, some calls to `println()` on a SPIFFS file would write two `0xff`s instead of `\r\n` (and sometimes replace the character to be printed with an `0xff` as well). The problem persisted after formatting the SPIFFS filesystem. Changing over to LittleFS solved the problem.

##### WiFi cannot connect until power cycle bug

During continuous outdoor usage I consistently encountered the supposedly closed [Arduino#2235](https://github.com/esp8266/Arduino/issues/2235)/[#5527](https://github.com/esp8266/Arduino/issues/5527) after ~ 1 week of runtime.

Suggested workarounds which did not seem to work for me:

1. `WiFi.persistent(false)` on first initialization
2. `WiFi.disconnect()` *before* `WiFi.begin()`


#### LightPlayer

With 2 PWM outputs for the LEDs, 3 pins for the MAX7219 display and 2 pins for the TCS34725 (I2C, technically only needed during calibration) the ESP8266 was already running out of pins. Luckily the [`MD_MAX72XX` library](https://github.com/MajicDesigns/MD_MAX72XX) supports software SPI on arbitrary pins, meaning that one of the SPI pins can be moved to the otherwise useless (non-PWM, non-I2C) pin `GPIO16`.

* 2x PWM outputs: `D7` (`GPIO13`, 8th on Uno shield) to middle cable, `D8` (`GPIO15` is 11th??? on Uno shield) to outer cable)
* potentiometer dial input (`A0`)
* `MAX72xx` LED display `MD_MAX72XX(MD_MAX72XX::FC16_HW, 16, 12, 14, 4)`, with DIN to `D0` (`GPIO16`, 3rd on Uno shield), `CLK` to `D6` (`GPIO12`, 7th on Uno shield), `CS` to `D5` (`GPIO14`, 6th on Uno shield)
* TCS34725 over I2C (`SCL` on `D1` aka `GPIO5`, 4th on Uno shield, `SDA` on `D2` aka `GPIO4`, 5th on Uno shield)

### Mk2

### LightLoggerMk2

Second design for continuous environmental light logging using a Wemos D1 Mini, this time kept in light sleep mode which only gets woken when a continuously measuring TCS34725 sensor encounters a readout that is noticably different (brighter/darker) from the last persisted one, and raises a wake interrupt for storing/persisting data.

### Power consumption

The first physical LightLogger used a bad Wemos D1 mini model, possibly with 12mA consumption even during deep sleep (according to [my own measurements](https://kevinstadler.github.io/notes/wemos-d1-mini-clones-sleep-mode-current-power-consumption-esp8266/)). Even with near-permanent light measurements, performance with the currently already quite over-powered battery and solar panel should not be affected.

* D1 Mini: .8mA in light sleep (3.2mA with bad models)
* D1 Mini: 16-18mA during wake periods
* TCS34725: .235mA during reads, .065mA during wait

With the original 300mAh battery (130mA charging current), assuming permanent sleep reads, battery runtime can be up to 300 hours, even permanently on (but without modem) a D1 mini could be powered for ~ 17 hours.

Supposedly charging current can be set to as low as 50mA (for a 100-150mAh battery), but the 300mAh battery is already tiny so no more space to save on that front. The only thing worth saving would be the *size* of the solar panel. According to [this thread](https://www.rcgroups.com/forums/showthread.php?1477186-Minimum-Current-required-to-charge-LiPo-Battery) charging rate can be as low as I want, but maybe not exceeding C/10... There are super compact (10x2.6cm) 5V 50mA solar panels though, those would correspond to C/6 charging rate...

<!-- Also if want to avoid charging restarting on every wake...16mA consumption is more than 10% of the 130mA current, strange that it doesn't always switch on actually? -->

### Continuous sensor read timings

TCS wait times between reads can be 1-256 steps, step size of either 2.4ms (614ms max) or 28.8ms (7.37s max). Rules of thumb:

* in very bright (daylight) conditions with the short integration times but little potential for sudden light changes it's probably enough to take a measurement every few seconds
* in very dark (night time) conditions with maximum integration time it might be necessary to do constant readings (but make sure that wakes are only triggered by increases that exceed the error/noticable range).

To catch lightning: "the average duration is 0.2 seconds made up from a number of much shorter flashes (strokes) of around 60 to 70 microseconds" ([Wikipedia](https://en.wikipedia.org/wiki/Lightning#Distribution_and_frequency))

### LightPlayerMk2

Only two differences to Mk1:

* switch away from Arduino software PWM to use `new_pwm` with 100Hz PWM frequency (based on [this analysis](https://kevinstadler.github.io/notes/esp8266-software-pwm-comparison-12v-led-strips/))
* no linear interpolation between datapoints, jump straight to next value (which, based on the improved logging regime, should be less than just noticably different anyway)

### Pin assignment

* D0 = GPIO3 = RX
* D1 = GPIO1 = TX
* D2 = GPIO16 = MD_MAX72xx DIN
* D3/D15 = GPIO5 = MD_MAX72xx
* D4/D14 = GPIO4 = MD_MAX72xx
* D5/D13 = GPIO14 = TCS34725 SDA
* D6/D12 = GPIO12 = WW (attached on D12)
* D7/D11 = GPIO13 = TCS34725 SCL

* D8 = GPIO0 = TCS34725 3.3V
* D9 = GPIO2 = LED/unused
* D10 = GPIO15 = CW LED

#### Transistor input spike observations

* WW (`GPIO12`) spiked a little bit, 20k pulldown resistor did the trick
* CW (`GPIO15`) did not spike, UNTIL the extra TCS got attached to `GPIO14` and `GPIO13`, then *significant* spike on turn-on!

## `thelightonmyroof`

Web interface for inspecting live and historical data from the `LightLogger`, live at [thiswasyouridea.com/thelightonmyroof/](https://thiswasyouridea.com/thelightonmyroof/)
