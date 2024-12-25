/*
    A Signal Generator for the masses.

    https://corz.org/ESP32/square-sine-triangle-wave-signal-generator/

    ESP32 Signal Generator for Sine, Square and Triangle Waves. With serial, touch, potentiometer,
    button and web control, memory presets, import/export, loops/macros, music and much more.

    Frequency Range 1Hz to 500kHz/40MHz/150kHz    (Sine~~/Square++/Triangle waves^^)
    Pulse Width (Duty Cycle) / Sawtooth control from 0 to 100%

    Free. Open Source. Apache License.

    (c) me & the boys @ corz.org 2023

*/

// Should be about right..
String version = "7.4.3.2";   // "Silicon as Cities"


// Essential i2s/DAC libraries (these are installed along with the ESP32 boards)..
#include "soc/rtc_cntl_reg.h"
#include "soc/sens_reg.h"
#include "soc/rtc.h"
#include "driver/dac.h"
#include "driver/i2s.h"

// "permanent" preferences storage (also installed by default)
// https://espressif-docs.readthedocs-hosted.com/projects/arduino-esp32/en/latest/api/preferences.html
// We use this for storing settings and presets.
#include "nvs_flash.h"
#include <Preferences.h>

// No external libraries required.



/*
   BEGIN PREFS
              */


/*
   Initial values for wave shape, frequency and pulse width (duty cycle)

                         */
char mode = 'r';          // s=Sine, r=Rectangle/Square, t=Triangle/Sawtooth. Use single quotes (as it's a char).
float_t frequency = 1000; // 1000Hz == 1kHz
uint8_t pulse = 50;       // Pulse width 0 to 100%
                          // A square wave with a pulse width of 0 or 100% is a flat line
                          // and possibly other widths, depending on resolution.
/*
 You will notice I use standard C (stdint.h) definitions for integer values. As well as being more
 portable, I like being able to see exactly how big a number I'm working with, and keep them as
 small as possible. This is the way.

 If you are confused by these integer definitions, /read/ on, if not, /move/ on..

  u == unsigned == positive numbers only. Simple as that.

  No "u" means "signed", meaning it can go negative. So..

  uint8_t means an unsigned 8-bit value, which therefore has 256 possible values, from 0 to 255.

  If that doesn't make sense, then neither will this joke:

    There are 10 kinds of people in the world.
    Those who understand binary, and those who don't.

  A "signed" integer (int8_t) has the exact same number of possible values except they can go
  negative, so the range is instead -128 to 127.

  uint16_t is 16 bit, so can store 65536 values (0 to 65535).

  int16_t, same but being signed, can go negative and so the range (still 65536) is -32,768 to
  32,767.

  uint32_t is a 32 bit integer which can store 4,294,967,295 values. (https://en.wikipedia.org/wiki/4,294,967,295)

  And so on.

  Assigning a value outside the range of your chosen type can contain (e.g. uint8_t = 256) will
  result in "wrap-around" behaviour, which means if you did..

    uint16_t plusPosition = myString.indexOf("+");

  and "+" was NOT contained in myString, you would get 65535, instead of the -1 you expect. You
  would need to use a signed integer instead, e.g. int16_t, which can go negative.

  More exotic types..

  uint_least8_t is the same as uint8_t, but gives us the smallest type of unsigned int which has
  at /least/ 8 bits. In other words, optimised for memory. Of course there's also a signed version.

  I have found that replacing all uint8_t with uint_least8_t does, in fact, give us a slightly
  smaller memory footprint, but hardly worth it.

  There's also uint_fast8_t - the same but allows the compiler to assign a larger number of bits if
  it is more efficient to do so (i.e. saves it having to mask upper bits (as it's a 32 bit system)
  to ensure those registers are not set - useful in tight loops, to save CPU cycles, I guess).

  There's no code here that requires fast8_t types.

  Logic follows that it is also quite possible that using 8 bit values will be *slower* on our 32
  bit ESP32, but the difference is too small to be worth sacrificing readability. If we were working
  with tight loops, it might be a different story. Syat*^.

  BTW, I have no idea what the "_t" is about, but there it /always/ is! Tee-ing. For some reason.
  (just kidding; it means "type", and is supposed to denote cross-portability. hmm.)

  ^* Roughly translates (as I was taught) to: "To the best of my knowledge, at this time.".
*/


/*
   Square/Rectangle Wave Pin..
   25 or 26 makes sense (we'll be using their DAC output), saves re-attaching pins/clips/probes.
                          */
const uint8_t PWMPin = 26;
                          /*
   I see interference using pin 25 if also using DAC channel 1 for Sine wave.
   If both square and sine are on pin 26, I don't see this. Odd.
*/


/*
  SINE Wave channel/pin..
  channel 1 = DAC pin 25, channel 2 is pin 26
                                              */
dac_channel_t channel = DAC_CHANNEL_2; // or DAC_CHANNEL_1, of course.


/*
   Built-in limits for the generators. (Hz)
   If you request values greater than these, they will be auto-corrected.
   If your hardware can handle it, increase these (future chips!)**.
                                        */
uint32_t SineUpperLimit  = 500000;     // Sine 500kHz - you could go higher, but over 500kHz waveforms seriously degrade.
uint32_t RectUpperLimit  = 40000000;   // Rectangle/Square 40MHz
uint32_t TriUpperLimit   = 150000;     // Triangle/Sawtooth 150kHz
                                                                                    /*
    The available square wave frequency is related to PWM resolution.

    A 7-bit resolution will give you lovely square waves up to 624kHz.
    A 3-bit resolution will give you lovely square waveforms up to 10MHz, but duty cycle will be off if it's not 50%.
    A 1-bit resolution will give you waves up to 40MHz, but by that time they are pure sine waves (50% only).

    Send b<number> to change resolution bit depth on-the-fly.

    ** I notice that "future chips", namely the S3 and C3 variants do not have the DAC peripheral. (!!!)
       This is surely a mistake on Espressif's part; the DAC output is one of the ESP32's coolest features,
       and user's ignorance about what "8 bit" DAC actually means*** should not put you off
       incorporating this feature for those of us who *do* understand how digital audio works. *sigh*

       *** 8-bit DAC does NOT == "phone quality sound".
*/


// Lower limits (Hz)
// These are handy to prevent you crashing your module..
uint16_t TriLowerLimit   = 153;
uint16_t SineLowerLimit  = 16;
uint16_t RectLowerLimit  = 1;


// You might not want to do this..
bool checkLimitsOnBoot = true;


// Initial Wave Amplitude.. (1-4)
uint8_t waveAmplitude = 4;


/*
  You can use touch/web-buttons to increase/decrease the frequency.

  Touch Up the frequency on GPIO pin 33
  Touch Down the frequency on GPIO pin 32

  On the DevKit board, these pins are right next to the DAC pins, so you can
  easily attach a single block connector to all four pins.  */

// GPIO Pin number. Default is 32.
// If I'm currently using the Wemos D1 R32 to develop Signal Generator, this /may/ be set to 15
uint8_t touchUPPin = 33;

// DOWN Pin. Default is 32, but you can use any pin that accepts touch (most).
uint8_t touchDOWNPin = 32;

// Default frequency touch/button step
float_t fStep = 100; // Hz
/*
  Alter this on-the-fly in the serial command console with f<frequency value> (s* also works).

  NOTE: With Sine Waves, you may need to be grounded /yourself/ for touch to work (touch the pin
  and then also touch the sheath of the USB plug, for example). But after a few touches this way, it
  often works as normal, except maybe in reverse, or stops working. Very weird! This is on the
  DevKit board. Trying different pins for touch can get around this. Pin 15 works flawlessly on my
  D1 R32.

  (if you watch the touch readings, "normal" is around 80-something, but when Sine wave is running
  it's in the 600s - I might look into this)

  If regular touch stops working (after a switch to Sine wave + touch), a reboot (x) will fix it.

  The moral of the story is, TRY DIFFERENT PINS!

  The touch facility would be trivial to replace/augment with regular buttons, or a rotary pot; I
  wanted a quick-and-dirty solution that worked without any extra peripherals - you can just touch
  the module directly! Or stick a DuPont cable on it and touch the other end, or a crocodile clip
  onto some foil ... ("Kids! This is how me make synths!"). I use this way more than I expected to.
  Kids also love it. Hint: Set to a 1Hz square wave and hold the DOWN touch as a rhythm track and
  use the UP touch for your bass track. Now get those fingers moving! (remote OFF)

  NOTE: There is now limit checking when you use touch to change the frequency, so you can't easily
  push beyond the limits of your chip, and crash it. You can disable this, below, if required.

*/

// When values go below this number, the touch is activated.
// You might need to experiment to find the best value, depending on all sorts of factors;
// module used, length of wire, relative humidity... 50 works for both of my test devices with
// regular 50-150mm PCB jumper cables.
uint16_t touchThreshold = 50;


// By default, we don't store changes in frequency caused by touch commands.
bool storeTouch = false;
// But we do store changes caused by web button UP/DOWN commands
bool storeButton = true;
// Do as you will.


// If you want your touch/button commands to ignore the limits (above), set this to false.
bool stepsObeyLimits = true;

// Length of time (ms) for the touch debounce for PWM/Resolution touches (default: 100ms)
uint16_t deBounce = 100;


// You can choose what your touches will be changing; either frequency ('f'), pulse width
// ('p'), or resolution bit depth ('b'). Please use single quotes (it's a char).
//
char touchMode = 'f';


// If you prefer, you can disable the reporting of touches to the serial console, even when
// extended info is enabled. If you do a lot of touching, this might be preferable.
bool reportTouches = false;
//
// You can toggle this from the command-line with the command "rt".
// You can also set absolutely: "rte" will enable and "rtd" will disable reporting touches.


// Normally, step UP and DOWN for PWM is five percent, but you can change this here..
// Pulse Width Step (%)..
uint8_t pStep = 5;
//
// Alter this on-the-fly with j<int>


// Resolution bit depth always steps by exactly 1 (bit)



/*
   Potentiometer Control
                          */

// You can attach a potentiometer (variable resistor) to your board and control *something*.
// Don't enable this unless you actually hooked up a potentiometer!
//
bool usePOT = false;

// Which Pin for Potentiometer?
uint8_t potPIN = 34; // GIOP34
//
// Attach one end to GND, the other end to your board's 3.3V pin and the middle to THIS pin.
// Size doesn't matter. I'm using a 100k linear pot (with a nice detent at the half-way mark).


// Pot Controlling What?
// p = pulse width, f = frequency, b = resolution bit depth
//
char potMode = 'f';


// When controlling frequency with a potentiometer, you will likely want different limits.
// Perhaps audible frequencies..
uint16_t FreqLowerPotLimit = 50;
uint16_t FreqUpperPotLimit = 13000;


// You can increase the accuracy and speed (smaller steps, more fine-grained control) of the
// potentiometer control, at the expense of increased ghost changes. The default is 1, which works
// best for pwm/resolution changes. The maximum is 100, which is great for /automatically/
// generated "music"!
uint8_t stepAccuracy = 1;

// Okay, this is a misnomer. It's really all about speed and randomness.

// NOTE: You can alter this on-the-fly with v<int>    (V for Variable Resistor)



/*
   Physical Buttons..
                        */
bool useButtons = true;

// Total number of buttons..
const uint8_t buttonCount = 5;

/*
  The number of button controls you can create is limited only by your available GPIO pins.

  To create a button control, supply two pieces of information: 1: GPIO Pin number and 2. The
  command(s) to execute when that button is pressed.

  You can have physical button(s) send *any* command; change signal settings, switch presets, start
  or load loops and macros, reboot, whatever^^. The example below is an Emergency Stop button.

  To use this, you will obviously need to wire up a button. One side to the 3.3V rail, the other to
  your chosen GPIO pin. See the directories around this sketch for pics. By "button" I mean any
  control that can do a non-latching (i.e. it doesn't remain in the pushed state), momentary short-
  circuit will work fine. You could touch two wires together in a pinch.

  There is code inside startup() which sets the mode of this pin to internal pull-down, meaning you
  don't need to use a resistor. This won't work on the input-only GPIOs pins: 34-39, which have no
  internal pull-up/down resistor. If you are using one of these pins, hook up a 330 Ohm (pull-down)
  resistor between the GPIO-side of the button and GND. There's a pic of this setup in the parent
  folder.

  NOTE: Buttons use the "QCommand" mechanism to send commands; which is the highest priority type of
  command. Also, when a button is pressed, Signal Generator will skip directly to command
  processing, bypassing touch, delay and potentiometer control checks.

  A button press will also jump right into the middle of a loop / macro and is therefore THE highest
  priority command. I don't know about you, but when I press a button, I expect immediate results.

 */

// Set the GPIO pin of any button for which you wish to create a physical button control.
// Set to 0 to disable a button.

// Apologies for the arrays, but it saves a lot of code down the way and makes it trivial to add
// new buttons, if you need them (don't forget to alter buttonCount, above).
//
uint8_t buttonPins[buttonCount] = {
  14,     // These other pins work fine on my modules..
  0,      // 12
  0,      // 15 works for my NODE-MCU WROVER, but standard WROVER Kit apparently not. Ali is best!
  18,     // 13 (this button control won't be created as the corresponding command is blank)
  0       // 5  (remember n00bs, no comma after the final value!)
};

// Be careful which GPIO pins you use. You will want to avoid 1, 2, 3, 4, 6-11, 16, 17, 34-39 and
// probably others. Testing is key. That and a decent copy of the pinout for your current module.
// Crashing or a broken terminal connexion are sure signs you need to switch pins.


// And now some commands for those buttons..
// Only one of these button controls will be created. Can you guess which?
//
String buttonCommands[buttonCount] = {
  "end",    // Real handy as a physical control!
  "!",      // If button 2 pin == 0, this is ignored.
  "loop",   // Ditto
  "",       // If button 4 pin != 0, but this is empty, button control 4 is /not/ initialised.
  "stop"    // In other words, both Pin AND Command must be set in order to create a physical button control.
};
/*
  Obviously, the first command in the list corresponds to the first pin the list, i.e. button 1,
  and so on. Values separated by commas, except the final value. It's a list (array).

  If you need more buttons, simply adjust buttonCount (above) and add values (pin number and
  command) into the two above lists. Also, send me a pic of that box!

  NOTE: If you are in the middle of a loop/macro when you press the button, your button command
  will *immediately* take effect and once complete, the loop/macro will restart from its beginning.
  Unless your command is "end", of course, which destroys the queue completely.

  NOTE: You can chain multiple commands, just like a regular command-line command can (!), creating..

    Button-Activated Macros (BAMs!)

  .. that use some memory (which you have loads of) but /ZERO/ NVS entries. Think about it.

  Anything you would normally throw over the command-line works here.

  ^^ To test this, I created a "default setup" button by replacing that first simple "end" command
  with the /complete/ output from my current dev board's "export all" command, all 3,351 characters
  of it; with loops and macros and a dozen presets, pasted right here between the double-quotes.

  Then Upload. "wipe". Clean reboot. Hit the button... Booyah!! Everything is restored perfectly.

  So that's another use for a button**: Factory Reset. Or maybe: Signal Generator Setup.

  Here's an idea. You could have a macro which plays decreasing frequencies slowly. When someone
  hears the tone (blindfold), they hit the button (command: !) and then the current settings are
  printed out, as usual. Now you know the upper-limit* of their (or your) hearing. Repeat a few
  times for accuracy, of course.

  * Even one single unit of alcohol will lower this DRAMATICALLY.
    This hopefully shouldn't be an issue when testing kids' hearing.
    (if you didn't laugh there, you need a break!)

  ** Aye, there are near endless uses for a button.***

  *** It's almost like the word "useful" was created *just* for buttons.

*/



/*
  Extended Information.

  Print extended information to the serial console, or not..

  NOTE: You can toggle this from the command-line/console/url/etc. with "e".
  This is automatically disabled during loops.
                  */
bool eXi = true;

/*
   From now on, all "extended information" variables shall be named "eXi".
                                                                            */


// Sine Factor..
//
// We keep this crazy variable name as homage..
// float_t SINFAKT = 127.0;
float_t SINFAKT = 131.3;
/*
  (measured for step size == 1 and no divider - 8MHz-ish)

  It's basically a magic number. Someone else proposed 125.6.

  I very much suspect this number needs to be tweaked for different chips.

  If you are creating sine waves around a particular set of frequencies, you can tweak this to
  achieve better accuracy within /that/ range of frequencies.
*/



/*
    Presets
              */

// Maximum number of presets.
// After a lot of testing, I find that 50 works well for the default partition size. See notes above.
uint8_t presetMAX = 50;
/*
  NOTE: The ABSOLUTE MAXIMUM number of presets you can create is 253. Attempting to save preset 254
  may crash your module, or at the very least give you a nasty error. No matter how much NVS space
  you have (see below for how to increase it).

  Changing this does not affect your NVS in any way; it simply changes the number of presets Signal
  Generator attempts to read and write. If you saved 50 presets then changed this to 25, the other
  25 presets (26-50) will still be there.
*/
/*
  If layering is enabled, rather than store ALL the current settings to the preset, only those
  settings WHICH HAVE BEEN SET will be saved to the preset. This enables you to do cool and
  interesting things with your presets.

  When this is enabled, you still have the option to save every setting, by simply setting it before
  you save a preset, so it becomes the stored default setting**.

  If you want your presets to always contain *every* current setting, regardless of whether or not
  you manually set it, set this to false. This is simpler but *much* less flexible, and uses more
  memory.

  ** A quick way to do this is with the "d" command, which sets *all* the current settings as you
  default settings ("m" on its own does the same thing when layering is disabled), populating the
  default NVS preset (your defaults, i.e. active on reboot) with whatever settings are /currently/
  active.

  NOTE: You can use "," or "k" on the command-line, at any time, to see the current stored settings,
  i.e. those stored in NVS which would be saved in the new preset. Hit <enter> to see which settings
  are currently /in effect/.

                           */
bool layerPresets = true;
/*
    So, if you want a preset to save a specific setting, ensure you set that /before/ you save the
    preset to memory. For example, if you wipe your defaults and switch all settings except the step
    size; when you create a preset, step size will /not/ be recorded with that preset. When this
    preset is loaded, it will inherit your /current/ step size. Again, this is by design, and is
    true for *all* settings when layering is enabled.

    SO, you can create, let's say a Triangle wave with a 100% pulse width (right-leaning saw-
    tooth) and small step size, but *without* setting the frequency. In future, you can select
    this type of wave at any frequency by simply loading the preset whilst /at/ that frequency.
    Or load it and set your required frequency!

    You can save presets that only set /one/ particular setting, e.g. frequency, and then switch
    between your chosen frequencies using the other current wave settings. Or whatever.

    One preset could simply set the pulse width to zero, another to 100%. The possibilities are
    limitless.

    Not only does this give us HUGE flexibility, it also saves precious /space/ in your NVRAM.

    To wipe your current default preferences (to get that blank slate to start a preset afresh), use
    the "w" command.

    Toggle Layering Preset from the command-line with "lp".
    Or use absolute setting commands: "lpe" to enable, "lpd" to disable.
*/


/*
  Save ALL Settings. Always.

  It probably makes more sense to save only the settings which apply to your current waveform, e.g.
  we wouldn't need to save PWM step size when generating a Sine wave. However, you may have other
  plans, and need settings saved for future waveform-switching fun. It's your call.

  Set this to false to save ONLY settings which apply to the current waveform. The upshot* of this
  is that if you did something like this..

    import m=s,f=500,s=50,j=10,b=10,v=2,h=f

  The PWM Step size (j) and Resolution Bit Depth (b) would NOT be saved to the preset. This uses
  less memory, of course.

  To always save all (committed) settings to a preset, set this to true.

  * As improbable as this sounds, it was writing the word "upshot" here that got me started reading
    Douglas Adams again. For some reason, the word "upshot" always makes me think of Douglas Adams,
    perhaps because it's used in the Hitch-hikers Guide. Or perhaps some other reason. But there you
    have it; Doug's in my mind most every day and I can think of far worse things to have popping
    into your mind every day, like perhaps the Ravenous Bugblatter Beast of Traal... Where was I?..

 */
bool saveALL = true; // Ooh, that rhymes!
/*
   true by default because losing data is like a wee death
                                                               */

/*
  You can toggle this from the command-line with "sa". You can also enable with "sae" and disable
  with "sad", if absolute setting is required; i.e. at the start of a big import.
*/



/*

 During "export all", all presets and all loops are exported. Always.

 When exporting *individual* presets, you can choose to export ALL data within a preset (including
 any loop/macro data) or just the signal data (the default).

 If you enable this, Signal Generator can tag any loop/macro data contained within the preset onto
 the end of the export data. However, as the loop/macro and preset data are not necessarily related,
 it can be confusing to have this data exported here.

 Either way, you can always get your loop/macro data in exportable format by doing:

  ll

 For individual lines or:

  lll

 For one big importable list of all your loops.
                      */
bool exportALL = false;

// Toggle with "ea". "eae" to enable, "ead" to disable.
// NOTE: This preference is not saved to NVS, so will revert to *this* setting on reboot.



/*

  LOADS OF PRESETS AND LOOPS!

  (aka. how to get more Non-Volatile Storage space.. aka. Increase your NVRAM partition size)

  Okay, so you for some reason need *LOADS* of presets, like "hunners". Or maybe you just arrived
  here looking for a way to increase your ESP32 NVS size. Fair enough. This can be done.

  You will need to do a wee bit of minor hacking. First, create a new partition scheme file, here..

  ~/.arduino15/packages/esp32/hardware/esp32/2.x/tools/partitions/  (or *your OS* equivalent; '~/' == home)

    TODO -  I spotted an example sketch with its own partitions.csv file in the program directory.
            check how this works.

  Contents be this (or something similar - this was my first test and now I have 709 free "entries",
  SO, TEST COMPLETE! If you need OTA or something else, feel free to play around and let me know):

# Name,   Type, SubType,  Offset,   Size,    Flags
nvs,      data, nvs,      0x9000,   0x7000,
app,      app,  ota_0,    0x10000,  0x300000,
spiffs,   data, spiffs,   0x310000, 0xE0000,
coredump, data, coredump, 0x3F0000, 0x10000,

  Let's name this file "nvs_increase.csv".

  The initial 0x9000 offset is required so we don't overlap the partition table itself. You can
  even leave the other offsets blank and let the chip work it out for you. Nice.

  Then in your boards file (~/.arduino15/packages/esp32/hardware/esp32/2.x/boards.txt) add a menu
  item for your new partition scheme..

  Let's add it to the menu for the DoIt DevKit V1 board, which in fact has NO partition menu at all!
  Unless you add it, probably right under ...menu.FlashFreq..

esp32doit-devkit-v1.menu.PartitionScheme.nvs_increase=Enlarge NVS Size
esp32doit-devkit-v1.menu.PartitionScheme.nvs_increase.build.partitions=nvs_increase

  Did you notice the entry on the second line matches our file name? Good.

  Yes, it also matches *both* the Arduino IDE menu entries, but it doesn't need to; this is simply
  for convenience and readability.

  If you are adding a menu item where there was no partition menu at all, it's a good idea to add a
  default entry, to get back to:

esp32doit-devkit-v1.menu.PartitionScheme.default=Default 4MB with spiffs (1.2MB APP/1.5MB SPIFFS)
esp32doit-devkit-v1.menu.PartitionScheme.default.build.partitions=default

  Restart Arduino IDE and your new menu items will appear. Select the "Enlarge NVS Size" item and
  upload your sketch (wiping the entire storage one time is recommended). Enjoy.

  NOTE: This is a *very* basic partitioning scheme, with a simple emphasis on getting more NVS. If
  you are using this for some other sketch that needs OTA, etc., you will need to adapt to your
  specific requirements. I was simply interested in finding out if it could be done. Yes.

  https://espressif-docs.readthedocs-hosted.com/projects/arduino-esp32/en/latest/tutorials/partition_table.html

  NOTE: If you are using this partition scheme and switch boards, remember to re-select the
  "Increase NVS Size" entry before you upload. You might want to put it *first* in the list inside
  your boards.txt file so that it becomes the default partition scheme; to prevent "mistakes".

  I should add, with this partition scheme, as well as having oodles of free "entries", I'm also
  only using 29% of the available program space. Clearly there is more space available for NVS or
  SPIFFS or whatever-you-like. These modules sure are roomy!

*/



/*
    WiFi Remote Control
                           */

// Wouldn't it be handy to be able to alter settings remotely. (Hell Yes! That's why use ESP32)
// Comment out this next line to completely disable WiFi remote control..
#define REMOTE

// We're using a compiler define here (compiler commands are preceded by a hash character) so we can
// skip a ton of code if you have remote control disabled. (maybe to speed things up for a bit of
// music making)

// In other words, we can put blocks of code between #if defined/#endif conditions, and if it's NOT
// defined, the compiler ignores *all* the code between the conditions; does not see it at all.

// We can also set this on-the-fly (so long as you leave in the above #define REMOTE)
bool RemControl = true;

// You can switch this with "remote" (to toggle), "remotee" to enable and "remoted" to disable.


// WiFi Connect time-out (in seconds, 1-255). This should never happen.
uint8_t timeOut = 10;


// If you have enabled wifi remote control, enter your network credentials here..
//
const char *ssid      = "";
const char *password  = "";


// If you want to set a custom host name for your device, put it here.
// This may be set to whatever board I'm currently working with.
// If this is empty Signal Generator won't attempt to setup a host name.
//
const char *SGhostName = "";
//
// You will probably want to use this same host name on your router's (fixed IP) DHCP lease.

/*

    Host name also applies to..
                                  */
/*
  Soft Access Point

  Signal Generator has a built-in Wifi Access Point. (Gateway IP: 192.168.4.1)

  I find this to be more robust and widely-compatible than mDNS. Simply connect your WiFi to
  Signal Genereator's Access point and enjoy.

  Set the SSID for the Access Point here..
  (this is what will appear in the list of available WiFi networks on your phone/PC/whatever)

  Make this blank to disable the Soft AP.
                                         */
String softAPSSID = "SignalGenerator";


/*
  Password for the AP..
  Leave this blank to have an open network (i.e. no security)
                                    */
String softAPPass = "SigzSigzFutnik";

/*
  You may want to set a static IP Address in the WiFi connexion details on your phone / tablet /
  PC / whatever device, for your <softAPSSID> connexion, e.g..

    IP = 192.168.4.2
    Gateway = 192.168.4.1
    DNS Server = 192.168.4.1

  Also of note, the AP setting (in WiFi config) doesn't work on my laptop, but when I switch the
  network type to "infrastructure", it works fine. A tomorrow issue.

*/
/*
  AP Only mode

  If you are out and about and can't /both/ (signal generator AND controller device) connect to a
  shared local WiFi network, set this to true and Signal Generator will skip connecting to a local
  network and immediately switch over to full AP-only operation.

  This should enable you to connect easily even where there are no networks available.

  Signal Generator should be at http://192.168.4.1, though can check your WiFi connexion
  details to see the IP address of the current access point (where Signal Generator is located)
  once you have connected to its AP, then point your browser to the same IP.

  If you have a serial connexion, you will see the AP IP printed to the console.

  If you dig into the code below, you can also hack in a different IP address.
  The code you need is there already.

                    */
bool onlyAP = false;

// You can switch this from the command-line with the wap/waa commands (WiFi Access Point, WiFi All
// Access). Your device will reboot and start up in the new mode.



// If your board isn't defined, you can uncomment this..
// static const uint8_t LED_BUILTIN = 2;


/*
  SUCCESS!!! LED

  We will flash the built-in LED on a successful WiFi connexion. Enter its GPIO pin here..
  ("LED_BUILTIN" should be set automatically for your board. Or else, try 2.)
                            */
// const uint8_t led = LED_BUILTIN;
const uint8_t led = 2;

// see: ~/.arduino15/packages/esp32/hardware/esp32/CURRENT-VERSION/variants/YOUR-BOARD-HERE/pins_arduino.h
// Or C:\Users\<YOU>\.arduino15\etc.. I would imagine; if you are doing this on Windows.
// If it exists, that is. How to add? Hmm.


// Reboot Daily?
// If you are running for a *long* time with lots of access requests, this makes sense.
// NOTE: if you are playing a loop, this is ignored until the loop ends.
bool dailyReboot = true;



// When printing out the current details, Signal Generator prefers to print out a symbol for the
// current waveform (e.g. Triangle Wave △ 1.025kHz), which makes for quick readability. This info
// appears in both console and web. But if you prefer, you can have a plain old "@" symbol..
//
bool printWaveSymbols = true;


// You can use cpu<number> to set the cpu frequency (240/160/80) on-the-fly.
// If remote control is enabled, your device will reboot.
uint32_t cpuSpeed = 240;


// "wipe" commands can only be performed from a serial console attached to the device.
// Or not..
bool wipeIsSerialOnly = true;



// Command delimiter.
// Normally ';' (semicolon), but you can use something else, if you need to..
char commandsDelimiter = ';';




/*
    END PREFS
               */




#if defined REMOTE

  #include <WiFi.h>
  #include <WebServer.h>

  /*
    I recommend you setup proper DNS names for all your IoT devices. It's much easier to type "uno"
    into your browser than it is to type "192.168.1.245", or whatever. Also easier to remember.
    Once a device is setup with a fixed function, you could use that as the name, e.g. "sig", or
    "laser".

    This functionality is usually easy enough to setup on most routers (best). Or else use your
    hosts file on your PC/tablet/phone/whatever. Set the same host name (above, in your prefs) also.

    So, initialize the web server and set the port number to 80.

    Setting the port number is optional (you can just do "WebServer server"), as port 80 is the
    default. But this reminds me how easy it is to change.
                                                          */
  WebServer server(80);
  const String _PLAIN_TEXT_ = "text/plain;charset=UTF-8"; //  We will use this String a lot.
  const String _HTML5_TEXT_ = "text/html;charset=UTF-8"; // This is the default for HTML5, anyway.

#endif


// Initialize Preferences Instance..
Preferences prefs;


// For Rectangle Wave..
const uint8_t PWMChannel = 0;


// The initial number of PWM Resolution bits.
// Somewhere between 3 and 7 is probably best for general use.
uint8_t PWMResBits = 6;

// Number of steps used to achieve the correct ratio:
uint16_t PWMSteps = 64;
//
// This is automatically adjusted if resolution changes.
// But Signal Generator will use *this* setting until then. *wink*


// Buffer for creating the Triangle/Sawtooth function.
uint_fast32_t tBuff[128];

// i2s port number.. (has to be 0 for DAC functions)
static const i2s_port_t i2s_num = (i2s_port_t)0;


// Store this..
char oldMode = '~';

// This gets flipped when your chosen frequency has been auto-limited.
bool didLimit = false;

// If square wave fails at requested frequency/resolution, this gets flipped.
bool recFailed = false;

// Light debounce for PWM and resolution touches..
uint32_t touchTimer;


// These are WebConsole commands which are created from clicks in the main web interface.
String WebCommand = "";

// We can chain commands with a delimiter (normally semicolon, ";").
// These commands get queued, in here..
String QCommand = "";

// This string is set with the output from commands. It is the console "output", like you would get
// in a serial console. In the Web Console, we use AJAX to fetch it right after a command is sent.
String LastMessage = "";

// For logging/serial console purposes, it seems like a good idea to differentiate
// between the two types of command. This makes it so.
bool fromWebConsole = false; // currently only used for frequency step adjustments

// The first time a new client loads the web console page they get the full list of command.
// We use this string to store "known" clients, so we don't repeat that on subsequent requests.
// This is reset on reboot.
String knownClients;

// Might as well set this now (It will be used a /lot/. Often as an addendum to some other String)..
const String _OUT_OF_RANGE_ = "Preset Number Out-Of-Range! (1-" + (String)presetMAX  + ")";

// Store pot values..
uint16_t analogValue = 0;
uint16_t analogValueOLD = analogValue;

// Loop settings..
bool iLooping = false, eXiTmp; // iLooping is true when Signal Generator is playing a loop/macro.
String loopCommands; // The currently loaded loop/macro commands.

// Non-blocking delay..
bool amDelaying = false;
uint32_t delayTime = 500; // In case the first delay is blank
uint32_t delayStart;
// Not *too* important, but if you have *HUGE* delays in your loops, you /could/ prevent the ESP32
// device from doing basic routine maintenance; so we use a simple non-blocking method.


// Default Musical Octave..
uint8_t myOctave = 4;


// Physical Buttons..
struct button {
  uint8_t pin;
  String command;
  uint8_t state;
  uint8_t oldState;
};

struct SGButtons {
  button my_butt[buttonCount];
};

SGButtons buttons;



// Yes, it's String, but we need one single character only, which is why I earlier fooled you earlier
// into thinking this was a char! Let the compiler deal with it.
String commandDelimiter = (String)commandsDelimiter;


// Handy info we define once at compile time and spit out on boot-up.

// Technically, this is the way..
// const char compile_time[] = __DATE__ " @ " __TIME__;

// HOWEVER, the compiler on my system always uses the /current/ time to set __TIMESTAMP__
// AND the date format is better (it states the "day"), so..
const char *compile_time  = __TIMESTAMP__;



// Experimental section:





// OKAY, let's go..



/*
   Configure the I2S bus..

   (see i2s_types.h / i2s.h for details)

   This is used for creating the triangle/sawtooth wave.

                                      */

// When driver installed == 0 (== ESP_OK) - those errors were annoying me!
int8_t INi2S = -1;

// Messing with this will only eat time. No, seriously.
i2s_driver_config_t i2s_config = {
  .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN),
  .sample_rate = 10000000, // This nice number is ignored. (it's updated dynamically)
  .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
  .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
  .communication_format = I2S_COMM_FORMAT_STAND_MSB,
  .intr_alloc_flags = 0,
  .dma_buf_count = 2,
  .dma_buf_len = 32,      // Also updated dynamically
  .use_apll = 0,          // If you set this to 1, also set the fixed_mclk to not-0. i.e. 10000000
  .tx_desc_auto_clear = false,
  .fixed_mclk = 0,
  .mclk_multiple = I2S_MCLK_MULTIPLE_DEFAULT, // Another parameter added to suppress compiler messages :roll:
  .bits_per_chan = I2S_BITS_PER_CHAN_DEFAULT
};



/*
   Start/Switch Signal Generator..

   Sine/Triangle return the *actual* frequency which for Triangle, should be the same as what was
   sent, but for Sine will be the closest frequency the cosine generator could setup.

   If a square wave failed, this will be reported to the serial console as a system error and to the
   web as an addendum to your usual information == " [FAILED]", so you know to try again with a
   different frequency / resolution / pulse width.
                              */
void startSignal(String from, bool doReport = false) {

  // For tracking down hardware quirks when I thought they were bugs. I left it in as it's handy.
  // This lets us know where the signal was started /from/..
  if (eXi && doReport) \
    Serial.printf(" Start Signal -> mode: %s (called from: %s)\n", makeHumanMode(mode).c_str(), from.c_str());

  uint32_t freq; // For LEDC errors
  frequency = abs(frequency);

  recFailed = false;

  switch (mode) {

    case 't' :
      mode = 't';
      stopSignal(); // Triangle always needs this
      startTriangle();
      // This is always (the same as) what was sent (until that bug in i2s_driver_install is fixed)
      frequency = triangleSetFrequency(frequency, pulse);

      break;

    case 'r' :
      mode = 'r';
      if (oldMode != 'r') stopSignal();
      freq = startRectangle();
      rectangleSetFrequency(frequency, pulse);
      if (freq == 0) recFailed = true;
    break;

    case 's' :
      mode = 's';
      if (oldMode != 's') stopSignal();
      startSinus();
      frequency = sinusSetFrequency(frequency);
    break;
  }

  oldMode = mode;
}


/*
  Stop output
                */
void stopSignal() {

  switch (oldMode) {
    case 't' :
      if (INi2S == ESP_OK) {
        i2s_driver_uninstall(i2s_num);
        INi2S = -1;
      };
      dac_i2s_disable(); // this shouldn't work! Without it, you'll need *luck* to make a 150kHz Triangle.
      // see driver/dac.h (this is enabled automatically on driver install, btw)
      break;
    case 'r' :
      reallyDetatchPWM();
      break;
    case 's' :
      dac_output_disable(channel);
      break;
  }
}



// Wrappers for setting char-based prefs..

void setMode(char myMode, bool doSave = true) {
  if (myMode == 'r' || myMode == 't' || myMode == 's') {
    mode = myMode;
    if (doSave) prefs.putChar("m", myMode);
  }
}

void setTouchMode(char myMode, bool doSave = true) {
  switch (myMode) {
    case 'p' :
    case 'd' : // duty cycle
      touchMode = 'p';
      break;
    case 'b' :
    case 'r' : // resolution
      touchMode = 'b';
      break;
    default:
      touchMode = 'f';
  }
  if (doSave) (prefs.putChar("h", touchMode));
}


/*

  Switch back-and-forth between default and specified preset's settings.

  If a preset does not exist, return false, otherwise return true.
  Whatever happens, we still switch.

  This is used a /lot/.

  Call with *no* parameters to start the /main/ (default) prefs.

  NOTE to Arduino IDE Devs:

    It's specifying default values for variables fed to functions which messes with the prototype
    generator. i.e. this..

                                 */
bool prefsSwitch(int8_t preset = 0) {

  String lPreset = "sg";

  // First, end the current prefs instance, whatever it is..
  prefs.end();

  // If the new namespace is to be a preset, set a name for this preset's namespace..
  // ("sg" with preset number appended, e.g. "sg3")
  if (preset != 0) lPreset += (String)preset;

  // Start the new prefs instance..
  prefs.begin(lPreset.c_str());

  // Check if this preset exists and return that status as a boolean..
  if (prefs.getChar("i", -1) == -1) return false;

  // It exists!
  return true;
}




/*

  Sine Wave..
                 */

/*
  Fire up the DAC!

  We are using the ESP32's built-in (8-bit) cosine generator.

                   */
void startSinus() {

  // Release output for PWM Pin..
  dac_output_enable(channel);

  // Enable tone generator - common to both channels
  SET_PERI_REG_MASK(SENS_SAR_DAC_CTRL1_REG, SENS_SW_TONE_EN);

  int8_t scale;

  switch (channel) {

      // Sure, we could create variables and what-not, but I like these two big sections!

      case DAC_CHANNEL_1:

          // Enable & connect tone generator to this channel
          SET_PERI_REG_MASK(SENS_SAR_DAC_CTRL2_REG, SENS_DAC_CW_EN1_M);
          // Invert MSB, otherwise part of the waveform will be inverted
          SET_PERI_REG_BITS(SENS_SAR_DAC_CTRL2_REG, SENS_DAC_INV1, 2, SENS_DAC_INV1_S);
          switch (waveAmplitude) {
            case 1: // The B in front of the int denotes a binary value follows. In other words..
              scale = B11; // 1/8        // three  (not eleven)
              break;
            case 2:
              scale = B10; // 1/4        // two
              break;
            case 3:
              scale = B01; // 1/2        // one
              break;
            default: // 4
              scale = B00; // 1/1        // zero    (the default - no scaling - see a pattern?)
              break;
          }
          // Set the cosine generator scaling to our now-determined value..
          SET_PERI_REG_BITS(SENS_SAR_DAC_CTRL2_REG, SENS_DAC_SCALE1, scale, SENS_DAC_SCALE1_S);
          break;

      case DAC_CHANNEL_2:

          SET_PERI_REG_MASK(SENS_SAR_DAC_CTRL2_REG, SENS_DAC_CW_EN2_M);
          SET_PERI_REG_BITS(SENS_SAR_DAC_CTRL2_REG, SENS_DAC_INV2, 2, SENS_DAC_INV2_S);

          // Same again for channel 2..
          switch (waveAmplitude) {
            case 1:
              scale = B11; // B b b binary.
              break;
            case 2:
              scale = B10;
              break;
            case 3:
              scale = B01;
              break;
            default:
              scale = B00;
              break;
          }
          SET_PERI_REG_BITS(SENS_SAR_DAC_CTRL2_REG, SENS_DAC_SCALE2, scale, SENS_DAC_SCALE2_S);
          break;

        default :
          break;
  }
}

// freq = dig_clk_rtc_freq * SENS_SAR_SW_FSTEP/65536

/*
  Calculate frequency for sine wave.
                                        */
float_t sinusSetFrequency(float_t frequency) {
  /*
    Formula frequency = step * SINFAKT / div
    step is the steps per clock pulse
    div is the pre-scaler for the 8MHz clock (RTC 8M clock divider)
    There are 8 pre-scalers from 1 to 1/8 around the combination pre-scaler and
    to find the step count, we test all eight pre-scaler variants.
    The combination with the smallest frequency deviation is chosen.
  */

  // if (eXi) Serial.println("\n Calculating frequency deviations.."); //debug
  // if (eXi) Serial.println(" round        frequency    steps        delta"); //debug

  float_t f;
  float_t delta, delta_min = 9999999.0;
  int s, step = 1, divi = 0; // store best variant here

  for (uint8_t div = 0; div < 8; div++) {

    s = round(frequency * (div+1) / SINFAKT);
    if ((s > 0) && ((div == 0) || (s < 1024))) {

      f = SINFAKT * s / (div+1);
      delta = abs(f - frequency);

      // // handy for debugging and FOR SCIENCE!
      // But will create empty mega-lines in your console even when eXi is false. Weird!
      // (this is why the linebreak hack before the final output inside loop())
      // if (eXi) Serial.print(div); Serial.print("        "); //debug
      // if (eXi) Serial.print(f); Serial.print("        "); //debug
      // if (eXi) Serial.print(s);Serial.print("        "); //debug
      // if (eXi) Serial.println(delta); //debug

      if (delta < delta_min) { // Deviation is less! -> Store current values..
        step = s;
        divi = div;
        delta_min = delta;
      }
    }
  }
  // We keep the one with the leastest deviance.

  // Set the "real" frequency value
  frequency = (SINFAKT * step) / (divi + 1);

  // // Hmm..
  // float_t foo = RTC_FAST_CLK_FREQ_APPROX / ( divi + 1 ) * (float_t)step / 65536; //debug
  // if (eXi) Serial.printf("\n internal clock estimates frequency @ %.2f \n", foo); //debug

  // Set frequency of internal CW generator common to both DAC channels
  REG_SET_FIELD(RTC_CNTL_CLK_CONF_REG, RTC_CNTL_CK8M_DIV_SEL, divi);

  // Set steps per master clock pulse
  SET_PERI_REG_BITS(SENS_SAR_DAC_CTRL1_REG, SENS_SW_FSTEP, step, SENS_SW_FSTEP_S);

  if (eXi) Serial.printf(" returning frequency: %.2f \n", frequency);

  return frequency;
}


// TODO ASCII Art for each wave section (make it easy to find in overview)



/*
    Square/Rectangle Wave..

                          */
int startRectangle() {
  reallyDetatchPWM(); // No, really.
  // if (ledcSetup(PWMChannel, frequency, PWMResBits) == 0) { return 0; } // debug
  uint32_t ledc = ledcSetup(PWMChannel, frequency, PWMResBits);
  ledcAttachPin(PWMPin, PWMChannel);
  ledcWrite(PWMChannel, (PWMSteps * pulse) / 100.0);
  return ledc;
}

/*
  Set frequency/resolution/duty cycle for rectangle wave..
                                                          */
void rectangleSetFrequency(float_t frequency, int8_t pulse) {
  ledcSetup(PWMChannel, frequency, PWMResBits);
  // Set the pulse width / duty cycle..
  // No matter how many times I write this simple math, I still need to /think/ about it! perrrr-cent.
  ledcWrite(PWMChannel, (PWMSteps * pulse) / 100.0);
}

/*
  Completely switch off PWM pins. And wiggle them.
  This seems long-winded but unlike more sane approaches, works.
  Bash bash bash!
                          */
void reallyDetatchPWM() {
  ledcDetachPin(PWMPin);
  pinMode(PWMPin, INPUT);
  pinMode(PWMPin, OUTPUT);
  digitalWrite(PWMPin, HIGH);
  digitalWrite(PWMPin, LOW);
  ledcDetachPin(PWMPin);
}
// Your trial-and-error detection meter just pinged, right?



// If the square wave failed, recFailed gets flipped to true.
// This function returns a String we can tag onto some output.
String recState() {
  String fl = "";
  if (recFailed) fl = " [FAILED]";
  return fl;
}





/*
  Triangle/Sawtooth Wave..

  This wave has a nice shape up to around 25.5kHz.
  And occasionally thereafter!
                                      */
void startTriangle() {
  i2s_set_pin(i2s_num, NULL); // I2S is used with the DAC
}

/*
  Here be goodies for those fine souls who read the comments..

  Got more than one ESP32 module?*
  Got an oscilloscope with two channels** (and X-Y mode)?

  If you said yes to both, here's some extra fun with Signal Generator..

  Triangle waves can be set fairly accurately. These and Sine waves are what you want for creating
  groovy displays to wow and amaze.

  Upload Signal Generator to two units, one connected to channel one of your scope, the other to
  channel two. Fire up a triangle wave on both generators. Set the first to exactly 1k.

  Now the fun begins; set the second generator to 1.001k, or 1001, so we have a little variance
  between the two. Set X-Y mode on your scope. Tada! For slow-mo, try 1000.2

  Play with the sawtooth angle on generator 2. Try p50, or p100, p0, etc.. I did say this was just
  for fun; simply for setting up a cool af space where folk might come in and go "Woah! What are you
  up to mate!?", or "Dad! That's amazing". ("Oh, that's nothing. What can I do for YOU?").

  Set the sawtooths to add up to 100; 30 on one, 70 on the other; is fun. Set the frequency of one
  unit to multiples (+ a bit), e.g. 4000.5, for rotating triagonal crowns. The farther apart the
  frequencies are, the more complex the shape. The farther from exact 0 the second frequency is, the
  faster the animation (assuming unit one is set to a frequency ending zero, e.g. 1000).

  The direction of the animation is determined by which unit is running at the higher frequency.
  Mess around and you will surely find some favourites. Then save that preset!

  Your oscilloscope will be doing cool things, see***. These kinds of slow-mo visuals are what your
  oscilloscope wants to be doing when you aren't using it for anything serious. If anyone asks, you
  can always say you are monitoring the converging tangent variable quotients in the crystal
  oscillations of the doodah whatever, by this point they've switched off. But if they haven't, you
  turn to them and, with intensity, stare into their eyes saying, "It's the heartbeat of an AI".
  Don't laugh, or you'll spoil it.


  * There is no way to get a single ESP32 unit to simultaneously output two different triangle/sine
    waves at different frequencies. So you need two modules. Just as well they are so cheap!


  ** If you are buying an oscilloscope, ffs, don't buy one with only one channel. These days, you
     can pick up a highly capable two channel DSO for peanuts. The ten quid you would save cheaping
     out buying a single channel scope will end up costing you seven times that. Word.

     While my poor Gould 1604 sits smoke-damaged, awaiting repair, I researched my fingers off and
     got myself a Zeeweii DSO2512G..

       https://www.aliexpress.com/item/1005004696526847.html

     This was £70 well-spent. You can't save X-Y images. There's no built-in stand. And that's
     pretty much all the down-sides out of the way. A fantastic portable DSO with features well
     beyond its price point (FFT display, Video Out, PC Save, USB-C Charging, Signal Generator, and
     more); not to mention, a big long-lasting battery and useful manual. If you are in the market
     for a portable two channel scope, I recommend this unit. And no, I don't get commission.

       NOTE: To power off a DSO2512G quickly, do: Power > Left > OK


  *** If you /don't/ see animated flexing boxes and triangle snakes and what-not, you probably need to
    mess around with your scope's ranges. If there's an "auto" button, start there.

*/


// Set frequency for triangle with corresponding pulse width (sawtooth angle)..
float_t triangleSetFrequency(float_t frequency, int8_t pulse) {

  float_t f = frequency;
  uint8_t buffLen = 64;
  // First the appropriate buffer size is determined.
  // Some trial-and-error here would probably produce even better numbers..
  if (frequency < 5001) {
    buffLen = 64;
  } else if (frequency < 10001) {
    buffLen = 32;
  } else if (frequency < 25001) {
    buffLen = 16;
  } else {
    buffLen = 8; // 8 is the minimum
  }
  // Sample rate must output both buffers in one period.
  uint32_t rate = frequency * 2.000000 * buffLen;

  // For the output to work, the I2S sampling rate must be above 5200.
  // If the sampling rate gets too low, you crash..
  if (rate < 5200) rate = 5200;

  // Set the real frequency value..
  frequency = rate / 2.000000 / buffLen;

  // Remove I2S driver..
  if (INi2S == ESP_OK) {
    i2s_driver_uninstall(i2s_num);
    INi2S = -1;
  };

  // Customize the configuration..
  i2s_config.sample_rate = rate;
  i2s_config.dma_buf_len = buffLen;

  // (re-)install driver with new settings..
  INi2S = i2s_driver_install(i2s_num, &i2s_config, 0, NULL);
  if(INi2S != ESP_OK) {
  /*
     This never happens because of a bug in i2s_driver_install()
     which has STILL not been fixed (2023-2).

     So it always returns 0 (ESP_OK).
                                                      */
    delay(250); // This never happens, but just in case! One day ...
    INi2S = i2s_driver_install(i2s_num, &i2s_config, 0, NULL);
  } // MCU coding is some pragmatic sh*t!

  if(INi2S == ESP_OK) {

    // Set the sampling rate..
    i2s_set_sample_rates(i2s_num, rate);

    // Fill the buffer
    fillBuffer(pulse, buffLen * 2);

    // And write it out..
    size_t bytes_written;
    i2s_write(i2s_num, (const char *)&tBuff, buffLen * 8, &bytes_written, portMAX_DELAY);

    // Guess what?
    return frequency;
  }

  // If this didn't work out, return the original frequency..
  return f;
}


/*
  Fill buffer for triangle waveform

  Parameters upTime is the duration for the increase in percent (so you can make saw-tooth shapes)
  Parameter buffSize specifies the buffer size for one period.
  The values for one period are written to the buffer.
*/
void fillBuffer(uint8_t upTime, uint8_t buffSize) {

  // A bit hacky, but now we can scale amplitude the same way (well, same scaling*) sine waves can..
  float_t Amplitude = 256.0 / (5-waveAmplitude);
  // * Sine wave will scale from the /centre/, whereas this will scale from 0,0. Your scope can handle it.

  uint8_t downTime;  // Time for the falling edge, in %.
  uint_fast32_t sample; // 32Bit data word (I2S requires two channels of 16 bits each)
  float_t divUP, divDOWN, buffValue;
  downTime = 100 - upTime; // MATH!

  // Calculate the number of steps for rise and fall
  uint16_t stepsUP = round(((1.0 * buffSize) / 100) * upTime);
  uint16_t stepsDOWN = round(((1.0 * buffSize) / 100) * downTime);
  uint16_t i;

// If you desperately need to /easily/ make messy 150k Triangle waves..
// Add a couple of Serial.prints here..
// Serial.printf(" stepsUP: %i\n", stepsUP); //debug
// Serial.printf(" stepsDOWN: %i\n", stepsDOWN); //debug
// Otherwise you may have to restart the wave a *few* times (Hit <enter>. Again!).
// Disabling WiFi remote control also makes 150k easy to hit. But where's the fun in that?
// On reboot, the signal comes up before wifi gets started. Try a reboot.
// Enabling Verbose debug output is a sure-fire way to get 150k. Crazy but true.
// Or you might try sending: r;t;r;t;r;t;r;t  -> loop14=: restart Triangle :;r;t;r;t;r;t;r;t;end

  // Compensation for possible rounding errors
  if ((stepsUP + stepsDOWN) < buffSize) stepsUP++;

  // Amplitude change per step for rise and fall
  divUP = Amplitude / stepsUP;
  divDOWN = Amplitude / stepsDOWN;

  // Fill the buffer
  buffValue = 0; // Increase (rising edge) starts with 0
  for (i = 0; i < stepsUP; i++) {
    sample = buffValue;
    sample = sample << 8; // Move bytes to the higher-value byte
    tBuff[i] = sample;
    buffValue += divUP; // Increase value
  }
  buffValue = Amplitude-1; // Falling edge starts with maximum value (255, or some scale thereof).
  for (i = 0; i < stepsDOWN; i++) {
    sample = buffValue;
    sample = sample << 8;
    tBuff[i + stepsUP] = sample;
    buffValue -= divDOWN;
  }
}



/*
   Switch PWM resolution bit depth..
   Also set the corresponding number of steps used to achieve the correct ratio:

   bits      =>  steps     approx. limits (min / max)

   16        =>  1024      1Hz    -   78KHz

   ...                     Observe*-> No change..

   10        =>  1024      1Hz    -   78KHz
   9         =>  512       2Hz    -   156kHz
   8         =>  256       4Hz    -   313kHz
   7         =>  128       8Hz    -   626kHz
   6         =>  64        16Hz   -   1.25MHz
   5         =>  32        31Hz   -   2.5MHz
   4         =>  16        62Hz   -   5.01MHz
   3         =>  8         123Hz  -   10MHz      (10019569.00Hz on this board)
   2         =>  4         245Hz  -   20.04MHz   (20039000.00Hz)
   1         =>  2         489Hz  -   40.08MHz   (40078000.00Hz)  Our square wave is now a beautiful Sine wave!

   NOTE: As you reduce the bits, your ability to get accurate duty cycles diminishes (unless they are 50%!).

   * In the style of "Radio Silence", by Thomas Dolby.

                                                            */
uint8_t switchResolution(uint8_t newbits, bool save = true) {

  if (newbits > 12) newbits = 12;
  if (newbits < 1) newbits = 1;

  switch (newbits) {

    case 1: PWMSteps = 2; break;
    case 2: PWMSteps = 4; break;
    case 3: PWMSteps = 8; break;
    case 4: PWMSteps = 16; break;
    case 5: PWMSteps = 32; break;
    case 6: PWMSteps = 64; break;
    case 7: PWMSteps = 128; break;
    case 8: PWMSteps = 256; break;
    case 9: PWMSteps = 512; break;
    case 10: PWMSteps = 1024; break;
    // No Point ...
    case 11: PWMSteps = 2048; break;
    case 12: PWMSteps = 4096; break;
    // Seriously, stop now. We need RMT or something for the really low stuff.
  }
  if (save) prefs.putUChar("b", newbits);
  return newbits;
}

// The coding hammer has been unleashed!



/*

   Create human-readable strings..

   If you don't want to spend the time writing "these sorts" of functions, you should stop coding
   and go do what you love, instead.

                                   */
String makeHumanMode(char myMode) {
  switch (myMode) {
    case 's' : return "Sine";
    case 't' : return "Triangle";
    case 'r' : return "Square";
  }
  return "";
}

String makeHumanTouchMode(char myMode) {
  switch (myMode) {
    case 'f' : return "Frequency";
    case 'p' : return "Pulse Width";
    case 'b' : return "Resolution Bit Depth";
  }
  return "";
}

/*
  Human-Readable Frequency..

  Used for displaying human-readable frequencies to the user.
  Also for creating human-readable strings for export, e.g. 40m instead of 40000000.00

                                                            */
String makeHumanFrequency(float_t f, bool exporting = false) {

  String humanF = "";
  float_t newF;
  char buffer[18];

  // Display values.. (e.g. 40MHz)
  String mV = "MHz";
  String kV = "kHz";
  String hV = "Hz";

  // Export values.. (e.g. 40m)
  if (exporting) {
    mV = "m";
    kV = "k";
    hV = "";
  }

  if (f > 999999.999) {
      newF = f/1000000.000;
      snprintf(buffer, 18, "%.9f", newF);      // A "safer" version of sprintf(). Not required here as
      humanF = chopZeros((String)buffer) + mV; // we have complete internal control of the string (buffer).
  } else if (f > 999.999) {
      newF = f/1000.000;
      //newF = roundf(newF * 100) / 100; // to 2 decimal places.

      snprintf(buffer, 15, "%.6f", newF);      // Could be handy for dealing with user input and other stuff.
      humanF = chopZeros((String)buffer) + kV; // Essentially, we specify the buffer size as we go. How Lame!
  } else {
      humanF = chopZeros((String)f) + hV;
  }

  return humanF;
}




/*
  Ensure the user hasn't attempted to go outside their own user-set limits..
                                */
void checkLimits(float_t freq) {

    // We could set the lowest upper frequency limit (triangle wave).
    // etc.. but it's less confusing to just set them /all/ below..
    uint32_t u_limit = 0;
    uint32_t l_limit = 0;
    didLimit = false;

    // Set frequency limits based on current mode..
    switch (mode) {
      case 's' : // Sine
        u_limit = SineUpperLimit;
        l_limit = SineLowerLimit;
        break;
      case 'r' : // Square/Rectangle
        u_limit = RectUpperLimit;
        l_limit = RectLowerLimit;
        break;
      case 't' : // Triangle/Sawtooth
        u_limit = TriUpperLimit;
        l_limit = TriLowerLimit;
        break;
    }

    // Check lower and upper limits. Simple.
    if (freq < l_limit) {
      frequency = l_limit;
      didLimit = true;
    }

    if (freq > u_limit) {
      frequency = u_limit;
      didLimit = true;
    }
}



/*
   Convert a "user" frequency (e.g. "2.5k") into a float value..

   Used for *all* user-input frequency, so we can have kHz and MHz values.
   As well as plain ole Hz, of course.

   It's so compact because we are working with String functions.

                                          */
float_t humanFreqToFloat(String newFreq) {
  newFreq.toLowerCase();
  if (newFreq.endsWith("k") || newFreq.endsWith("m")) {
      int8_t sLen = newFreq.length();
      String newFCMD = newFreq.substring(0, sLen-1); // Numeric part of the command
      if (newFreq.endsWith("k")) newFreq = newFCMD.toFloat() * 1000;
      if (newFreq.endsWith("m")) newFreq = newFCMD.toFloat() * 1000000;
  }
  return newFreq.toFloat();
}

/*
    Set a new frequency step size..
                                      */
void frequencyStepSet(String newFreq) {

  float_t thisFreq = humanFreqToFloat(newFreq);

  // Enforce sensible limits
  if (thisFreq < 1) thisFreq = 1; // 1Hz
  if (thisFreq > 10000000) thisFreq = 10000000; // 10MHz
  fStep = thisFreq; // NOW we set it.
  prefs.putFloat("s", fStep);
}


/*
   Set signal frequency..
                                                        */
void frequencySet(String newFreq, bool OVRide = false, bool doReport = true) {

  frequency = humanFreqToFloat(newFreq);

  if (eXi && doReport) Serial.printf(" User requested frequency:\t%s\n", makeHumanFrequency(frequency).c_str());
  if (OVRide == false) checkLimits(frequency);
  prefs.putFloat("f", frequency);
  if (eXi && doReport) Serial.printf(" Setting frequency:\t%s\n", makeHumanFrequency(frequency).c_str());
}



/*

  This is based on code from esp32-hal-ledc.c
  The typedefs for this are set in esp32-hal-ledc.h (which is already included)

  Except here we use it to set the frequency of *any* signal type..

                                                        */
uint32_t setNoteFrequency(note_t note) {

  const uint16_t noteFreqBase[12] = {
  // Note:    C         C#       D        Eb       E        F       F#        G       G#        A       Bb        B
  // typedef: NOTE_C,   NOTE_Cs, NOTE_D,  NOTE_Eb, NOTE_E,  NOTE_F, NOTE_Fs,  NOTE_G, NOTE_Gs,  NOTE_A, NOTE_Bb,  NOTE_B
              4186,     4435,    4699,    4978,    5274,    5588,   5920,     6272,   6645,     7040,   7459,     7902
  };
  uint32_t noteFreq = (uint32_t)noteFreqBase[note] / (uint32_t)(1 << (8-myOctave));
  frequencySet((String)noteFreq, false, false);
  return noteFreq;
}


/*

  Play a specific musical note.

  usage: *note[|octave]

  "note" is standard musical notes: C C# D Eb E F F# G G# A Bb B.

  Octave can be a number from 0-8, or a +/- symbol to donate an increase or decrease in octave.

  If octave is omitted, it remains at whatever was last set.

  This isn't really designed for playing music; there are better ways to go about that.

  Sure, you could use it to pop out a ditty..

    *g|4;~500;*g;~500;*d|+;~500;*d;~500;*e;~250;*f#;~250;*g;~250;*e;~250;*d;~500;.

  Or other sound effects..

    loop=*c|5;~50;*d;~50;*e;~50;*f;~50;*g;~50;*a;~50;*b;~50;*c|6;~50;*b|5;~50;*a;~50;*g;~50;*f;~50;*e;~50;*d;~50;

  But really it's designed to play single musical notes, for reference, tuning, etc..

                                          */
uint32_t playMusicalNote(String musicData) {

  int16_t Split = musicData.indexOf("|");
  String thisNote;
  uint32_t newFreq;

  if (Split != -1) {
    thisNote = musicData.substring(0, Split);
    String tmp = musicData.substring(Split+1);
    tmp.trim();
    if (tmp != "") {
      if (tmp == "+") {
        myOctave += 1;
      } else if (tmp == "-") {
        myOctave -= 1;
      } else {
        myOctave = tmp.toInt();
      }
    }
  } else {
    thisNote = musicData;
  }

  if(myOctave > 8) myOctave = 8;
  thisNote.trim();

  if (thisNote == "c#" || thisNote == "db") {
    newFreq = setNoteFrequency(NOTE_Cs);
  } else if (thisNote == "d") {
    newFreq = setNoteFrequency(NOTE_D);
  } else if (thisNote == "eb" || thisNote == "d#") {
    newFreq = setNoteFrequency(NOTE_Eb);
  } else if (thisNote == "e") {
    newFreq = setNoteFrequency(NOTE_E);
  } else if (thisNote == "f") {
    newFreq = setNoteFrequency(NOTE_F);
  } else if (thisNote == "f#" || thisNote == "gb") {
    newFreq = setNoteFrequency(NOTE_Fs);
  } else if (thisNote == "g") {
    newFreq = setNoteFrequency(NOTE_G);
  } else if (thisNote == "g#" || thisNote == "ab") {
    newFreq = setNoteFrequency(NOTE_Gs);
  } else if (thisNote == "a") {
    newFreq = setNoteFrequency(NOTE_A);
  } else if (thisNote == "bb" || thisNote == "a#") {
    newFreq = setNoteFrequency(NOTE_Bb);
  } else if (thisNote == "b") {
    newFreq = setNoteFrequency(NOTE_B);
  } else {
    newFreq = setNoteFrequency(NOTE_C);
  }
  return newFreq;
}




// Set the Sine / Triangle amplitude level..
// (1 - 4) 1 = 1/8th, 2 = 1/4, 3 = 1/2, 4 = full wave.
//
bool setAmplitude(uint8_t newAmp) {
  if (newAmp > 0 && newAmp <= 4) {
    waveAmplitude = newAmp;
    prefs.putUChar("a", waveAmplitude);
    return true;
  }
  return false;
}




/*
  Set a voltage on the DAC pin directly.. ( 0 - 3.3 )

  I discovered that if you slam a dacWrite(some voltage) into the DAC before starting a PWM
  rectangle/square wave signal, you can adjust the amplitude of that signal. Sort of.

  This is an experimental feature. We are doing things we probably shouldn't, and it may produce
  unexpected results.

  One result I didn't expect, when I set the voltage to 3.3; rather than get no change (3.3V should
  be the default), I get an increase (of brightness, or whatever). Hmm.

*/
String directDACVolts(float_t voltsValue) {
  if (voltsValue > 3.3) Serial.printf(" Value exceeds 3.3! Will be automatically limited to 3.3\n");
  uint16_t vV = min((int)(voltsValue * 100), 330); // map() uses integer math
  uint8_t newVal = map(vV, 0, 330, 0, 255);
  String ret = "Writing Mapped value (0-255): " + (String)newVal + " to DAC.\n";
  stopSignal();
  dacWrite(PWMPin, newVal);
  startSignal("DAC Voltage Adjust");
  return ret;
}





/*
  Step is from:

    touch (0)
    web command/button (1)
    serial console (2)
    web console (3)

  (there may yet be others, i.e. button, thought-control, who knows!?)
                                      */
void frequencyStepUP(int8_t stepSource) {

  if (eXi) Serial.printf(" Step UP() activation from: %s\n", stepFROM(stepSource).c_str());
  frequency += fStep;
  if (stepsObeyLimits) checkLimits(frequency);
  // the compiler will sort this out! (for readability..)
  if ((stepSource == 0 && storeTouch == true) || (stepSource >= 1 && storeButton == true)) {
    prefs.putFloat("f", frequency);
  }
  if (stepSource == 3) {
    LastMessage = "Frequency UP to " + makeHumanFrequency(frequency);
  }
  startSignal("StepUP");
}

//TODO create generic handler for all sources (with prefs save/not, etc.)
// or use better mech for f changes
// just remove fromWebConsole?

void frequencyStepDOWN(int8_t stepSource) {

  if (eXi) Serial.printf(" Step DOWN() activation from: %s\n", stepFROM(stepSource).c_str());
  frequency -= fStep;
  if (stepsObeyLimits) checkLimits(frequency);
  if ((stepSource == 0 && storeTouch == true) || (stepSource == 1 && storeButton == true)) {
    prefs.putFloat("f", frequency);
  }
  if (stepSource == 3) {
    LastMessage = "Frequency DOWN to " + makeHumanFrequency(frequency);
  }
  startSignal("StepDown");
}

// Helper function
// TODO: or add "from" other handlers
String stepFROM(int8_t stepSource) {
  String from;
  switch (stepSource) {
    case 0:
      from = "touch";
      break;
    case 1:
      from = "web";
      break;
    case 2:
      from = "console";
      break;
    case 3:
      from = "web console";
      break;
  }
  return from;
}


// Set the size of the PWM step.. (%)
bool setPulseStep(int8_t newStepSize) {
  if (newStepSize > 0 && newStepSize <= 100) {
    pStep = newStepSize;
    prefs.putUChar("j", pStep);
    return true;
  }
  return false;
}


// Set the pulse width percentage..
// we use int16_t for newPulse, as it might swing into the negative and we don't want it wrapping
// around (i.e. modulo behaviour).
int8_t setPulseWidth(int16_t newPulse, bool doSave = true) {
  if (newPulse > 100) newPulse = 100;
  if (newPulse < 0) newPulse = 0;
  pulse = newPulse;
  if (doSave) prefs.putUChar("p", pulse);
  return pulse;
}

void pwmStepUP(bool doSave = true) {
  setPulseWidth(pulse+pStep, doSave);
  startSignal("pwmUP");
}

void pwmStepDOWN(bool doSave = true) {
  setPulseWidth(pulse-pStep, doSave);
  startSignal("pwmDOWN");
}


void bitsStepUP(bool doSave = true) {
  PWMResBits = switchResolution(PWMResBits+1, doSave);
  startSignal("bitsStepUP");
}

void bitsStepDOWN(bool doSave = true) {
  PWMResBits = switchResolution(PWMResBits-1, doSave);
  startSignal("bitsStepDOWN");
}



// Generic "touch" handlers. We can set the touch to shift
// frequency, pulse width, or resolution bit depth.

void touchUPStep() {
  switch (touchMode) {
    case 'f' :
      frequencyStepUP(0);
      break;
    case 'p' :
      pwmStepUP(storeTouch);
      break;
    case 'b' :
      bitsStepUP(storeTouch);
      break;
  }
}

void touchDOWNStep() {
  switch (touchMode) {
    case 'f' :
      frequencyStepDOWN(0);
      break;
    case 'p' :
      pwmStepDOWN(storeTouch);
      break;
    case 'b' :
      bitsStepDOWN(storeTouch);
      break;
  }
}


/*
   Prefs & Presets..
                      */


void loadDefaultPrefs() {

  Serial.println("\n Retrieving preferences from NVRAM..\n");

  // Fire up NVS for preferences storage..
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) WipeNVRAM();
  ESP_ERROR_CHECK(err);

  /*
    TLDR: Keep preset names under 32 characters in length.

    Whenever you save a preset or name it, you use up NVRAM "entries".

    "What is an 'entry'?", you may ask. And this is a good question.

    Well, if you store a variable pair in NVS that is simply 'a'=='5' (char == char) you will use up
    one 'entry'. Otherwise; to quote the documentation..

      "For strings and blobs, this depends on value length."

    In other words, "how long is a piece of String?".

    Signal Generator uses the minimum amount of characters to store preset information; the only
    thing which will eat up your memory space quickly, is long "names". aka. The String.

    Basically, you want to keep the names of your presets short, so as not to use up 'entries':

    +--------+----------+----------+----------------+-----------+---------------+----------+
    | NS (1) | Type (1) | Span (1) | ChunkIndex (1) | CRC32 (4) |    Key (16)   | Data (8) |
    +--------+----------+----------+----------------+-----------+---------------+----------+

                                            Primitive  +--------------------------------+
                                            +-------->  |     Data (8)                   |
                                            | Types     +--------------------------------+
                      +-> Fixed length --
                      |                    |           +---------+--------------+---------------+-------+
                      |                    +-------->  | Size(4) | ChunkCount(1)| ChunkStart(1) | Rsv(2)|
        Data format ---+                    Blob Index  +---------+--------------+---------------+-------+
                      |
                      |                             +----------+---------+-----------+
                      +->   Variable length   -->   | Size (2) | Rsv (2) | CRC32 (4) |
                            (Strings, Blob Data)     +----------+---------+-----------+

    https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/storage/nvs_flash.html

    Testing reveals that one "entry" can hold 32 characters of String data (including null
    terminator), so you use one entry for every 32 characters. I can't imagine needing more than
    31 characters for a perfectly descriptive name!

    See elsewhere in this sketch for how to dramatically increase the size of your NVS.
*/

  // Begin the default Preferences instance..
  prefsSwitch();
  loadPrefs(false);
}



/*
   Read prefs from NVRAM for /current/ NameSpace.

   NOTE: NameSpace is always set /before/ calling, with prefsSwitch(x)..

                               */
void loadPrefs(bool isPreset) {

  // Signal settings..
  mode = prefs.getChar("m", mode);
  frequency =  prefs.getFloat("f", frequency);
  fStep = prefs.getFloat("s", fStep);
  pulse = prefs.getUChar("p", pulse);
  pStep = prefs.getUChar("j", pStep);
  PWMResBits = switchResolution(prefs.getUChar("b", PWMResBits), false);
  touchMode = prefs.getChar("h", touchMode);
  waveAmplitude = prefs.getUChar("a", waveAmplitude);

  // Global Switches..  (loaded once at INIT only)
  if (!isPreset) {

    eXi = prefs.getBool("e", eXi);
    if (eXi) Serial.printf(" Extended Information: Enabled\n");

    RemControl = prefs.getBool("r", RemControl);
    if (eXi) Serial.printf(" Remote Control: %s\n", RemControl ? "Enabled" : "Disabled");

    reportTouches = prefs.getBool("t", reportTouches);
    if (eXi) Serial.printf(" Report Touches: %s\n", reportTouches ? "Enabled" : "Disabled");

    onlyAP = prefs.getBool("w", onlyAP);
    if (eXi) Serial.printf(" AP Only Mode: %s\n", onlyAP ? "Enabled" : "Disabled");

    layerPresets = prefs.getBool("l", layerPresets);
    if (eXi) Serial.printf(" Layering Presets: %s\n", layerPresets ? "Enabled" : "Disabled");

    saveALL = prefs.getBool("c", saveALL);
    if (eXi) Serial.printf(" Save ALL Settings: %s\n", saveALL ? "Enabled" : "Disabled");

    exportALL = prefs.getBool("x", exportALL);
    if (eXi) Serial.printf(" Export ALL Settings: %s\n", exportALL ? "Enabled" : "Disabled");

    usePOT = prefs.getBool("u", usePOT);
    if (eXi) Serial.printf(" Use Potentiometer: %s\n", usePOT ? "Enabled" : "Disabled");

    loopCommands = prefs.getString("o", loopCommands);
    if (eXi && loopCommands != "") Serial.printf(" Default Loop/Macro: %s\n", loopCommands.c_str());

    cpuSpeed = prefs.getUInt("z", cpuSpeed);
    if (eXi) Serial.printf(" CPU Speed: %iMHz\n", cpuSpeed);

    Serial.printf("\n Touch UP Pin: %i\n Touch DOWN Pin: %i\n", touchUPPin, touchDOWNPin);
  }
}


/*
  If the code above confuses you, read on..

  It's because I used "The Ternary Operator" (aka. The Conditional Operator); a magical wee device
  that uses two characters (? and :) to do wonderful logic. While it can be mind-bending to
  comprehend when first seen in the wild, it's actually quite a simple construct, like this..

    braces optional   |---HERE! The 1st part   |--- HERE! The 2nd part
                      V                        V
  (condition to test) ? <do THIS if it's true> : <do THIS if it's false>

  SO, "Ternary", meaning, "having three parts", those parts being delimited by the ? and :

  Which is basically the same as doing:

    if (condition to test) {
      do THIS if it's true
    } else {
      do THIS if it's false
    }

  Except with *less code*. And that's just the bare-bones logic. Once you start adding variables and
  strings into the mix, things can get messy. The ternary operator enables you to seriously compress
  your code. In the MCU world, this matters. Look at this..

    eXi = eXi ? false : true;

  And boom! The extended info flag just got flipped. i.e. if it was true, it is now false, and if it
  was false it is now true.

  The /best/ thing about the ternary operator is that you can use it in all sorts of places where a
  regular if/then condition would be impossible; like inside return statements and printf commands;
  places which would instead require writing lots of extra code /before/ the command. You can even
  nest them. Think about it.

  You will find quite a few examples, below ...

 */


/*

  Copy all the settings from one preset to another preset.

  *Everything* is copied, including name and any loops/macros which might be stored in there.

  NOTE: If you make a copy of a copy, Signal Generator will number them so you don't get confused or
  eat up memory with "My Preset (COPY) (COPY) (COPY) (COPY) (COPY) (COPY) (COPY) (COPY) (COPY)" ...

                                                    */
String copyPreset(uint8_t copyFrom, uint8_t copyTo) {

  if (copyFrom < 1 || copyFrom > presetMAX) return "Source " + _OUT_OF_RANGE_;
  if (copyTo < 1 || copyTo > presetMAX) return "Target " + _OUT_OF_RANGE_;

  if (prefsSwitch(copyFrom)) {

    // Sure, we could just write from one to the other, but this could potentially waste "entries",
    // creating prefs that don't exist.
    char Cmode = prefs.getChar("m", '~');
    float_t Cfrequency =  prefs.getFloat("f", -1);
    float_t CfStep = prefs.getFloat("s", -1);
    char CtouchMode = prefs.getChar("h", '~');
    uint8_t CwaveAmplitude = prefs.getUChar("a", '~');
    uint8_t CPWMResBits = prefs.getUChar("b", '~');
    uint8_t Cpulse = prefs.getUChar("p", '~');
    uint8_t CpStep = prefs.getUChar("j", '~');
    String CloopCommands = prefs.getString("o", "");
    String CmyName = prefs.getString("n", "");
    String newName = "(COPY)";

    prefsSwitch(copyTo);
    prefs.putChar("i", 1);

    if (Cmode != '~') prefs.putChar("m", Cmode);
    if (Cfrequency != -1) prefs.putFloat("f", Cfrequency);
    if (CfStep != -1) prefs.putFloat("s", CfStep);
    if (CtouchMode != '~') prefs.putChar("h", CtouchMode);
    if (CwaveAmplitude != '~') prefs.putUChar("a", CwaveAmplitude);
    if (CPWMResBits != '~') prefs.putUChar("b", CPWMResBits);
    if (Cpulse != '~') prefs.putUChar("p", Cpulse);
    if (CpStep != '~') prefs.putUChar("j", CpStep);
    if (CloopCommands != "") prefs.putString("o", CloopCommands);

    // There is a name to copy..
    // This is the sort of code "no one" writes. erm..
    if (CmyName != "") {
      newName = CmyName + " " + newName;
      int8_t foundCpyPOS = CmyName.indexOf("(COPY");
      uint8_t cNum; // ESP32 max namespaces == 253!
      // This is already a copy.. *What Are You Playing At!?!*
      if(foundCpyPOS != -1) { // And here it is again using String functions..
        cNum = CmyName.substring(foundCpyPOS+5, CmyName.lastIndexOf(")")).toInt();
        newName = CmyName.substring(0, foundCpyPOS) + "(COPY" + (String)((cNum != 0) ? cNum += 1 : 2) + ")"; // Hah!
      }
      prefs.putString("n", newName);
    }
    prefsSwitch();
    return "Preset copied OK";
  }
  return "Source Preset Not Found!";
}



/*
  Load a Preset..

  Presets are very simply stored the exact same way as your regular preferences, except with
  "<number>" appended to the regular "sg" we normally use for a "NameSpace" name.

                                     */
String loadPreset(int8_t presetNumber) {

  if (presetNumber < 0 || presetNumber > presetMAX) return _OUT_OF_RANGE_;

  // If young programmers would only learn this one simple thing, it would change lives..
  bool presetExists = true;
  //
  // *ahem* young programmers; note how this variable is named after *exactly what it represents*
  // and not HoneyBunny or foo2 or mVar or x or whatthefuckelse. Your code may (unbelievably) be of
  // interest to *other* people, who have no clue what you mean by "funkDoodle". By "other people",
  // I also mean YOU, five or ten years from now, looking at code YOU wrote thinking WTF is going
  // on here?!? Trust me; memory is fickle. I can't even remember starting this sentence.
  //
  // What am I saying? In a decade there will be no more coders! Chat GPT et al FTW!
  //
  // I should add, once the code passes through the compiler, the variable names go POOF! Vanishing
  // into machine instructions, jumping around memory registers... Tokens. Readability will always
  // trump any other imagined improvements. Humans first!

  String retStr = "Loading ";

  // Preset 0 is special - it's the main preset (0 used internally only, there is no "sg0" preset)
  if (presetNumber == 0) {
    retStr += "Defaults: ";
  } else {
    // Check this preset exists..
    if (prefsSwitch(presetNumber)) {
      retStr += "Preset " + (String)presetNumber + " '" + prefs.getString("n") + "': ";
    } else {
      presetExists = false;
    }
  }

  // Load prefs as usual..
  if (presetExists) loadPrefs(true);

  // Back to regular prefs..
  if (presetNumber != 0) prefsSwitch();

  return (presetExists) ? retStr + "OK" : "No Such Preset!";
}



/*
  Save a preset to NVS..
  If preset number is 0, we save over main (default) prefs..
                                                              */
String savePreset(int8_t presetNumber, bool defaults = false) {

  if (presetNumber < 0 || presetNumber > presetMAX) return _OUT_OF_RANGE_;

  String ret = "Saving Defaults: ";

  // For layering, we save only settings which have been manually set (to defaults).
  // So we grab those (checking they exist) before we switch to the preset's preferences namespace.

  char lMode = prefs.getChar("m", '~');       // Mode
  float_t lFreq = prefs.getFloat("f", -1);    // Frequency
  float_t lStep = prefs.getFloat("s", -1);    // Frequency Step Size
  char lTcMd = prefs.getChar("h", '~');       // Touch Handler
  uint8_t lAMP = prefs.getUChar("a", '~');    // Wave Amplitude
  uint8_t lBits = prefs.getUChar("b", '~');   // Resolution Bit Depth
  uint8_t lPulse = prefs.getUChar("p", '~');  // Pulse Width (Duty Cycle)
  uint8_t lpStep = prefs.getUChar("j", '~');  // PWM Step Size

  if (presetNumber != 0) {
    ret = "Saving Preset " + (String)presetNumber + ": ";
    prefsSwitch(presetNumber);
    // Create an id so that in future we can check for a preset's existence..
    prefs.putChar("i", 1);
    // If it doesn't have an id (==1),
    // it's just an empty preset space (or rather, "NameSpace") which we will ignore.
  }

  if (layerPresets && !defaults) {

    // Layering uses the absolute minimum "entries" to save a preset..

    if (lMode != '~') prefs.putChar("m", lMode);
    if (lFreq != -1) prefs.putFloat("f", lFreq);
    if (lStep != -1) prefs.putFloat("s", lStep);
    if (lTcMd != '~') prefs.putChar("h", lTcMd);
    if ((mode != 'r' || saveALL) && lAMP != '~') prefs.putUChar("a", lAMP);
    if ((mode == 'r' || saveALL) && lBits != '~') prefs.putUChar("b", lBits);
    if ((mode != 's' || saveALL) && lPulse != '~') prefs.putUChar("p", lPulse);
    if ((mode != 's' || saveALL) && lpStep != '~') prefs.putUChar("j", lpStep);

  } else {

    // No layering - simply save "all" (/applicable) current settings..

    prefs.putChar("m", mode);
    prefs.putFloat("f", frequency);
    prefs.putFloat("s", fStep);
    prefs.putChar("h", touchMode);
    if (mode != 'r' || saveALL) prefs.putUChar("a", waveAmplitude);
    if (mode == 'r' || saveALL) prefs.putUChar("b", PWMResBits);
    if (mode != 's' || saveALL) prefs.putUChar("p", pulse);
    if (mode != 's' || saveALL) prefs.putUChar("j", pStep);
  }

  if (prefs.getChar("i") == 1) ret += "OK"; // Looks Good

  if (presetNumber != 0) {
    prefsSwitch();
  } else {
    ret += " (Saving current settings to DEFAULTS  - will persist on reboot).";
  }

  // Let user know how much NVS space is left..
  return ret + getFreeEntries();
}



/*
   Name a Preset..

   Give your preset a name and that name will appear as a pop-up title in your web controller page.
   As well as make it easier to find in a big list of presets.

                                         */
String namePreset(String presetNameData) {

  String presetNUM;
  String presetNAME;

  // When the stream encounters a non-number, this gets flipped..
  bool stillDigits = true;

  char tChar;
  int8_t success = -1;

  // We check each character. While we're still getting digits, we consider these characters to be
  // the numeric part (preset number). When the input switches to "=" or "not digit", we assume that
  // the "name" part has begun.

  // Yes, this stream parser came first, but only gets to chomp at two bytes.
  for( uint8_t i = 0; i < presetNameData.length(); i++ ) {

    // We set an arbitrary limit of 2 characters on the digit part of the command. Aww!
    //
    // If you set your maximum number of presets to >99, this code won't work.
    // But then, if you are using over a hundred presets, you ain't got space for names!
    if (i == 2) stillDigits = false; // Something more robust would be easy to code. Or remove this line.
    // This enables you to use numbers at the start of names for presets with 2 digits, e.g..

    //  n33500Hz wave

    // I know; it seems like a hobbling for such a lovely chunk of code. Och well.
    // Addendum: I got another chance to shake this about in the loop loading code. So, all good!

    // Let's examine *this* character of the input..
    tChar = presetNameData.charAt(i);

    // This enables you to use numbers at the start of names for presets 1-9, e.g. n1=1 bar foo
    // As well as *any* other preset number. Which is why this is also the format Signal Generator
    // uses for /exporting/ presets.
    if (tChar == '=') {
      stillDigits = false;
      continue;
    }

    // Check if it's a number..
    if (isDigit(tChar) and stillDigits) {
      presetNUM += tChar; // It's still a String
    } else {
      // We've switched to not-numbers now - this must be the *name* part of the command
      presetNAME += tChar;
      stillDigits = false;
    }
  }

  uint8_t NUM = presetNUM.toInt();
  if (NUM < 1 || NUM > presetMAX) success = -3;

  presetNAME.trim();
  if (presetNAME == "") success = -4;

  if (success == -1) {
    prefsSwitch(presetNUM.toInt());
    if (prefs.getChar("i", -1) != -1) {
      if (prefs.putString("n", presetNAME) != 0) success = 0;
    } else {
      success = -2;
    }
    prefsSwitch();
  }

  String result;

  switch (success) {
    case -1:
      result = "Could not set name of preset " + presetNUM;
      break;
    case -2:
      result = "Preset " + presetNUM + " does not exist.";
      break;
    case -3:
      result = _OUT_OF_RANGE_;
      break;
    case -4:
      result = "No name supplied!";
      break;
    default: // 0
      // NOTE: the web controller looks for this *exact* text to switch out the button titles
      // on-the-fly when you rename a preset with the built-in console. If you make changes to
      // the text here, you should also change the text in getCommandStatus(), inside WebPage.h
      result = "Set name of preset [" + presetNUM + "] to \"" + presetNAME + "\"";
  }

  return result;
}



/*
  Get Preset Name..

  Grab the "name" of a particular preset and return it as a String.

  This is used to replace the titles of the preset buttons with the name of your preset.
  Way cooler than lesser mortals might think.
                                         */
String getPresetName(uint8_t thisPreset) {
  prefsSwitch(thisPreset);
  String myName = prefs.getString("n", "");
  prefsSwitch();
  return myName;
}



/*
  Wipe a Preset.

  Wipe either the current defaults or a specified preset.

  NOTE: This does not remove the namespace; simply empties it.

  FYI: There is currently NO way to delete /only/ a namespace.
       If you want it truly gone, you must wipe the entire NVRAM. Deal with it.

                                  */
bool wipePreset(int8_t presetNumber) {

  if (presetNumber > presetMAX) return false;

  bool cleared = false;

  if (presetNumber != 0) prefsSwitch(presetNumber);

  // It may already be cleared..
  if (presetEmpty()) {

    cleared = false;

  } else {

    // Check this preset exists. If so, wipe it.
    if (prefs.getChar("i", -1) != -1) {

      // Loops stored here, remove individual keys..
      String wLoops = prefs.getString("o", "");
      if (wLoops != "") {
        prefs.remove("m");
        prefs.remove("f");
        prefs.remove("s");
        prefs.remove("h");
        prefs.remove("a");
        prefs.remove("b");
        prefs.remove("p");
        prefs.remove("j");
        cleared = true;

      } else {

        // No loops here. We can do a proper clear().
        cleared = prefs.clear();
      }

    } else {

      // No such preset..
      cleared = false;
    }

  }
  if (presetNumber != 0) {
    prefsSwitch();
  } else {
    prefs.putChar("i", 1); // Now it's a "real" Pinocchio, erm, preset.
  }

  return cleared;
}



/*
  Check if there are any settings in a preset (used for loop/preset wipes).

  Remember to switchPrefs(*) before you call this.

  Returns true if the /current/ preset namespace is empty of signal settings (of which there are 8).
  See inside loop() for a list of which letter means what.

                   */
bool presetEmpty() {
  bool isEmpty = true;
  if (prefs.getChar("m", '~') != '~') isEmpty = false;
  if (prefs.getFloat("f", -1) != -1) isEmpty = false;
  if (prefs.getFloat("s", -1) != -1) isEmpty = false;
  if (prefs.getChar("h", '~') != '~') isEmpty = false;
  if (prefs.getUChar("a", '~') != '~') isEmpty = false;
  if (prefs.getUChar("b", '~') != '~') isEmpty = false;
  if (prefs.getUChar("p", '~') != '~') isEmpty = false;
  if (prefs.getUChar("j", '~') != '~') isEmpty = false;
  return isEmpty;
}


// Regular if(char) mostly works well enough. Still..
bool isEmptyChar(const char **myChar) {
   if (*myChar && !*myChar[0]) {
    return true;
  }
  return false;
}




/*
  URL Decode..

  If you use spaces and weird characters in your preset names sent over the web, they will be URL-
  Encoded automatically by your browser; so we use this to decode them back to plain text.

  In fact, we run *all* web commands through this function.

  Original: ~/.arduino15/packages/esp32/hardware/esp32/2.x/libraries/WebServer/src/Parsing.cpp

  Its weakness is that it will balk on single "%" characters.

  If you must put "%" characters in your preset names, it's best to use the serial terminal to do
  it. There are great serial terminals on ALL platforms, so no excuse.

  Or else use the built-in web console (maybe the one in the title of the main page), which
  automatically (compensates for the non-AI behaviour of modern browsers and) encodes input "%"
  characters to "%25", which /this/ function will then decode back to "%", for processing.

  Boom! A probability ratio of 1:1 has been achieved! **

                              */
String urlDecode(String text) {
  String decoded = "";
  char temp[] = "0x00";
  uint16_t len = text.length();
  uint16_t i = 0;
  while (i < len) {
    char decodedChar;
    char encodedChar = text.charAt(i++);
    if ((encodedChar == '%') && (i + 1 < len)) {
      temp[2] = text.charAt(i++);
      temp[3] = text.charAt(i++);
      decodedChar = strtol(temp, NULL, 16);
    } else {
      if (encodedChar == '+') {
        decodedChar = ' ';
      } else {
        decodedChar = encodedChar;  // normal ascii char
      }
    }
    decoded += decodedChar;
  }
  return decoded;
}

/*
 ** Forgive me; I'm currently introducing the eldest to the wonders of Douglas Adams.
    (as bedtime story)
 */



// Trim Delimiters from the start and end of command strings..
//
// I don't care how many CPU cycles we use, let's do it RIGHT!
void trimDelims(String &Commands) {

  // Start with simple trim() or else following remove(s) may fail..
  Commands.trim();

  // We addeth these and then we taketh them away..
  while (Commands.endsWith(commandDelimiter)) {
    Commands.remove(Commands.length()-1);
    Commands.trim();
  }
  // Also remove any pesky preceding delimiters..
  while (Commands.startsWith(commandDelimiter)) {
    Commands.remove(0, 1);
    Commands.trim();
  }
}



/*
  Remove trailing zeros after the decimal point of a frequency value (e.g. 20000.00 > 20000)
  Fortunately, it's already a String. Yeehaw! We can use String() functions!
  If there's no "." in the String, we simply return the original number.

  I challenge you to produce a more compact function!

  Strings FTW!
                                    */
String chopZeros(String bigNumber) {
  if (bigNumber.indexOf(".") != -1) {
    while (bigNumber.endsWith("0")) bigNumber.remove(bigNumber.length()-1);
    if (bigNumber.endsWith(".")) bigNumber.remove(bigNumber.length()-1);
  }
  return bigNumber;
}



/*
  Return the current settings as a String..

  (to be printed out via serial console / web / AJAX )
                            */
String getCurrentSettings() {

  char bpl;
  if (PWMResBits != 1) bpl = 's';

  String wForm = "@";
  if (printWaveSymbols) {
    switch (mode) {
      case 's' :
        wForm = "\u223F"; //  ∿
        break;
      case 'r' :
        wForm = "\u25FB"; //  ◻
        break;
      case 't' :
        wForm = "\u25B3"; //  △
    }
  }
  char buffer[164];
  sprintf(buffer, "\t%s Wave %s %s\n", \
    makeHumanMode(mode).c_str(), wForm.c_str(), makeHumanFrequency(frequency).c_str());
  if (mode != 's') sprintf(buffer + strlen(buffer), "\tPulse Width: %i%%\n", pulse);
  // Resolution should not matter for Triangle wave. No really. Hmm.
  if (mode == 'r') sprintf(buffer + strlen(buffer), "\tPWM Resolution: %i bit%c\n", PWMResBits, bpl);
  sprintf(buffer + strlen(buffer), "\tFreq Step Size: %s\n", makeHumanFrequency(fStep).c_str());
  if (mode != 's') sprintf(buffer + strlen(buffer), "\tPWM Step Size: %i%%\n", pStep);
  if (mode != 'r') sprintf(buffer + strlen(buffer), "\tAmplitude Level: %i\n", waveAmplitude);
  sprintf(buffer + strlen(buffer), "\tTouch Mode: %s\n", makeHumanTouchMode(touchMode).c_str());

  return (String)buffer;
}

// The TABs ("\t") / NewLines ("\n") are converted to <div> / </div> elements via JavaScript on the main page.




/*

 Return the available commands as a String..

 aka. "Help".
                     */
String getCommands() {
  char cbuf[3000];
  sprintf(cbuf, "\n Commands:\n\n");
  sprintf(cbuf + strlen(cbuf), "\ts              Sine Wave\n");
  sprintf(cbuf + strlen(cbuf), "\tr              Rectangle / Square Wave\n");
  sprintf(cbuf + strlen(cbuf), "\tt              Triangle / Sawtooth Wave\n");
  sprintf(cbuf + strlen(cbuf), "\t'              Cycle through Rectangle > Sine > Triangle Waves\n");
  sprintf(cbuf + strlen(cbuf), "\t*[k/m]         Frequency [Hz/kHz/MHz]\n");
  sprintf(cbuf + strlen(cbuf), "\t+/-*[k/m]      Increase/Decrease Frequency by *[Hz/kHz/MHz]\n");
  sprintf(cbuf + strlen(cbuf), "\tb*             Resolution Bit Depth [1-10]\n");
  sprintf(cbuf + strlen(cbuf), "\tp*             Pulse Width (Duty Cycle ~ percent[0-100]) \n");
  sprintf(cbuf + strlen(cbuf), "\ts*[k/m]        Step size for Frequency [Hz/kHz/MHz]\n");
  sprintf(cbuf + strlen(cbuf), "\tj*             Step size for Pulse Width (Jump!) [1-100]\n");
  sprintf(cbuf + strlen(cbuf), "\th[f|p|b]       Touch Handler [frequency|pulse width|res bits]\n");
  sprintf(cbuf + strlen(cbuf), "\t[ ]            Frequency Step DOWN / UP\n");
  sprintf(cbuf + strlen(cbuf), "\t\\ /            Pulse Width Step DOWN / UP (also - and +)\n");
  sprintf(cbuf + strlen(cbuf), "\tz a            Resolution Bit Depth Step DOWN / UP\n");
  sprintf(cbuf + strlen(cbuf), "\ta*             Sine/Triangle wave Amplitude * (1-4) Default is 4.\n");
  sprintf(cbuf + strlen(cbuf), "\t*n[|o]         That's a literal *! Play musical note 'n' at octave 'o'\n");
  sprintf(cbuf + strlen(cbuf), "\t~[*]           Delay for * milliseconds (omit number to use previous time).\n");
  sprintf(cbuf + strlen(cbuf), "\td              Overwrite Defaults (with current settings)\n");
  sprintf(cbuf + strlen(cbuf), "\tm*             Save Current Defaults to Preset Memory *\n");
  sprintf(cbuf + strlen(cbuf), "\tl*             Load Preset Number * (Think: MEMORY -> LOAD!)\n");
  sprintf(cbuf + strlen(cbuf), "\tlist           List All Presets (and their settings - url: /list)\n");
  sprintf(cbuf + strlen(cbuf), "\tlp[e/d]        Toggle Layering of Presets [enable/disable]\n");
  sprintf(cbuf + strlen(cbuf), "\t[enter]        Print Current Settings (and restart generator)\n");
  sprintf(cbuf + strlen(cbuf), "\t,              Print Stored Default Settings (from NVS)\n");
  sprintf(cbuf + strlen(cbuf), "\tk*             Print Stored Settings for Preset Number *\n");
  sprintf(cbuf + strlen(cbuf), "\to*[k/m]        Override Limits, Set Frequency to *[Hz/kHz/MHz]\n");
  sprintf(cbuf + strlen(cbuf), "\te              Toggle Extended Info\n");
  sprintf(cbuf + strlen(cbuf), "\tx              Reboot Device\n");
  sprintf(cbuf + strlen(cbuf), "\tl              Load Stored Default Signal Settings\n");
  sprintf(cbuf + strlen(cbuf), "\tbuttons        Print Out Current Physical Button Control Assignments\n");
  sprintf(cbuf + strlen(cbuf), "\tsa[e/d]        Toggle Save ALL Settings [enable/disable]\n");
  sprintf(cbuf + strlen(cbuf), "\tea[e/d]        Toggle (Individual) Export ALL Settings [enable/disable]\n");
  sprintf(cbuf + strlen(cbuf), "\trt[e/d]        Toggle the Reporting of Touches [enable/disable]\n");
  sprintf(cbuf + strlen(cbuf), "\tup[e/d]        Toggle Use Potentiometer Control [enable/disable]\n");
  sprintf(cbuf + strlen(cbuf), "\tv*             Set the Handler/Accuracy of the Potentiometer [p|f|b/1-100]\n");
  sprintf(cbuf + strlen(cbuf), "\treset          Reset to Hard-Wired Defaults (and reboot)\n");
  sprintf(cbuf + strlen(cbuf), "\tw[*]           Wipe Stored Signal Settings [for Preset *]\n");
  sprintf(cbuf + strlen(cbuf), "\twipe           WIPE ENTIRE NVRAM (and reboot). Careful now!\n");
  sprintf(cbuf + strlen(cbuf), "\texport[*/all]  Export Importable Settings [for preset/all]\n");
  sprintf(cbuf + strlen(cbuf), "\timport *       Please Read The Fine Manual!\n");
  sprintf(cbuf + strlen(cbuf), "\tloop[*]=[?]    Load [?] Commands into Loop/Macro [number *] (RTFM!)\n");
  sprintf(cbuf + strlen(cbuf), "\tloop[*]        Start Playing Loop/Macro [number *] (RTFM!)\n");
  sprintf(cbuf + strlen(cbuf), "\tend            End the Currently Playing Queue/Loop/Macro\n");
  sprintf(cbuf + strlen(cbuf), "\t@*             Set * Volts directly on the DAC Pin (0 - 3.3).\n");
  sprintf(cbuf + strlen(cbuf), "\tstop/.         Stop the Currently Playing Signal (enter to restart)\n");
  sprintf(cbuf + strlen(cbuf), "\tll[l]          List Loops/Macros (if available) [single importable list]\n");
  sprintf(cbuf + strlen(cbuf), "\tmem            Print Out Memory Usage Information\n");
  sprintf(cbuf + strlen(cbuf), "\tcpu*           Set CPU Frequency to *[240/160/80] MHz%s\n", \
                                                                   RemControl ? " (and reboot)" : "");
  sprintf(cbuf + strlen(cbuf), "\tremote[e/d]    Remote Control Toggle [Enable/Disable]\n");
  sprintf(cbuf + strlen(cbuf), "\twap/waa        Set WiFi AP Only / Station + AP (and reboot)\n");
  sprintf(cbuf + strlen(cbuf), "\tc              Print Out Available Commands (this screen - url: /help)\n");
  return (String)cbuf;
}


/*
  Print contents of NVS/NVRAM (settings); either the default settings or for a preset.

  Don't assign anything, just print out what's stored in the NVRAM..

  During very first init, we silently run through all 50* presets so that the NameSpace memory is
  pre-allocated and the user doesn't get confused when they list their presets for the first time
  (and 50* entries "vanish"). It also means when saving presets the change in the number of entries
  reflects the actual settings. It only takes a moment.

  * or whatever you set in your prefs
                                                                                   */
String getNVRAMPresetData(uint8_t presetNumber = 0, bool listing = false, bool silent = false) {

  char thisType[16];

  if (presetNumber != 0) {
    prefsSwitch(presetNumber);
    if (!silent) {
      // Check this preset exists..
      if (prefs.getChar("i", -1) == -1) {
        if (!listing) LastMessage = "Preset " + (String)presetNumber + " does not exist";
        prefsSwitch();
        return "";
      }
      sprintf(thisType, " for Preset %i", presetNumber);
    }
  }

  char nvbuf[1024] = {'\0'}; // Prevents re-listings of non-existent presets (with loop data inside)
  if (!silent) {             // as the array is still there in the stack.
    if (presetNumber != 0) {
      if (!presetEmpty()) {
        sprintf(nvbuf, "\n NVRAM Settings%s: ", thisType);
        String nTmp = prefs.getString("n", "");
        if (nTmp != "") {
          sprintf(nvbuf + strlen(nvbuf), "\n\n\t\"%s\"\n\n", nTmp.c_str());
        } else {
          sprintf(nvbuf + strlen(nvbuf), "\n\n");
        }
      }
    } else {
      sprintf(nvbuf, "\n\tDefault NVRAM Settings:\n\n");
    }

    char bpl;
    if (prefs.getUChar("b") != 1) bpl = 's';

    // Only collect prefs that exist..
    if (prefs.getChar("m", '~') != '~') {
      sprintf(nvbuf + strlen(nvbuf), "\tGenerator Mode:\t\t%s Wave\n", makeHumanMode(prefs.getChar("m")).c_str());
    }
    if (prefs.getFloat("f", -1) != -1) {
        sprintf(nvbuf + strlen(nvbuf), "\tFrequency:\t\t%s\n", makeHumanFrequency(prefs.getFloat("f")).c_str());
    }
    if (prefs.getUChar("a", '~') != '~') {
      sprintf(nvbuf + strlen(nvbuf), "\tAmplitude Level:\t%i \n", prefs.getUChar("a"));
    }
    if (prefs.getFloat("s", -1) != -1) {
      sprintf(nvbuf + strlen(nvbuf), "\tFrequency Step:\t\t%s \n", makeHumanFrequency(prefs.getFloat("s")).c_str());
    }
    if (prefs.getUChar("p", '~') != '~') {
      sprintf(nvbuf + strlen(nvbuf), "\tPulse Width:\t\t%i%% \n", prefs.getUChar("p"));
    }
    if (prefs.getUChar("j", '~') != '~') {
      sprintf(nvbuf + strlen(nvbuf), "\tPWM Step:\t\t%i%% \n", prefs.getUChar("j"));
    }
    if (prefs.getUChar("b", '~') != '~') {
      sprintf(nvbuf + strlen(nvbuf), "\tPWM resolution:\t\t%i bit%c\n", prefs.getUChar("b"), bpl);
    }
    if (prefs.getChar("h", '~') != '~') {
      sprintf(nvbuf + strlen(nvbuf), "\tTouch Mode:\t\t%s\n", makeHumanTouchMode(prefs.getChar("h")).c_str());
    }

    if (eXi && !listing) sprintf(nvbuf + strlen(nvbuf), getFreeEntries().c_str());
  }

  if (presetNumber != 0) prefsSwitch();
  return (String)nvbuf;
}



/*
  Return the list of presets and their settings as a String.

  The first time you run this, 50* NameSpaces get created, using 50 "entries".
  These are your 50 available presets. (*or whatever number you set in your prefs, above)

  This function needs to be below getNVRAMPresetData() or Arduino IDE prototype generator pukes.

                                     */
String listPresets(bool init = false) {
  String xMSG, tmpString;
  for( uint8_t i = 1; i <= presetMAX; i++ ) xMSG += getNVRAMPresetData(i, true, init);
  xMSG = (xMSG != "") ? "Listing Preset Memories..\n" + xMSG : "No Presets Exist!\n";
  if (!init) return xMSG + getFreeEntries();
  return getFreeEntries();
}



/*
  Return the stored loops/macros as a String (command: ll / lll)

  Output can be imported directly into Signal Generator..

    ll

  for individual lines (one for each stored loop/macro)

    lll

  for one long line containing all loops/macros so you can import them all-at-once.

                                 */
String listLoops(bool oneLine) {

  String xMSG, loopEnd = "\n", divider = "";
  if (oneLine) {
    loopEnd = commandDelimiter;
    divider = "\n";
  }
  String sLoops = prefs.getString("o", "");
  if (sLoops != "") xMSG = " loop=" + sLoops + loopEnd;

  for( uint8_t i = 1; i <= presetMAX; i++ ) {
    prefsSwitch(i);
    sLoops = prefs.getString("o", "");
    if (sLoops != "") xMSG += " loop" + (String)i + "=" + sLoops + loopEnd;
  }

  prefsSwitch();
  return (xMSG != "") ? "Listing Stored Loops/Macros..\n\n" + xMSG + divider + getFreeEntries(): "No loops found!";
}



/*

  End the currently running queue/loop/macro..

                */
void endLoop() {
  iLooping = false; // Switch the looping flag
  QCommand = "";   // Remove remaining commands from the queue
  eXi = eXiTmp;   // Put extended info flag back to what it was before we started looping
}




// It's best to access this stuff directly, get /all/ the numbers.
// https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/storage/nvs_flash.html
String getFreeEntries() {
  char nvbuf[64];
  nvs_stats_t nvs_stats;
  nvs_get_stats(NULL, &nvs_stats);
  sprintf(nvbuf, "\n Used %d of %d total NVS entries (%d free)",
            nvs_stats.used_entries, nvs_stats.total_entries, nvs_stats.free_entries);
  return (String)nvbuf;
}



/*
  Wipe NVRAM/NVS  aka. "Non Volatile Storage".

  If you have been working with other sketches that use NVS for "permanent" storage, before you
  start saving setting in Signal Generator* you will probably want to run code something like this.

  You can do this from the serial console, with the command: wipe
                    */
void WipeNVRAM() {
  esp_err_t ret = nvs_flash_init();
  ESP_ERROR_CHECK(nvs_flash_erase()); // The actual wipe
  ret = nvs_flash_init();
  ESP_ERROR_CHECK(ret);
}




/*
   Web Server functions.
                          */

#if defined REMOTE

void startServer() {

  // We use "unsigned long" for time because the device may be running for a /long/ time.
  uint32_t currentTime = millis();
  uint32_t startTime = currentTime;

  bool WebControl = true;
  bool APControl = true;

  if (eXi && softAPSSID == "") Serial.println("\n Soft Access Point Disabled.");

  if (onlyAP && softAPSSID != "") {

    WiFi.mode(WIFI_AP);
    if (!isEmptyChar(&SGhostName)) WiFi.setHostname(SGhostName);

    if (eXi) Serial.println("\n Running in Access Point Only Mode.");

  } else {

    WiFi.mode(WIFI_AP_STA); // The default
    if (!isEmptyChar(&SGhostName)) WiFi.setHostname(SGhostName);

    if (!isEmptyChar(&ssid)) {

      Serial.printf("\n Connecting to %s", ssid);
      WiFi.begin(ssid, password);

      while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print("."); // debug info in your console
        currentTime = millis();
        if (currentTime > (startTime + (timeOut * 1000))) {
          Serial.printf("\n\n Connecting to %s FAILED.\n", ssid);
          WebControl = false;
          break;
        }
      }
      if (WiFi.status() == WL_CONNECTED) {
        Serial.print(" Success!\n Local WiFi Web Address: http://"); // I'd love to use printf here!
        Serial.print(WiFi.localIP());
        Serial.println("/");
      }
    } else { WebControl = false; }
  }

  // Access Point..
  if (RemControl && softAPSSID != "") {

    // You can customise the IP address.. (default is 192.168.4.1)
    IPAddress local_IP(192,168,4,1); // NOTE: commas.
    IPAddress gateway(192,168,4,1);
    IPAddress subnet(255,255,255,0);
    WiFi.softAPConfig(local_IP, gateway, subnet);

    if (WiFi.softAP(softAPSSID, softAPPass)) {
      Serial.print("\n WiFi Access Point Established.\n Connect your WiFi to ");
      Serial.println("\"" + softAPSSID + "\".");
      Serial.print(" Once connected to the AP, point your browser to: http://");
      Serial.print(WiFi.softAPIP());
      Serial.println("/");
    } else { APControl = false; }
  } else { APControl = false; }

  /*

  For debugging, I recommend you edit your WebServer.cpp file (line 296-ish) to instead read..

    log_v("New client: client.remoteIP()=%s", client.remoteIP().toString().c_str());

    i.e. remoteIP instead of localIP, which is information we already-the-f*ck have.
    (see sendWebConsole() for how to access remote IP from /inside/ a sketch)

  */

  // OKAY, we have at least one web interface running..
  //
  if (APControl || WebControl) {

    if (!isEmptyChar(&SGhostName)) {
      Serial.println("\n You can also access your Signal Generator here: http://" + (String)WiFi.getHostname() + "/");
    }

    Serial.println();
    Serial.println(" The Web Console is at /console");
    Serial.println(" To see a list of available commands, send: c");
    Serial.println();
    Serial.println(" Load a simple two-button frequency up/down interface at /simple");
    Serial.println(" Or the same page, but instead controlling Pulse Width at /pwm or Resolution at /bits");
    Serial.println();
    Serial.println(" You can send *any* command as a simple URL, e.g. /p25 to set Pulse Width to 25%");
    Serial.println();
    Serial.println(" Some text/plain URLs you may find useful: /wave /frequency /status /list /help");
    Serial.println();

    // Flash LED to notify us that remote control is up
    pinMode(led, OUTPUT); // Probably not actually necessary for built-in LED.
    for (uint8_t i = 0; i < 3; i++) {
      digitalWrite(led, HIGH);
      delay(100);
      digitalWrite(led, LOW);
      delay(50);
    }

    // WiFi.printDiag(Serial); // Print WiFi connexion to serial interface (SSID, password, etc.)


    // Setup URL handling for web server..

    server.on("/", sendRoot);

    /*
    To hell with reloading the page on every click!

    We load the main page ONE TIME and use AJAX Requests for "everything".

    If you know AJAX, move on. If not, /read/ on..

    Once you wrap your head around what's going on, AJAX is super-simple.

    So, your browser uses Javascript** to send web requests, just like you might with an actual web
    browser. This happens in the background, in its own "thread" when you click something, so you
    can do other stuff in the page while the AJAX request waits for a "response". You can try any of
    the AJAX URLs in a regular web browser and see what your ESP32 is sending back. NOTE: Some won't
    work correctly unless you also send arguments, e.g..

      /setFreqency?frequency=2k

    (note: with Signal Generator, "/2k" would achieve the same effect. Anyways ...)

    The server (your ESP32 device) /responds/ * with a message, it's just plain text, but it can be
    used for most anything. For example, if you navigate your web browser to:

      http://<Your-Signal-Generator-IP>/frequency

    You should get back the current frequency, in human-readable format. Or /status and more.

    Your browser (again using the magic of JavaScript) uses this "response" as data; perhaps placing
    these data into a designated HTML <element> with a specific id="string-we-use-to-find-this-
    element". Or some other change.

    This facility enables us to replace the contents of web page's HTML elements (usually just
    text) *in-situ*, so you can see the result of what you did, i.e. click a button, as-it-happens.
    For example, if you did "n9=My Groovy Preset", the response to the rename command will be
    intercepted and used to switch the title of the button for the preset you just renamed.

    This is how we created "Web 2.0". By the way.

    I have tried to provide a readable template for this sort of malarkey in ESP32. See the pages
    themselves (WebPage.h and Console.h) for the JavaScript side of the magic (Them's Elves!).

    * Of course, the *real* "response" is what the server (your EPS32 device) actually does in
    /response/ to your commands, but we are using "response" as a technical term here*.

    * Like "referer". *sigh*

    ** JavaScript, aka. "ECMAScript", is a code interpreter that runs in the background in your
    web browser. There are many wonderful things about the pig that is JavaScript, but in the MCU
    context, the most important thing to note is that the code runs *in the browser* and *not on
    the ESP32 device*, so anything you do in JavaScript costs you zero CPU; it's all happening in
    the client's web browser. In other words, Go Nuts!

    */

    /*
      These are all the URLs the server "knows" how to respond to; because we explicitly tell it.
      Every other request is treated as a "WebCommand" and handled inside loop().

      Having a proper handler function enables us to provide a web-specific response to a command.

      Handler functions will either complete the command or else populate webCommand the same way
      plain /urls do (to be picked up and processed inside loop()). Either way, we have the ability
      to a provide a proper web/AJAX response back to the web client (maybe an AJAX request).

      NOTE: URLs are CaSe-SeNsiTive.

                                                  */
    server.on("/setFreqency", handleFreqencyChange);
    server.on("/setStep", handleStepChange);
    server.on("/setBitDepth", handleBitDepthChange);
    server.on("/setPulseWidth", handlePulseWidthChange);

    server.on("/setUP", handleFreqencyUP);
    server.on("/setDOWN", handleFreqencyDOWN);

    server.on("/pulseUP", handlePulseUP);
    server.on("/pulseDOWN", handlePulseDOWN);

    server.on("/resUP", handleResUP);
    server.on("/resDOWN", handleResDOWN);

    server.on("/setSquare", handleSetSquareWave);
    server.on("/setSine", handleSetSineWave);
    server.on("/setTriangle", handleSetTriangleWave);
    server.on("/wave", handleUpdateWaveMode);
    server.on("/frequency", handleUpdateFrequency);
    server.on("/status", handleStatus);
    server.on("/reboot", handleReboot);

    server.on("/loadPreset", handleLoadPreset);
    server.on("/savePreset", handleSavePreset);

    // WebConsole holy sh*t!
    server.on("/console", sendWebConsole);
    server.on("/LastMessage", handleLastMessage);

    // A simple up/down page
    server.on("/simple", sendSimplePage);
    server.on("/freq", sendSimplePage);
    // Easily adapted to other up/down uses..
    server.on("/pwm", sendSimplePage);
    server.on("/bits", sendSimplePage);

    // In case someone accesses AJAX/command URLs directly ..
    server.on("/favicon.ico", handleFavicon);
    // Yes!

    // It's trivial to setup handlers for links folk might use directly.
    // Instead of "OK", we can instead send what they would /expect/..

    server.on("/help", sendHelpPage);
    server.on("/c", sendHelpPage);

    // Proper Case variants for readable lists on non-compliant browsers (Bromite, et al).
    // Rather than text/plain, it's text/html, with output inside <pre></pre> tags. *sigh*
    server.on("/Help", sendHelpPageHTML);
    server.on("/C", sendHelpPageHTML);

    // ditto..
    server.on("/list", sendListPage);
    server.on("/List", sendListPageHTML);
    server.on("/presets", sendListPage);
    server.on("/Presets", sendListPageHTML);

    // End a loop/macro from the web..
    server.on("/end", handleEndLoop);

    // 404 errors.. Or are they!?! What magic awaits..
    server.onNotFound(handleWebConsole);

    // Fire up the server..
    server.begin();
    Serial.println(" Web Server Started. Enjoy!");

  } else {
    Serial.println("\n\n Connecting to WiFi FAILED.\n Web controls disabled.");
  }

}


/*
  Handle web client requests/actions..
                                      */

// Send Main page..
// Note: if you already have the page open in a browser, there's no need to reload it when you
// reboot (which will use memory - not an issue as we have plenty, still..). Everything is AJAX,
// so you can just click-and-go. Same story for the "simple" pages.
void sendRoot() {

  // HTML you can edit..
  #include "WebPage.h"

  // This will be included at the preprocessor stage (if you have wifi remote enabled), but works
  // well enough inside here and keeps things clearer for me and saves memory. Test it!

  // WebPage is a but a simple String created in WebPage.h, which does nothing else.
  // (which is why it's fine to include it /inside/ a function - same for /console)

  // You can have fun right here with all sorts of web page variable tokens..
  WebPage.replace("{Version}", version);
  /*
    So "{Version}" (no quotes) in your HTML file is replaced with the contents of the variable
    "version", which we set way up near the top of this sketch. So now the title of the page
    reflects the current version, without needing to edit the HTML.

    I wonder how much memory this uses? Anyway, ESP32 does have a *lot*.

    It certainly doesn't affect our (in)ability to access Signal Generator when running 20MHz
    signals. So enjoy! And feel sorry for your poor Arduino brethren, working in near-zero-k.
  */

  WebPage.replace("{1}", getPresetName(1));
  WebPage.replace("{2}", getPresetName(2));
  WebPage.replace("{3}", getPresetName(3));
  WebPage.replace("{4}", getPresetName(4));
  WebPage.replace("{5}", getPresetName(5));
  WebPage.replace("{6}", getPresetName(6));
  WebPage.replace("{7}", getPresetName(7));
  WebPage.replace("{8}", getPresetName(8));
  WebPage.replace("{9}", getPresetName(9));
  /*
    Yes! The name of your presets will appear as a pop-up title/balloon/whatever over the
    corresponding button. Pretty neat, and obviously, if you had the need, could be expanded to
    *more* buttons. Way faster than I expected.
*/
  // Serving the actual page is ridiculously easy..
  server.send(200, _HTML5_TEXT_, WebPage);
  // See also: sendSimplePage()

  if (eXi) Serial.printf(" HTTP Request: Main Page for client @ %s\n", \
                          server.client().remoteIP().toString().c_str());
}


/*
    Web Commands.

    These are fired from clicks in the main web page (AJAX).

    The comment above the function is the actual request /url

                                                                */
/*
/wave                     */
void handleUpdateWaveMode() {
  server.send(200, _PLAIN_TEXT_, makeHumanMode(mode));
  if (eXi) Serial.println(" HTTP Request: Get Waveform");
}

/*
/frequency                   */
void handleUpdateFrequency() {
  server.send(200, _PLAIN_TEXT_, makeHumanFrequency(frequency));
  if (eXi) Serial.println(" HTTP Request: Get Frequency");
}

/*
/setFreqency                */
void handleFreqencyChange() {
  for (uint8_t i = 0; i < server.args(); i++) {
    if (server.argName(i) == "frequency") {
      WebCommand = server.arg(i);
      break;
    }
  }
  server.send(200, _PLAIN_TEXT_, "Changing Frequency..");
  if (eXi) Serial.println(" HTTP Request (Frequency Change): " + WebCommand);
}

/*
/setStep                */
void handleStepChange() {
  for (uint8_t i = 0; i < server.args(); i++) {
    if (server.argName(i) == "step") {
      WebCommand = "f" + server.arg(i);
      break;
    }
  }
  server.send(200, _PLAIN_TEXT_, "Changing Frequency Step..");
  if (eXi) Serial.println(" HTTP Request (Step Size Change): " + WebCommand);
}

/*
/setBitDepth                */
void handleBitDepthChange() {
  String iData;
  for (uint8_t i = 0; i < server.args(); i++) {
    if (server.argName(i) == "bits") {
      iData = server.arg(i);
      WebCommand = "b" + iData;
      break;
    }
  }
  char bpl;
  if (iData.toInt() != 1) bpl = 's';
  server.send(200, _PLAIN_TEXT_, "Changing Resolution..");
  if (eXi) Serial.printf(" HTTP Request: Bit Depth. %s bit%c\n", iData.c_str(), bpl);
}

/*
/setPulseWidth                */
void handlePulseWidthChange() {
  for (uint8_t i = 0; i < server.args(); i++) {
    if (server.argName(i) == "pulse") {
      WebCommand = "p" + server.arg(i);
      break;
    }
  }
  server.send(200, _PLAIN_TEXT_, "Changing Pulse Width..");
  if (eXi) Serial.println(" HTTP Request:" + WebCommand);
}

/*
/setUP                  */
void handleFreqencyUP() {
  frequencyStepUP(1);
  String mf = makeHumanFrequency(frequency);
  server.send(200, _PLAIN_TEXT_, "Frequency UP to " + mf + recState());
  if (eXi) Serial.printf(" HTTP Request: Frequency UP to %s\n", mf.c_str());
}

/*
/setDOWN                  */
void handleFreqencyDOWN() {
  frequencyStepDOWN(1);
  String mf = makeHumanFrequency(frequency);
  server.send(200, _PLAIN_TEXT_, "Frequency DOWN to " + mf + recState());
  if (eXi) Serial.printf(" HTTP Request: Frequency DOWN to %s\n", mf.c_str());
}
/*
  If you want to switch the UP/DOWN buttons on the main page to do PWM or resolution instead of the
  usual frequency UP/DOWN controls, it's really simple. Having these proper handlers in place means
  better feedback on the main page. You could also use the WebCommand mechanism directly to do this
  stuff, but the main page doesn't request LastMessage, so you would lose the instant feedback.
  Ergo: handlers..
*/

/*
/pulseUP             */
void handlePulseUP() {
  pwmStepUP(storeButton);
  server.send(200, _PLAIN_TEXT_, "Pulse Width UP to " + (String)pulse);
  if (eXi) Serial.printf(" HTTP Request: Pulse Width UP to %i\n", pulse);
}

/*
/pulseDOWN             */
void handlePulseDOWN() {
  pwmStepDOWN(storeButton);
  server.send(200, _PLAIN_TEXT_, "Pulse Width DOWN to " + (String)pulse);
  if (eXi) Serial.printf(" HTTP Request: Pulse Width DOWN to %i\n", pulse);
}


/*
/resUP             */
void handleResUP() {
  bitsStepUP(storeButton);
  server.send(200, _PLAIN_TEXT_, "Resolution Bit Depth UP to " + (String)PWMResBits);
  if (eXi) Serial.printf(" HTTP Request: Resolution Bit Depth UP to %i\n", PWMResBits);
}
/*
/resDOWN             */
void handleResDOWN() {
  bitsStepDOWN(storeButton);
  server.send(200, _PLAIN_TEXT_, "Resolution Bit Depth DOWN to " + (String)PWMResBits);
  if (eXi) Serial.printf(" HTTP Request: Resolution Bit Depth DOWN to %i\n", PWMResBits);
}


// Those Three BIG buttons..

/*
/setSquare             */
void handleSetSquareWave() {
  WebCommand = "r";
  server.send(200, _PLAIN_TEXT_, "Setting Square Wave..");
  if (eXi) Serial.println(" HTTP Request: " + WebCommand);
}

/*
/setSine             */
void handleSetSineWave() {
  WebCommand = "s";
  server.send(200, _PLAIN_TEXT_, "Setting Sine Wave..");
  if (eXi) Serial.println(" HTTP Request: " + WebCommand);
}

/*
/setTriangle             */
void handleSetTriangleWave() {
  WebCommand = "t";
  server.send(200, _PLAIN_TEXT_, "Setting Triangle Wave..");
  if (eXi) Serial.println(" HTTP Request: " + WebCommand);
}


/*
/status

The signal info at the top/top-left of the main page..
                    */
void handleStatus() {
  String settings = getCurrentSettings();
  if (eXi) Serial.println(" HTTP Request: Get Current Settings.");
  server.send(200, _PLAIN_TEXT_, settings);
}


/*
/reboot             */
void handleReboot() {
  Serial.println(" HTTP Request: Reboot.");
  WebCommand = "reboot";
  server.send(200, _PLAIN_TEXT_, "Rebooting..");
  if (eXi) Serial.println(" HTTP Request: " + WebCommand);
}


/*
/end             */
void handleEndLoop() {
  endLoop();
  if (eXi) Serial.println(" HTTP Request: End Loop.");
  server.send(200, _PLAIN_TEXT_, "Exiting Loop");
}

// We could use the WebCommand mechanism here, but would lose the ability to immediately return the
// status to the main web page.

/*
/loadPreset             */
void handleLoadPreset() {
  String val, state;
  for (uint8_t i = 0; i < server.args(); i++) {
    if (server.argName(i) == "preset") {
      val = server.arg(i);
      state = loadPreset(val.toInt());
      startSignal("WebLoad");
      break;
    }
  }
  server.send(200, _PLAIN_TEXT_, " [" + state + "]");
  if (eXi) Serial.println(" HTTP Request: " + state);
}

/*
/savePreset             */
void handleSavePreset() {
  String val, state = "FAIL";
  for (uint8_t i = 0; i < server.args(); i++) {
    if (server.argName(i) == "preset") {
      val = server.arg(i);
      state = savePreset(val.toInt());
      break;
    }
  }
  server.send(200, _PLAIN_TEXT_, " [" + state + "]");
  if (eXi) Serial.println(" HTTP Request: " + state);
}



/*
  Not Found?

  Handle "not found" documents and translate those URLs into commands..

  i.e. requesting /s2k will set the frequency step to 2000Hz. How neat is that!

                          */
void handleWebConsole() {
  fromWebConsole = true;
  WebCommand = server.uri().substring(1); // lop off preceding forward slash. Boom! Instant console command.
  server.send(200, _PLAIN_TEXT_, WebCommand + ": OK\nSee /LastMessage for any output from your command.");
  if (eXi) Serial.printf(" HTTP Request: WebCommand: %s\n", WebCommand.c_str());
}

// If you have ESP32 debug enabled, you will see an error here. Or else hack your WebServer.cpp.


/*
  Send the last message.
  Handle the AJAX request that comes in directly after you send /any/ command from the web console.

  This is beautiful and enables us to use the web as a proper console, with responses right there on
  your phone/tablet/PC/whatever. If there is no message available (e.g. they sent a blank command)
  we print out the current signal settings, just like we do in the regular console.
                          */
/*
/LastMessage             */
void handleLastMessage() {
  server.send(200, _PLAIN_TEXT_, (LastMessage == "") ? getCurrentSettings() : LastMessage);
  LastMessage = "";
}


// So let's create a web console..

/*
/console             */
void sendWebConsole() {

  // HTML you can edit..
  #include "Console.h"

  // WebConsole is the String created in Console.h

  // New clients get a list of commands..
  String myClient = server.client().remoteIP().toString();
  if (knownClients.indexOf(myClient) == -1) { // is this client known yet?
    WebConsole.replace("<!--{insert}-->", getCommands()); // Please, never forget the usefulness of the HTML comment.
    knownClients += myClient; // Add this client to the list of "known" clients
  }
  server.send(200, _HTML5_TEXT_, WebConsole);
  if (eXi) Serial.printf(" HTTP Request: WebConsole for client @ %s\n", myClient.c_str());
}

/*
    A simple frequency up/down page..

    This is designed for doing adjustments with your phone/tablet whilst in a less-than-focused-on-
    tech state. It's just two big buttons.

    Perfect for controlling a laser.
    (when any focus you have is focused on keeping it out of your eyes! And other stuff...)

/[various]            */
void sendSimplePage() {
/*
    We could omit the favicon code now as we are serving a favicon image directly from /favicon.ico
    But I think this (base64-encoded) PNG favicon works better for /proper/ pages. It has
    transparent rounded corers, ffs!

    btw, if you didn't know; "base64" is just a simple way to convert binary data into /text/ data,
    i.e. data that can be easily stored and transmitted using traditional 7-bit ASCII text-based
    methods (HTTP, email, etc.).

    Note: UP + DOWN and also LEFT + RIGHT keyboard arrow keys work as controllers in this page.

    We use HEREDOC so we don't need to escape all the quotes and what-not.
*/
  String SimplePage = R"SimplePage(<!DOCTYPE html><html><head><title>ESP32 Signal Generator</title><meta name="viewport" content="width=device-width, initial-scale=1"><link rel="shortcut icon" type="image/png" sizes="16x16" href="data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAQAAAC1+jfqAAAABGdBTUEAALGPC/xhBQAAAAlwSFlzAAAuIwAALiMBeKU/dgAAAAd0SU1FB+cBFRcAJyFRDxkAAAAZdEVYdENvbW1lbnQAQ3JlYXRlZCB3aXRoIEdJTVBXgQ4XAAAA3ElEQVQoz4WRP0tCcRSGH34u0ReIxoKQwBovTQ1iGPgxwqFdcg/nvoHQ3DdoqqFJh8ylMXFsMCSoIa+Pg97rn3vDZznDeTnnvOfFsgP/Y2CZpB07yZUwr13PLfmYowgAQy7Y55QqPTKo3nrsyJFFr5xuTEA/DbZVbUvm4gB9pkQAnAHvGxsCdNnhAIBD9nhLW01uAGJrXqcDG0YLu2Pxbn7DKg/ih6o98TmxueQEeAWgQ6CU2Fzy56UVf/yyaCuxuc6T2LDu7sJwRhB7b8EjX9JH5WT57e9KWFvingFTFG1S956a3gAAAABJRU5ErkJggg=="/></head><style>html{width:100% !important;height:100vh !important;color:#4CAF50;margin:0;}div,pre,p,form{margin:0;}.controller{margin:auto;width:96%;}label{display:none;}.step-button{width:95%;border-radius:0.15em;background-color:#4CAF50;border:none;color:white;text-align:center; padding:0.1em 0.2em;font-size:8em;font-weight:bold;margin:0.175em auto;}.step-button:hover{background-color:#00778f;}.step-button:active{background-color:#fff;color:#8cacb3;}#fup, #fdown {width:100%;height:40vh;}</style><body><div class="controller"><button id="fup" onclick="clickUP()" class="step-button" title="UP">&#8963;</button><button id="fdown" onclick="clickDOWN()" class="step-button" title="DOWN">&#8964;</button></div><script>function clickUP(){var AJAX=new XMLHttpRequest();AJAX.open("GET","setUP",true);AJAX.send();}function clickDOWN(){var AJAX=new XMLHttpRequest();AJAX.open("GET","setDOWN",true);AJAX.send();}document.onkeydown=ArrowKeys;function ArrowKeys(e){if(e.keyCode=='38'){clickUP();}else if(e.keyCode=='40'){clickDOWN();}else if(e.keyCode=='37'){clickDOWN();}else if(e.keyCode=='39'){clickUP();}}</script></body></html>)SimplePage";
/*
  If you edit this, have fun putting back the spaces! (hint: regex)

  Just kidding, there's a copy of the HTML in the same directory as this script.

  So, we have this lovely UP/DOWN page, why not put it to use; for power control. This was, in fact,
  the reason I started all this. I did the frequency first because it was more fun with the kids.

  This is how easy it is to add a whole new UP/DOWN parameter controller page, with all the keyboard
  control, favicon and cool phone-screen-filling goodness you can tap up-down-left-right-without-
  thinking..
                               */
  if (server.uri() == "/pwm") {
    SimplePage.replace("setUP", "pulseUP");
    SimplePage.replace("setDOWN", "pulseDOWN");
  }
  /*
    Boom!

    Let's just do them all while we're here.

    Resolution @ /bits..
                                */
  if (server.uri() == "/bits") {
    SimplePage.replace("setUP", "resUP");
    SimplePage.replace("setDOWN", "resDOWN");
  }
  /*
    There are two methods you can use to get a simple page like this adjusting what /you/ need.

    You can simply use the WebCommand mechanism, which translates raw /URLs into commands..

      SimplePage.replace("setUP", "a");
      SimplePage.replace("setDOWN", "z");

    Which the WebCommand mechanism would pick up as the "a" and "z" commands, meaning, shift bit
    rate UP or DOWN; these commands being handled inside loop() (aka. the void).

    It's just two simple controls that can do *whatever* you want.

    You could use the page to switch between two defined presets, e.g..

      SimplePage.replace("setUP", "l1");
      SimplePage.replace("setDOWN", "l2");

    or ABSOLUTELY ANY COMMAND YOU WANT. Or you could make a page with 50 buttons.

    The second method is to create proper handler functions for the URLs, like I did above. This is
    better for using on the main page (or /customURL) because you can provide instant feedback, via
    a proper server response. Here in the simple page we just need to fire two functions.

    Inside the startServer() function, point your new /customSimpleURL to this very function..

      server.on("/customSimpleURL", sendSimplePage);

    Pop some code inside here (sendSimplePage()) to do the replacements..

        if (server.uri() == "/customSimpleURL") {
          SimplePage.replace("setUP", "customUP");
          SimplePage.replace("setDOWN", "customDOWN");
        }

    Finally, handle the requests and provide responses inside your two custom up/down functions..

      void customUP() {
        DO UP BUTTON STUFF HERE
      }
      void customDOWN() {
        DO DOWN BUTTON STUFF HERE
      }

    That's it.

    See the many examples here for how to do web responses.

  */

  server.send(200, _HTML5_TEXT_, SimplePage); // Spit it out.
  if (eXi) Serial.printf(" HTTP Request: Simple Page for client @ %s\n", \
                            server.client().remoteIP().toString().c_str());
}


/*
  /favicon.ico

  If you accesses the AJAX/command links directly; which is sometimes handy; your browser will
  automatically request a favicon.ico. We could leave this blank, but that is annoying in a browser.

  After wasting an hour or so trying to get any browser to accept a base64 image, and then C++ to
  output one that didn't show up blank, I happened to notice "image/svg+xml" in the Accept headers..

  *sigh*

  So, this is how to send an actual favicon image programmatically in C/C++.. Use SVG. Boom!

  I say "Boom!", because in my search for a solution I found /so/ many stupid/irrelevant/pointless/
  plain old fuxing wrong answers that I almost lost faith in humanity-to-have-any-brain. *sigh2*

  Yes, of course it can be done. What century are you living in?

  NOTE: Add a space or line-break after the initial HEREDOC brace to instantly destroy the image.
*/

/*
/favicon.ico         */
void handleFavicon() {
  String favicon = R"SVGfavicon(<?xml version="1.0" standalone="no"?><!DOCTYPE svg PUBLIC "-//W3C//DTD SVG 20010904//EN" "http://www.w3.org/TR/2001/REC-SVG-20010904/DTD/svg10.dtd"><svg version="1.0" xmlns="http://www.w3.org/2000/svg" width="16.000000pt" height="16.000000pt" viewBox="0 0 16.000000 16.000000" preserveAspectRatio="xMidYMid meet"><g transform="translate(0.000000,16.000000) scale(0.100000,-0.100000)" fill="#777"><path d="M32 128 c-15 -20 -24 -58 -14 -58 6 0 13 11 16 25 8 31 23 32 31 3 21 -75 40 -94 63 -65 15 19 24 57 14 57 -6 0 -13 -12 -16 -26 -9 -34 -28 -26 -39 17 -9 35 -24 59 -37 59 -4 0 -12 -6 -18 -12z"/></g></svg>)SVGfavicon";
  server.send(200, "image/svg+xml", favicon);
}
/*
  If you use a light theme on your browser, you may prefer a black sine wave icon.
  If so, change fill="#777" to fill="#000". You would think it would be stroke="". Nah.
  The default should show up nicely in light or dark themes.

  For a black sine wave on a white background (with sharp corners! ofo!), you can add
  style="background: white" (hack) or perhaps.. style="stroke-width: 0px; background-color: white;"
  right inside the svg tag. see: https://www.w3.org/TR/SVG/styling.html I notice a couple of other
  useful looking tags in there - I may need to learn some SVG. This stuff is handy, if a bit clunky.

  I'd like to say that the icon is different from the main favicon because it's nice to be able to
  differentiate between regular and AJAX tabs but the truth is all the SVG converters I tried simply
  removed the lovely rounded-corner white background. Fair enough.

  But it's also true; I like to be able to tell the difference.
*/



// Proper handlers for /URLs folk might just try..
//
// We ALWAYS populate LastMessage, so Web Console still works (if you did "List", for example).

/*
/list               */
void sendListPage() {
  LastMessage = listPresets();
  server.send(200, _PLAIN_TEXT_, LastMessage);
  if (eXi) Serial.println(" HTTP Request: List Presets");
}
// It annoyed me that Bromite, et al, showed text/plain content with a proportional font, so this is
// a simple way to get a nicely formatted plain text list on one of these annoying browsers.
// Force it!

/*
/List               */
void sendListPageHTML() {
  LastMessage = listPresets();
  server.send(200, _PLAIN_TEXT_, "<!DOCTYPE html><pre>" + LastMessage);
  if (eXi) Serial.println(" HTTP Request: List Presets (HTML)");
}


/*
/help
/c                  */
void sendHelpPage() {
  LastMessage = getCommands();
  server.send(200, _PLAIN_TEXT_, LastMessage);
  if (eXi) Serial.println(" HTTP Request: help");
}

/*
/Help
/C                      */
void sendHelpPageHTML() {
  LastMessage = getCommands();
  server.send(200, _PLAIN_TEXT_, "<!DOCTYPE html><pre>" + LastMessage);
  if (eXi) Serial.println(" HTTP Request: help (HTML)");
}

/*
  These are URLs which would work as-is, but when accessed directly, would return only "OK" to the
  browser. We intercept the command with a handler and provide a better response.
*/


#endif

// End Web Functions




/*
  Set lots of things all-at-once, like this..

    import m=t,f=1k,p=100,s=1,j=10,h=p

  Or directly into a preset, like this..

    import3 m=t,f=1k,p=100,s=1,j=10,h=p

  See the main comments at the top for details.

                                    */
String importSettings(char *imported, uint8_t presetNumber = 0) {

  String thisPref;
  char iBuff[1064];
  sprintf(iBuff, "\n Importing Settings..\n\n");
  char bpl;
  char localMode = mode;

  // Importing settings directly into a preset..
  if (presetNumber != 0) {
    prefsSwitch(presetNumber);
    prefs.putChar("i", 1);
  }

  while ((thisPref = strsep(&imported, ","))) {

    // thisPref is now, for example: "p=100"

    char prefType = thisPref[0];            // 1st character (the 'p')
    char cVal = thisPref[2];                // 3rd character  (that would be '1' - unused in this example)
    String thisVal = thisPref.substring(2); // and this is "100"
    int8_t thisINT = thisVal.toInt();       // 100

    // Save prefs directly to the preset memory's namespace..
    if (presetNumber != 0) {

      switch (prefType) {
        case 'm' :
          prefs.putChar("m", cVal);
          localMode = cVal;
          sprintf(iBuff + strlen(iBuff), "\tSaving Mode: %s\n", makeHumanMode(cVal).c_str());
          break;
        case 'f' :
          prefs.putFloat("f", humanFreqToFloat(thisVal));
          sprintf(iBuff + strlen(iBuff), "\tSaving Frequency: %s\n", makeHumanFrequency(prefs.getFloat("f")).c_str());
          break;
        case 'p' :
          if (localMode != 's' || saveALL) {
            prefs.putUChar("p", thisINT);
            sprintf(iBuff + strlen(iBuff), "\tSaving Pulse Width: %i%%\n", thisINT);
          }
          break;
        case 'a' :
          if (localMode != 'r' || saveALL) {
            prefs.putUChar("a", thisINT);
            sprintf(iBuff + strlen(iBuff), "\tSaving Amplitude Level: %i\n", thisINT);
          }
          break;
        case 'j' :
          if (localMode != 's' || saveALL) {
            prefs.putUChar("j", thisINT);
            sprintf(iBuff + strlen(iBuff), "\tSaving Pulse Width Step: %i%%\n", thisINT);
          }
          break;
        case 'b' :
          if (localMode == 'r' || saveALL) {
            prefs.putUChar("b", thisINT);
            if (thisINT != 1) bpl = 's';
            sprintf(iBuff + strlen(iBuff), "\tSaving Resolution Bit Depth: %i bit%c\n", thisINT, bpl);
          }
          break;
        case 's' :
          prefs.putFloat("s", humanFreqToFloat(thisVal));
          sprintf(iBuff + strlen(iBuff), \
            "\tSaving Frequency Step: %s\n", makeHumanFrequency(prefs.getFloat("s")).c_str());
          break;
        case 'h' :
          prefs.putChar("h", cVal);
          sprintf(iBuff + strlen(iBuff), "\tSaving Touch Mode: %s\n", makeHumanTouchMode(cVal).c_str());
          break;
      }

    } else { // Default settings..

      switch (prefType) {

        // We could use this mechanism to save to a preset, too, but it would destroy your
        // current default settings in the process.

        case 'm' :
          setMode(cVal);
          sprintf(iBuff + strlen(iBuff), "\tSetting Mode to: %s\n", makeHumanMode(mode).c_str());
          break;
        case 'f' :
          frequencySet(thisVal, false, false);
          sprintf(iBuff + strlen(iBuff), "\tSetting Frequency to: %s\n", makeHumanFrequency(frequency).c_str());
          break;
        case 'p' :
          sprintf(iBuff + strlen(iBuff), "\tSetting Pulse Width to: %i%%\n", setPulseWidth(thisINT));
          break;
        case 'a' :
          if (setAmplitude(thisINT)) sprintf(iBuff + strlen(iBuff), "\tSetting Amplitude Level to: %i\n", thisINT);
          break;
        case 'j' :
          if (setPulseStep(thisINT)) sprintf(iBuff + strlen(iBuff), "\tSetting Pulse Width Step to: %i%%\n", thisINT);
          break;
        case 'b' :
          PWMResBits = switchResolution(thisINT);
          if (PWMResBits != 1) bpl = 's';
          sprintf(iBuff + strlen(iBuff), "\tSetting Resolution Bit Depth to: %i bit%c\n", PWMResBits, bpl);
          break;
        case 's' :
          frequencyStepSet(thisVal);
          sprintf(iBuff + strlen(iBuff), "\tSetting Frequency Step to: %s\n", makeHumanFrequency(fStep).c_str());
          break;
        case 'h' :
          setTouchMode(cVal);
          sprintf(iBuff + strlen(iBuff), "\tSetting Touch Mode to: %s\n", makeHumanTouchMode(touchMode).c_str());
          break;
      }
    }
  }
  if (presetNumber != 0) prefsSwitch();
  return (String)iBuff;
}




/*
  Export settings to an importable string..

    export    =   export defaults

    export4   =   export preset 4
    export 4          "
    export=4          "
    etc.              "

  To export *all* settings to a string that can be imported for a
  complete-setup-in-one-command, use:

    export all

  See the exportALLSettings(below) for more details.

                                                             */
String exportSettings(String exData = "0", bool all = false) { // "all" is true during an "export all" command

  if (exData.indexOf("all") != -1 ) return exportALLSettings(); // We'll be back!

  int8_t presetNUM = 0;
  int8_t success = 0;
  char prebuff[256];
  char eBuff[256];
  String loopData, thisPreset, eName;
  String first = exData.substring(0,1);
  loopData.reserve(20480);

  // Enable all three (export3 || export 3 || export=3) formats (for exporting presets).
  // As well as just plain "export" (to export defaults).
  if (exData[0] == '=') {
    presetNUM = exData.substring(1).toInt();
  } else if (first != "0" && first != "") {
    presetNUM = exData.substring(0).toInt(); // toInt() will handle " 4" as well as "4", " 04" and "04".
  }
  if (presetNUM < 0 || presetNUM > presetMAX) success = -1;

  // Number in range
  if (success == 0) {

    // We are working with a preset
    if (presetNUM != 0) {
      thisPreset = (String)presetNUM;
      // Which may or may not exist..
      if (!prefsSwitch(presetNUM)) success = -2;
      // Even if the preset here is empty, we are now in its namespace.
    }

    // The preset exists. Grab its settings..
    if (success == 0) {

      // prebuff only gets used if eBuff receives data.
      String eType;

      if (!all) {
        eType = "Exporting ";
        eType += (presetNUM == 0) ? "Default settings: \n\n " : \
                            "Preset " + thisPreset + ": \"" + prefs.getString("n") + "\"\n\n ";
        // Wipe default / preset settings before import..
        eType += "w" + ((presetNUM == 0) ? "" : thisPreset) + commandDelimiter;
      }
      sprintf(prebuff, "%simport%s ", eType.c_str(), (presetNUM == 0) ? "" : thisPreset.c_str());
      // Finished with prebuff

      // IF a setting exists, add it to the export string..
      if (prefs.getChar("m", '~') != '~') sprintf(eBuff, "m=%c,",  prefs.getChar("m"));
      if (prefs.getFloat("f", -1) != -1) sprintf(eBuff + strlen(eBuff), \
                                              "f=%s,", makeHumanFrequency(prefs.getFloat("f"), true).c_str());
      if (prefs.getUChar("p", '~') != '~') sprintf(eBuff + strlen(eBuff), "p=%i,", prefs.getUChar("p"));
      if (prefs.getUChar("b", '~') != '~') sprintf(eBuff + strlen(eBuff), "b=%i,", prefs.getUChar("b"));
      if (prefs.getFloat("s", -1) != -1) sprintf(eBuff + strlen(eBuff), "s=%s,", \
                                                makeHumanFrequency(prefs.getFloat("s"), true).c_str());
      if (prefs.getUChar("j", '~') != '~') sprintf(eBuff + strlen(eBuff), "j=%i,", prefs.getUChar("j"));
      if (prefs.getUChar("a", '~') != '~') sprintf(eBuff + strlen(eBuff), "a=%i,", prefs.getUChar("a"));
      if (prefs.getChar("h", '~') != '~') sprintf(eBuff + strlen(eBuff), "h=%c,", prefs.getChar("h"));

      if (prefs.getString("n", "") != "" && thisPreset != "") \
                      eName = commandDelimiter + "n" + thisPreset + "=" + prefs.getString("n");

      if (!all && prefs.getString("o", "") != "" && exportALL) {
        loopData += commandDelimiter + " loop" + ((presetNUM == 0) ? "" : thisPreset) + "=" + prefs.getString("o");
        success = 0;
      }
    }
    // If this was a preset, switch back to main prefs..
    if (presetNUM != 0) prefsSwitch();
  }

  String ret;
  uint8_t length = strlen(eBuff);

  if (length > 1) {
    // Remove trailing comma..
    eBuff[length-1] = '\0';
    // Join our Strings together..
    ret = (String)prebuff + (String)eBuff + eName;
  } else if (loopData == "" && success != -2) success = -3; // no data and not a preset


  // Traditionally, we return 0 on success..
  switch (success) {
    case -1:
      if (!all) return _OUT_OF_RANGE_;
      break;
    case -2:
      if (!all) return " No such preset";
      break;
    case -3: // no default settings to export
      if (!all) return " No settings to export!";
      break;
    case 0: // ZEROLuyah!
      return ret + loopData;
  }

  return "";
}


/*

  Export ALL Settings..

  Output a big string of text that you can use to duplicate one Signal Generator to a fresh Signal
  Generator, with all preset memories stored, named and ready-to-go. It will be something like..

    w1;import1 m=r,f=500,p=25,b=10,h=p;n1=Laser Control;w2;import2 m=t,f=1k,p=0,h=f;n2=Saw Left;w3;import3 p=100;n3=Switch Saw Right;w4;import4 m=s,f=440,s=153,a=4,h=f;n4=Music Maker;w5;import5 m=r,f=40m,p=50,b=1,s=1m,h=f;n5=40MHz Sine Wave Baby!;w6;import6 m=s,f=500,p=50,b=10,s=50,j=5,a=2,h=p;n6=Small Sine;w7;import7 m=r,f=1.5k,p=50,b=10,s=50,j=10,h=p;n7=1.5kHz ~ 50% PWM Laser Control;w;import m=r,f=1k,p=25,b=6,s=100,j=10,h=f

  (If you are new to Signal Generator, throw the above command into your command-line to get an
  instant example setup. "list" to have a look-see. "l1", "l2", etc.. to load them up.)

  It's basically just a string of commands separated by the ";" (semicolon - or whatever you set in
  your prefs) character:

      :repeat:
      wipe specified preset
      load settings into preset
      name the preset
      :repeat:

  The /final/ batch of settings is/becomes your *current* settings.

  And yes, it's fine to send that as a URL.

  Any stored loops/macros encountered along the way will be tagged onto the end.

                           */
String exportALLSettings() {

  String data = "", seperator, lD, loopData = "";

  data.reserve(20480);
  loopData.reserve(20480);

  for( uint8_t i = 1; i <= presetMAX; i++ ) {

    // Do this first (as it will switch us back to default prefs when done)
    String pData = exportSettings((String)i, true);

    // Switch to the preset namespace..
    if (prefsSwitch(i)) {

      // There was data in this preset..
      if (pData != "") {

        // Preset wipe (w*) commands..
        data += "w" + (String)i + commandDelimiter;

        // After the wipe, import these settings to defaults or a specified preset..
        data += pData + commandDelimiter;
      }

      // Grab any stored loop commands (for tagging onto the end of the export command)..
      lD = prefs.getString("o", "");
      if (lD != "") loopData += commandDelimiter + " loop" + (String)i + "=" + lD;
    }

    // And switch back..
    prefsSwitch();
  }
  // All presets have been interrogated

  // Finally, wipe the /current/ settings and import our *new* (current!) current settings..
  String curSettings = exportSettings("", true);
  if (curSettings != "") data += "w" + commandDelimiter + curSettings;
  lD = prefs.getString("o", "");
  if (lD != "") loopData = commandDelimiter + " loop=" + lD + loopData;

  // We're done! *phew*
  if (data != "" || loopData != "") return data + loopData;
  return "No settings to export!";
}

// Okay, technically, we don't export *all* the settable settings. The global flags will be whatever
// you have set as defaults in your sketch. However, as you know what they are, it's trivial to add
// ";e" or ";rte", etc. to your setup command, if required.



// (modified from the webcam server example)
// Gonna put something like this in all sketches from now on.
// Happens only once, at boot.
//
void printBasicSketchInfo() {
  // These are slow functions. Don't put them in a loop.
  int sketchSize = ESP.getSketchSize();
  int sketchFreeSpace = ESP.getFreeSketchSpace();
  // String sketchMD5 = ESP.getSketchMD5(); // MD5 is amazing, but you most likely don't need to know this.
                                            // We instead display the (more useful) exact compile date/time.
  // Serial.printf(" Sketch MD5: %s\n", sketchMD5.c_str());

  float_t sketchFreePct = 100 * sketchSize / sketchFreeSpace;
  Serial.printf("\n Sketch Size: %i (total: %i, %.1f%% used)\n", sketchSize, sketchFreeSpace, sketchFreePct);

  // Serial.printf(" Last Modified: %s\n", modified_time); // This is set up near the top (see notes there).
  // Serial.printf(" Compiled: %s\n", compile_time);
  Serial.printf(" Compiled: %s\n", compile_time);

  Serial.printf(" ESP32 SDK: %s\n", ESP.getSdkVersion());
  Serial.println();

}


// Set CPU Frequency..
// ESP32 options are: 240, 160, 80 (MHz)
bool setCPUSpeed(uint32_t newSpeed, bool init = false) {

  bool setFrequency = false;
  if (newSpeed == 240 || newSpeed == 160 || newSpeed == 80) {
    setFrequency = setCpuFrequencyMhz(newSpeed);
  } else {
    Serial.printf("\n Allowed CPU Speed Values: 240, 160 & 80\n");
    return false;
  }
  if (init || !RemControl) {
    Serial.printf("\n Crystal Frequency: %iMHz\n", getXtalFrequencyMhz());
    Serial.printf(" APB Frequency: %s\n", makeHumanFrequency(getApbFrequency()).c_str()); // Advanced Peripheral Bus
    Serial.printf(" CPU Frequency Set to: %iMHz\n", getCpuFrequencyMhz());
  }
  if (setFrequency) cpuSpeed = newSpeed;
  return setFrequency;
}



/*
  Physical push-buttons..

  See the notes around the buttons prefs for more details.

                             */
String setupPhysicalButtons(bool reportOnly = true) {

  char demButts[512] = {'\0'};

  if (useButtons) {

    Serial.println("\n Available Buttons:");

    for (uint8_t i = 0; i < buttonCount; i++) {
      if (buttonPins[i] != 0 && buttonCommands[i] != "") {
        if (!reportOnly) { // It wouldn't be a problem to re-assign, but still.
          buttons.my_butt[i].pin = buttonPins[i];
          buttons.my_butt[i].command = buttonCommands[i];
          buttons.my_butt[i].state = 0;
          buttons.my_butt[i].oldState = 0;
          pinMode(buttonPins[i], INPUT_PULLDOWN);
        }
        sprintf(demButts, " Button %i:  Pin: %i  Command: %s\n", i+1, buttonPins[i], buttonCommands[i].c_str());
      }
    }
  }
  return (String)demButts;
}



/*
  Main Loop (aka. the void)
                            */

void loop() {

  /*
    Start with Time and Tactile stuff..
                                      */
  bool buttonPressed = false;
  uint32_t currentTime = millis();
  bool tmpE = eXi;


  /*
    Buttons..
                  */

  // Only run checks if buttons are enabled.
  if (useButtons) {

    // Once button controls are created, this automatically loops through and checks them all..
    for (uint8_t i = 0; i < buttonCount; i++) {

      // If /this/ button control is enabled..
      if (buttons.my_butt[i].pin != 0) {

        // Read that button pin's current value..
        buttons.my_butt[i].state = digitalRead(buttons.my_butt[i].pin);

        // If the button's state has changed..
        if (buttons.my_butt[i].state != buttons.my_butt[i].oldState) {

          // And is now /pressed/..
          if (buttons.my_butt[i].state == 1) {

            // Perform the command..
            QCommand = buttons.my_butt[i].command;
            buttonPressed = true;
          }
          // This line will run twice; once when you press and again when you release the button.
          // But thanks to its statefulness, the command code (above) runs only /once/.
          buttons.my_butt[i].oldState = buttons.my_butt[i].state;
        }
      }
    }
  }


  // If a button is pressed, we will skip directly to command processing..

  if (!buttonPressed) {

    // Handle touches..
    if (touchRead(touchUPPin) < touchThreshold) {
      if (mode == 'f' || currentTime > (touchTimer + deBounce)) {
        touchTimer = currentTime;
        if (!reportTouches) eXi = false;
        touchUPStep();
        if (!reportTouches) eXi = tmpE;
        return;
      }
    }
    // Using interrupts here would spoil my fun!
    if (touchRead(touchDOWNPin) < touchThreshold) {
      if (mode == 'f' || currentTime > (touchTimer + deBounce)) {
        touchTimer = currentTime;
        if (!reportTouches) eXi = false;
        touchDOWNStep();
        if (!reportTouches) eXi = tmpE;
        return;
      }
    }

    // Buttons and Touches are the only thing which can break into a delay.

    /*
      Delay (~)..

      I wouldn't rely on this delay for anything mission-critical.
      Wemos D1 R32, and my WROOM32-based board are fine with 1ms times here.
      The WROVER modules, not so much.
                    */
    if (amDelaying) {
      if(currentTime < delayStart + delayTime) return;
      amDelaying = false; // Not actually required, if you think about it!
    }


    // It's VR Time!
    if (usePOT) {

      // Potentiometers provide a range of readings from 0 - 4095 or thereabouts, and not in an
      // entirely linear way, either. Still, quite useful. We map this range to cover instead the
      // range we want to control, e.g. 0-100 for PWM, 0-10 for PWM bit depth.

      // Turning you pot all the way left sets this value (to whatever you are controlling).
      uint16_t lowVAL = 0;

      // Turning all the way right (clockwise) sets this value.
      uint16_t highVAL = 10;

      // And this will be the actual value mapped from your potentiometer reading.
      uint16_t pValue;

      // Check for potentiometer changes..
      if (currentTime > (touchTimer + deBounce/stepAccuracy)) {

        touchTimer = currentTime;
        analogValue = analogRead(potPIN);
        // analogValue = constrain(analogValue, 4, 4092); // If you feel the need.

        // We have movement!
        if (analogValue > analogValueOLD + (101-stepAccuracy) || analogValue < analogValueOLD - (101-stepAccuracy) ) {

          // Switch pot mode
          switch (potMode) {
            case 'p' :
              lowVAL = 0;
              highVAL = 100;
              break;
            case 'f' :
              lowVAL = FreqLowerPotLimit;
              highVAL = FreqUpperPotLimit;
              break;
            case 'b' :
              lowVAL = 1;
              highVAL = 10;
          }

          pValue = map(analogValue, 0, 4096, lowVAL, highVAL);
          // if (eXi) Serial.printf("Setting %s to mapped POT Value: %i\n", makeHumanTouchMode(potMode), pValue); //debug

          switch (potMode) {
            case 'p' :
              setPulseWidth(pValue);
              break;
            case 'f' :
              frequencySet((String)pValue);
              break;
            case 'b' :
              switchResolution(pValue);
          }

          startSignal("Potentiometer Change");
          analogValueOLD = analogValue;
        }
      }
    }
  }


  /*
    A Command is available.

    Either from over the serial interface, or else the WebCommand string has been filled from the
    web console or *somewhere* (maybe plain /URL command). Or maybe there are commands in the queue.

    Well, we have a command from /somewhere/.

    First, we populate "raw" with that String, whatever it is..
                                                                       */
  if ((Serial.available() > 0) || (WebCommand != "") || QCommand !="") {

    bool isSerial = false;
    String raw, input, xMSG; // Raw user input, current command, re-usable temp String variable

    // The queue is handled first.
    if (QCommand != "") {

      // One single command is allowed to break into a macro/loop/queue..
      if (Serial.peek() > 0 && Serial.readStringUntil('\n') == "end") {
          endLoop(); // This also empties the queue.
          if (eXi) Serial.println(" Command: 'end'");
          return;
      }

      raw = QCommand;

    } else {

      // Serial input comes next. In the extremely unlike event that there is *also* a web command
      // available, it will be processed on the /next/ loop of the void (after any queue).

      if (Serial.peek() > 0) {

        // So we know where to send the main responses..
        isSerial = true;

        // Read command from the serial interface..
        raw = Serial.readStringUntil('\n');

      } else {

        // Or web..
        raw = urlDecode(WebCommand);
        WebCommand = ""; // Got it now. So delete it.
      }
    }

    // Now we have the raw command String.
    // Lop off any extraneous characters (new-lines, spaces, etc.) from the start/end of the command.
    raw.trim();


    /*
        Load [+Save] a loop..

        Seems like a lot of code.
        If you can think of an easier way to enable all this functionality, let me know.

        This is what happens when you write the (wishful) documentation /before/ the code, as I
        usually do, and while I sure was tempted at one point to just include a regex library, this
        was way more fun. And in the end, quite elegant.

                                   */

    int16_t eqPos = raw.indexOf("="); // Sing-Along Kiddies...
                                      // When a return value from an Arduino String.function we AsSIGN..
                                      // Our Integer Variable we must Positively Definitely SssssSIGN!
                                      // Because String functions may return Minus WuuuuuuONE
                                      // An' we don't want our variable creating modulosodebuggingngnnnn FUN!

    // This code only kicks in if your command begins "loop*=" (loop/macro load) ..
    if (eqPos != -1 && eqPos < 11 && raw.substring(0,4) == "loop") {

      // Lop off any extraneous leading / trailing delimiters and spaces..
      trimDelims(raw);

      // This is the default.
      // (no more loop load commands follow, or "loop" only found in the comment/name)
      loopCommands = raw.substring(eqPos+1);
      QCommand = "";

      // I see "loop" somewhere..
      if (raw.substring(eqPos).indexOf("loop") != -1 ) {

      /*
        It may /look/ like more loop loading commands, but could be loop *play* commands
        tagged onto the end of a loop, or even loop wipe commands. Also, users might
        understandably use the word "loop" in their names. Let's check /all/ that..
                                                                                          */
        int16_t gotLoopPlay = -1, gotLoopLoad = -1; // We store character positions here.
        bool insideComment = false, foundLoop = false;

        /*
          We will parse the String one character at a time, aka. "stream parser".
          Makes sense here as we have a lot to check for. Also, I like simple.
                                                            */
        for (int16_t i = (eqPos+1); i < raw.length(); i++) {

          // Ignore any spaces and move onto the next character (next iteration of the test loop)..
          if (raw[i] == ' ') continue;

          // Comment / Name starts here..
          if (raw[i] == ':') {
            insideComment = true;
            continue;
          }

          // Either no comments here or finished some comments..
          if ((String)raw[i] == commandDelimiter) {
            if (foundLoop) {
              // We already found "loop" and now the command has ended. Gotta be a simple (default) loop play..
              gotLoopPlay = i;
              break; // Break out of the test loop right now.
            }
            // Delimiting has occurred, move on..
            insideComment = false;
            continue;
          }

          // If inside comments/names, immediately move on to next iteration of our (yes, you and me!) test loop..
          if (insideComment) continue;

          // Found the word we are looking for.. (non-existent chars (e.g. -2 when we are at 0) are no problem)
          if (raw[i] == 'p' && raw[i-1] == 'o' && raw[i-2] == 'o'  && raw[i-3] == 'l') { // p o o l  <> l o o p
            foundLoop = true;
            continue;
          }

          // If we haven't yet found our word ("loop"), move on to next iteration..
          if (!foundLoop) continue;

          // OKAY, we are NOT inside comments and he HAVE found the word "loop". w00H00!
          // Let's test what comes /next/..

          // It's a number.. (we will "continue" as long as it's more digits)
          if (isDigit(raw[i])) continue;

          // No more digits. Or no digits. We're done.
          // What did we get?

          // We got a loop LOAD/Wipe command..
          if (raw[i] == '=') {

            // Here we store the position of *where* the load command ended (i).
            gotLoopLoad = i;
            break;

          // We got "loop" then maybe digits, but no '='. Must be a loop PLAY command, then.
          } else {
            gotLoopPlay = i; // store
            break;
          }

        } // End "loop" test loop.

        // I find reading the above code has a calming effect on me and more than once I've come
        // back here just to run Strings through it in my mind.

        // It was a loop play (loop*) command..
        if (gotLoopPlay != -1) {

          loopCommands = raw.substring((eqPos+1), gotLoopPlay); // This becomes part of our current loop load.
          QCommand = raw.substring(loopCommands.length() + (eqPos+1));

        // It was a loop load (loop*=) command..
        } else if (gotLoopLoad != -1) {

          loopCommands = raw.substring((eqPos+1), gotLoopLoad - (eqPos+1)); // Becomes the /next/ loop load.
          QCommand = raw.substring(loopCommands.length() + (eqPos+1)); // Again!
        }
        trimDelims(QCommand);
      }
      trimDelims(loopCommands);


      // Check for presence of preset number..
      String presetNumTest = raw.substring(4, eqPos); // From the end of "loop" up to the "=" sign
      presetNumTest.trim();
      int16_t presetNumber = presetNumTest.toInt();

      // Check preset number is valid..
      if (presetNumber < 0 || presetNumber > presetMAX) {
        LastMessage = "Specified " + _OUT_OF_RANGE_;
        if (isSerial || eXi) Serial.printf(" %s\n", LastMessage.c_str());
        return; // Bye Now!
      }

      // It's valid!

      // Save to preset memory * rather than default memory..
      if (presetNumber != 0) {
        prefsSwitch(presetNumber);
        // In case it doesn't exist - so it shows up in the loops list/export/etc..
        if (loopCommands != "-") prefs.putChar("i", 1);
      }

      // Wipe a loop..
      if (loopCommands == "-") {
        if (prefs.getString("o", "") != "") {
          prefs.putString("o", "");
          LastMessage = "Wiping loop commands";
          if (presetEmpty()) {
            LastMessage += " and empty preset";
            wipePreset(presetNumber);
          }
        } else LastMessage = "No loop commands found!";

      // Save the loop commands to NVS..
      } else if (loopCommands) {

        if (loopCommands.length() > 3999) { // Including null terminator..
          Serial.println(" Loop Commands Too Large To Store! (4000 bytes maximum)");
          Serial.println(" NOTE: You can split your commands into multiple loops and chain them together.");
        } else {
          LastMessage = (prefs.putString("o", loopCommands)) ? \
                                      "Saved loop commands" : "Failed to save loop commands";
        }
      }

      // The zero-or-not-zero-ness of code is what keeps pulling me back.
      if (presetNumber != 0) {
        LastMessage += " (for preset " + presetNumTest + ")";
        prefsSwitch();
      }

      if (isSerial || eXi) Serial.printf(" %s\n", LastMessage.c_str());

      // If this a single import, or the last in a series, print out available NVS "entries"..
      // Not foolproof, but fine for display purposes. In other words, horses for courses..
      if (QCommand.indexOf("loop") == QCommand.indexOf("=") && (isSerial || eXi)) \
                                              Serial.printf("%s\n\n", getFreeEntries().c_str());

      // All done!
      return;


    /*
       Normal operation..
                           */
    } else {

      /*
        Process chained commands..
        A *HUGE* amount of functionality for such a tiny chunk of code.
                                                                          */
      int16_t qTest = raw.indexOf(commandDelimiter);

      /*
        Command contains a semicolon (or your custom delimiter)..
                        */
      if (qTest != -1) {

        // Grab first command in chain; assign to "input"..
        input = raw.substring(0, qTest);

        // Remove this command and create the new queue from what's left..
        QCommand = raw.substring(qTest + 1);

      } else {

        // No delimiter. Ergo, a single command.. (possibly the last command in a queue of commands)

        input = raw;

        // Delete last command in the queue, if it exists.
        // If looping, instead re-populate QCommand with the loop commands..  Tada!

        QCommand = (iLooping) ? loopCommands : "";
      }
    }

    input.trim();

    // Now you can turn a brand new ESP32 device into a wifi-enabled Signal Generator with 50
    // named presets, macros and loops, in seconds.


    // Our actual command, "input" (a String) is now set.
    // if (eXi) Serial.printf(" Input Command: \'%s\'\n", input.c_str()); // debug

    /*
      NOTE: For sections which exit loop() (return;),
      we populate and potentially display LastMessage before we go.
                                                                      */

    /*
      Name a preset.
      Do this before we switch everything to lower case..
                                                           */
    if (input[0] == 'n' || input[0] == 'N' ) {
      LastMessage = namePreset(input.substring(1));
      if (isSerial || eXi) Serial.printf(" %s\n", LastMessage.c_str());
      if (eXi) Serial.printf("%s\n\n", getFreeEntries().c_str());
      return;
    }

    // Normalise to lowercase, in case someone left their CAPSLOCK on by mistake..
    input.toLowerCase();

    if (isSerial && eXi && QCommand == "") Serial.printf("\n Command: \'%s\'\n", input.c_str());


    // Capture first character of the command as a char.. (again!)
    char cmd = input[0];

    // 2nd character of input as a char
    char chr2 = input[1];

    // Grab the command substring starting at the 2nd character
    // (everything after the 'b', or whatever)
    String cmdData = input.substring(1);
    cmdData.trim();

    // We'll need this also as an integer..
    uint16_t iData = cmdData.toInt();

    // These values may change. If so, we will let the user know.
    // Setup some String & flags for these changes..
    String vChange = "", loadedPreset = "", savedPreset = "";
    bool mChange = false, fChange = false, pChange = false;
    bool bChange = false, tChange = false, jChange = false, aChange = false;

    // Initialising Booleans in Arduino IDE sets them to true. (!!!)
    // So we need to specify false for each and every boolean we initialise here. What fun!


    /*
        Start with commands we want processed FIRST..
        (this ordering makes very little difference in practice, but seems sensible)
                                                                                       */

    /*
      Delay..
      We use a non-blocking delay so the ESP32 can do "other stuff" in the background during long delays.

      This can also be used to create lighting displays, or whatever, e.g..

        p10;~10;p15;~9;p20;~8;p30;~7;p40;~6;p50;~5;p60;~4;p70;~3;p80;~2;p90;~;p95;~;p100;~;p95;~;p90;~3;p80;~4;p70;~5;p60;~6;p50;~7;p40;~8;p30;~9;p20;~9;p15;~10

                                                      */
    if (cmd == '~') {
      amDelaying = true;
      if (iData != 0) delayTime = iData;
      delayStart = millis();
      return;
    }

    // NOTE: If you send '~' on its own, you get the same delay as was /previously/ set.



    /*
      Play a musical note..
      Optionally at a specific octave.

      *<note>[|<octave]   e.g..   *c|6

      notes: C, C#, D, Eb, E, F, F#, G, G#, A, Bb, B
      Alternatives (e.g. Gb) are also accepted.

      Octave: 0-8 (middle is 4, the default) or -/+ to shift octave down/up by one octave: *c|+

                                      */
    if (cmd == '*' && cmdData != "") {
      xMSG = cmdData;
      xMSG.trim();
      uint32_t newFrequency = playMusicalNote(cmdData);
      LastMessage = " Set Musical Note (" + xMSG + ") Frequency: " + (String)newFrequency + "Hz\n";
      startSignal("Music");
      if (isSerial && eXi && QCommand == "") Serial.printf(LastMessage.c_str());
      return;
    }


    /*
        Increase Frequency by some value..
                                */
    if (cmd == '+' && cmdData != "") {
      xMSG = cmdData;
      xMSG.trim();
      frequencySet((String)(frequency + humanFreqToFloat(xMSG)));
      fChange = true;
    }

    /*
        Decrease Frequency by some value..
                                */
    if (cmd == '-' && cmdData != "") {
      xMSG = cmdData;
      xMSG.trim();
      frequencySet((String)(frequency - humanFreqToFloat(xMSG)));
      fChange = true;
    }

    // On their own, "-" and "+" control PWM.



    /*
        NOTE: We test against a single char OR a String.

        Commands can "begin with" a char, but must == a "String" (or substring).

        If you want a list of commands, you need to type exactly "c" (no quotes), so if you type
        "crap", you get nada. This is mainly to prevent typos setting off unwanted consequences.

        Also, some of the letters are dual (or more) use.
                                                                */



    /*
        Commands that reboot the device..
                                                     */
    /*
    Wipe entire NVRAM and reboot..  */
    if (input == "wipe") {
        if (!isSerial && wipeIsSerialOnly) {
          LastMessage = "NVRAM Wipe can only be performed from Serial Connexion";
          Serial.printf(" %s\n", LastMessage.c_str());
          return;
        } else {
          Serial.println(" Wiping NVRAM..");
          WipeNVRAM();
          input = "x";
        }
    }

    /*
      Reset settings to defaults (hard-written above, in the prefs) and reboot..

      Letters used so far: a b c e f h i j l m n o p q r s t u w x z

      "i", "n", "o" and "q" are used internally, for preset index, preset name, stored loop/macro
      data, and stored queued commands, respectively.

                      */
    if (input == "reset") {
      prefs.remove("a"); // waveAmplitude
      prefs.remove("b"); // PWMResBits
      prefs.remove("c"); // saveALL
      prefs.remove("e"); // eXi
      prefs.remove("f"); // frequency
      prefs.remove("h"); // touchMode
      prefs.remove("j"); // pStep
      prefs.remove("l"); // layerPresets
      prefs.remove("m"); // mode
      prefs.remove("p"); // pulse
      prefs.remove("r"); // RemControl
      prefs.remove("s"); // fStep
      prefs.remove("t"); // reportTouches
      prefs.remove("u"); // usePOT
      prefs.remove("w"); // onlyAP
      prefs.remove("x"); // exportALL
      prefs.remove("z"); // cpuSpeed
      Serial.println(" Wiping Stored Default Settings.");
      input = "x";
    }


    /*
    Set WiFi Access Point Only mode..   */
    if (input == "wap") {
      onlyAP = true;
      prefs.putBool("w", onlyAP);
      Serial.println(" Setting WiFi Access Point Only Mode..");
      input = "x";
    }

    /*
    Or regular (All Access) Station + AP mode.. */
    if (input == "waa") {
      onlyAP = false;
      prefs.putBool("w", onlyAP);
      Serial.println(" Setting WiFi Station + AP (All Access) Mode..");
      input = "x";
    }

    // Set a new CPU speed (and probably reboot)..
    if (input.substring(0,3) == "cpu") {
      String newSpeed = input.substring(3);
      newSpeed.trim();
      if (setCPUSpeed(newSpeed.toInt())) {
        prefs.putUInt("z", cpuSpeed);
        if (RemControl) input = "x";
      }
    }


    /*
    Toggle/Enable/Disable Remote Control.. */
    if (input.substring(0,6) == "remote") {
      switch (input[6]) {
        case 'e' :
          RemControl = true;
          break;
        case 'd' :
          RemControl = false;
          break;
        default:
          RemControl = (RemControl) ? false : true;
      }
      xMSG = (RemControl) ? "enabled" : "disabled";
      if (isSerial || eXi) Serial.printf("Remote Control is  %s\n", xMSG.c_str());
      prefs.putBool("r", RemControl);
      input = "x";
    }



    /*
    Simply Reboot..                          */
    if ((input == "x") || input == "reboot") {
      // If there commands still in the queue, store them for processing after reboot..
      if (QCommand != "") {
        if (QCommand.length() > 3999) {
          // There is a limit to how much you can store in one go:, 4000 bytes (including terminator).
          Serial.println(" Queued Commands Too Large To Store! (4000 bytes maximum)");
          Serial.println("Do your 'wipe' command on its own, THEN your commands.");
        } else {
          prefsSwitch(); // Initialise prefs namespace so we can use prefs storage
          if (prefs.putString("q", QCommand)) Serial.println(" Queued Commands Stored: OK");
        }
      }
      Serial.println(" Rebooting..");
      Serial.flush();
      prefs.end();
      ESP.restart();
    }


    /*
        Finished with commands that reboot.
                                            */


    /*
     List all stored loops/macros.
                        */
    if (input == "ll") {
      LastMessage = listLoops(false);
      if (isSerial || eXi) Serial.printf("\n %s\n", LastMessage.c_str());
      return;
    }

    /*
     Long List of Loops (single importable String)
                         */
    if (input == "lll") {
      LastMessage = listLoops(true);
      if (isSerial || eXi) Serial.printf("\n %s\n", LastMessage.c_str());
      return;
    }

    /*
      Print out button info..
                              */
    if (input == "buttons") {
      LastMessage = setupPhysicalButtons();
      if (isSerial || eXi) Serial.printf("%s", LastMessage.c_str());
      return;
    }

    /*
      Copy a preset to a new preset..
                                      */
    eqPos = input.indexOf(">"); // We'll just re-use this handy variable.
    if (eqPos != -1 && input.substring(0,4) == "copy") {
        String copyFrom = input.substring(4, eqPos);
        copyFrom.trim();
        String copyTo = input.substring(eqPos+1);
        copyTo.trim();
        int8_t cT = copyTo.toInt();
        LastMessage = copyPreset(copyFrom.toInt(), cT);
        if (LastMessage.indexOf("OK") != -1) LastMessage += "\n" + getNVRAMPresetData(cT);
        if (isSerial || eXi) Serial.printf(" %s\n", LastMessage.c_str());
        return;
    }


    /*
      Start the most-recently-loaded, or user-specified loop/macro..
                                         */
    if (input.substring(0,4) == "loop") {
      String pNum = input.substring(4); // Everything after "loop".
      pNum.trim(); // I've gotten superstitious about this now
      int8_t ipNum = pNum.toInt();
      String displayNum = "";

      if (ipNum != 0) {
       if (ipNum < 0 || ipNum > presetMAX) {
          LastMessage = "Specified Loop/Macro " + _OUT_OF_RANGE_;
          if (isSerial || eXi) Serial.printf(" %s\n", LastMessage.c_str());
          return;
       }
        prefsSwitch(ipNum); // Boom! Now you can save 50 loops.
        displayNum = " (" + pNum + ")";
      }

      // Play a preset's loop and if it doesn't exist, default back to (keep) the most recently-loaded loop..
      loopCommands = prefs.getString("o", loopCommands);
      if (loopCommands != "") {
        iLooping = true; // delimiters would act like an "end" command..
        while (loopCommands.endsWith(commandDelimiter)) loopCommands.remove(loopCommands.length()-1);
        QCommand = loopCommands;
        LastMessage = "Processing Loop Commands" + displayNum + ": " + loopCommands;
        if (isSerial || eXi) Serial.printf(" %s\n", LastMessage.c_str());
        eXiTmp = eXi;
        eXi = false;
      } else {
        LastMessage = "No Loop Commands Found: ";
        if (isSerial || eXi) Serial.printf(" %s\n", LastMessage.c_str());
      }
      if (ipNum != 0) prefsSwitch();
      return;
    }


    // Stop playing a macro (it reached the end)..
    if (input == "end") {
      endLoop();
      return;
    }

    // Stop currently running signal..
    if (input == "stop" || input == "." || input == "silence") {
      stopSignal();
      return;
    }

    /*
     Print version information..
                            */
    if (input == "version") {
      LastMessage = "Signal Generator v" + version;
      if (isSerial || eXi) Serial.printf(" %s\n", LastMessage.c_str());
      return;
    }

    /*
      Create Defaults (from current settings)

      A nice way to quixly (sic) get some preset into your current default settings.

      If layering is disabled, "m" would have the same effect. But when layering is enabled
      (the default), you need to use "d" to get ALL the current settings into defaults.
                      */
    if (input == "d") {
      LastMessage = savePreset(0, true);
      if (isSerial || eXi) Serial.printf(" %s\n", LastMessage.c_str());
      return;
    }
    // Remember, simply /beginning/ with 'd' is not enough.


    /*
       WTF!
       Frustration has occurred (except probably misspelt).
                        */
    if (input == "wtf") {
        LastMessage = "\n RTFM! ;o)";
        // Apparently this prints directly to UART, bypassing interrupts. Curious!
        // ets_printf("\n %s\n", LastMessage.c_str());
        if (isSerial || eXi) Serial.printf("\n %s\n", LastMessage.c_str());
        return;
    }
    // I use this as a test command.


    /*
      Print out a list of commands, to either console.
                                                          */
    if (input == "c" || input == "help" || input == "?") {
      // For obvious reasons, ? only works in a serial console, though you could certainly do /%3F !
      LastMessage = getCommands();
      if (isSerial || eXi) Serial.printf(" %s", LastMessage.c_str());
      return;
    }


    /*
      Step UP/DOWN from consoles/URL..

      < and > might seem more intuitive, but would require extra effort (i.e. the SHIFT key).
      The [ and ] key, as well as doing a half-decent job of portraying UP and DOWN (especially if
      you are running a square wave!), also happen to be right next to the <enter> key.
                      */
    if (input == "]") {
      frequencyStepUP(fromWebConsole ? 3 : 2); // 2 == regular serial console, 3 == web console
      fromWebConsole = false;
      fChange = true;
      // return;
    }
    if (input == "[") {
      frequencyStepDOWN(fromWebConsole ? 3 : 2);
      fromWebConsole = false;
      fChange = true;
      // return;
    }

    /*
      PWM step up/down by 5% (or whatever) at a time..
      All this work was to do this one thing. No seriously.
                                                      */
    if (input == "/" || input == "=" || input == "+") {
      pwmStepUP(storeButton);
      pChange = true;
    }
    if (input == "\\" || input == "-") {
      pwmStepDOWN(storeButton);
      pChange = true;
    }

    // Resolution UP/DOWN, 1 bit at a time..
    if (input == "a") {
      PWMResBits = switchResolution(PWMResBits+1);
      bChange = PWMResBits+1;
    }
    if (input == "z" ) {
      PWMResBits = switchResolution(PWMResBits-1);
      bChange = PWMResBits-1;
    }


    /*
      Extended Information in the serial/web console..
                      */
    if (input == "e") {
      // Flip the extended info flag..
      eXi = eXi ? false : true;
      // All Hail The Conditional Operator! It always works. But sometimes you need to add braces..
      LastMessage = "Extended Info: " + (String)(eXi ? "Enabled" : "Disabled");
      if (isSerial || eXi) Serial.printf(" %s\n", LastMessage.c_str());
      prefs.putBool("e", eXi);
      return;
    }


    /*
    List all presets and their settings.  */
    if (input == "list") {
      LastMessage = listPresets();
      if (isSerial || eXi) Serial.printf(" %s\n\n", LastMessage.c_str());
      return;
    }


    /*
        Import / Export..
                            */

    /*
      Import some settings into the default settings.
      Or directly into a specified preset..
                                          */
    if (input.substring(0,6) == "import") {
      xMSG = input.substring(6);
      xMSG.trim();
      // Grab preset digits, if they exist.
      String pNum;
      // Yet another way to do this..
      while (isDigit(xMSG.charAt(0))) {
        pNum += xMSG.charAt(0);
        xMSG = xMSG.substring(1);
      }
      xMSG.trim();
      char importDATA[72];
      // Convert String to char array for importSettings()..
      xMSG.toCharArray(importDATA, xMSG.length()+1);
      // Add a comma to the end of the char array..
      sprintf(importDATA + strlen(importDATA), ","); // .. so strsep() captures the last datum
      LastMessage = importSettings(importDATA, pNum.toInt()) + getFreeEntries() + "\n\n";
      if (isSerial || eXi) Serial.print(LastMessage);
      startSignal("import");
      return;
    }



    /*
      Export default settings or a preset's settings
                                          */
    if (input.substring(0,6) == "export") {
      LastMessage = exportSettings(input.substring(6));
      if (isSerial || eXi) Serial.printf("\n %s\n\n", LastMessage.c_str());
      return;
    }


    /*
      Toggle Export All ("ea" to toggle, "eae" to enable, "ead" to disable)

      Only applies to exporting of /individual/ presets.

                                      */
    if (input.substring(0,2) == "ea") {
      switch (input[2]) {
        case 'e' :
          exportALL = true;
          break;
        case 'd' :
          exportALL = false;
          break;
        default:
          exportALL = (exportALL) ? false : true;
      }
      xMSG = (exportALL) ? "Enabled" : "Disabled";
      LastMessage = "Export ALL is " + xMSG + ".";
      if (isSerial || eXi) Serial.printf(" %s\n", LastMessage.c_str());
      prefs.putBool("x", exportALL);
      return;
    }

    /*
      Toggle the reporting of touches to the console.
      "rt" to toggle, "rte" to enable, "rtd" to disable.

                                      */
    if (input.substring(0,2) == "rt") {
      switch (input[2]) {
        case 'e' :
          reportTouches = true;
          break;
        case 'd' :
          reportTouches = false;
          break;
        default:
          reportTouches = (reportTouches) ? false : true;
      }
      xMSG = (reportTouches) ? "enabled" : "disabled";
      LastMessage = "Reporting touches to the console is " + xMSG + ".";
      if (isSerial || eXi) Serial.printf(" %s\n", LastMessage.c_str());
      prefs.putBool("t", reportTouches);
      return;
    }


    /*
      Toggle Preset Layering ("lp" to toggle, "lpe" to enable, "lpd" to disable)

                                      */
    if (input.substring(0,2) == "lp") {
      switch (input[2]) {
        case 'e' :
          layerPresets = true;
          break;
        case 'd' :
          layerPresets = false;
          break;
        default:
          layerPresets = (layerPresets) ? false : true;
      }
      xMSG = (layerPresets) ? "Enabled" : "Disabled";
      LastMessage = "Layering Presets is " + xMSG + ".";
      if (isSerial || eXi) Serial.printf(" %s\n", LastMessage.c_str());
      prefs.putBool("l", layerPresets);
      return;
    }


    /*
      Toggle Save All ("sa" to toggle, "sae" to enable, "sad" to disable)

                                      */
    if (input.substring(0,2) == "sa") {
      switch (input[2]) {
        case 'e' :
          saveALL = true;
          break;
        case 'd' :
          saveALL = false;
          break;
        default:
          saveALL = (saveALL) ? false : true;
      }
      xMSG = (saveALL) ? "Enabled" : "Disabled";
      LastMessage = "Save ALL is " + xMSG + ".";
      if (isSerial || eXi) Serial.printf(" %s\n", LastMessage.c_str());
      prefs.putBool("c", saveALL);
      return;
    }


    /*
     Use Potentiometer?   up/upe/upd
                                      */
    if (input.substring(0,2) == "up") {
      switch (input[2]) {
        case 'e' :
          usePOT = true;
          break;
        case 'd' :
          usePOT = false;
          break;
        default:
          usePOT = (usePOT) ? false : true;
      }
      xMSG = (usePOT) ? "Enabled" : "Disabled";
      LastMessage = "Potentiometer Control is " + xMSG + ".";
      if (isSerial || eXi) Serial.printf(" %s\n", LastMessage.c_str());
      prefs.putBool("u", usePOT);
      return;
    }



    /*
    Memory information..               */
    if (input.substring(0,3) == "mem") { // "memory information please" will also work.
      char mbuf[380];
      sprintf(mbuf, "\n Free memory: %d bytes\n", esp_get_free_heap_size());
      sprintf(mbuf + strlen(mbuf), " This Task High Watermark: %d bytes\n", uxTaskGetStackHighWaterMark(NULL));
      sprintf(mbuf + strlen(mbuf), " Available Internal Heap Size: %d\n", esp_get_free_internal_heap_size());
      sprintf(mbuf + strlen(mbuf), " Minimum Free Heap Ever Available Size: %d\n", esp_get_minimum_free_heap_size());
      sprintf(mbuf + strlen(mbuf), " Total Heap: %d\n", ESP.getHeapSize());
      sprintf(mbuf + strlen(mbuf), " Free Heap: %d\n", ESP.getFreeHeap());
      sprintf(mbuf + strlen(mbuf), getFreeEntries().c_str());
      LastMessage = (String)mbuf;
      if (isSerial || eXi) Serial.println(mbuf);
      return;
    }


    /*
      Dummy command we can send from the web to get straight to the update section.
      If you send an empty command in the web console, this is what is *actually* sent.
      This should pass through without URL encoding in your browser.

      NOTE: This will NOT re-start the generator.
                      */
    if (input == "!") {

      // NOTHING IS HAPPENING HERE! MOVE ALONG! (in loud Vogon)

    /*
        Mode change:    Rectangle  /  Sine  /  Triangle / CYCLE
                                                                             */
    } else if (input == "r" || input == "s" || input == "t" || input == "'") {

      char newMode = cmd;
      // If input == "s", cmd also == 's' (cmd == input[0], a char)

      if (newMode != mode || input == "'") {

        // If the mode has changed, restart the generator..
        mChange = true;

        // ' to cycle modes..
        if (input == "'") {
          switch (mode) {
            case 'r' :
              mode = 's';
              break;
            case 's' :
              mode = 't';
              break;
            default :
              mode = 'r';
          }
        } else {
          mode = newMode;
        }
        startSignal("Console Mode Change");
      }
      // It may not have changed, but it has now been "set". So we save it..
      prefs.putChar("m", mode);


    } else {

    /*
        Then, commands that /begin/ with something.
        We will (probably) re-start the generator at the end of this section.

                                                      */
      bool doWipe = true;

      // At the end of all this, we will likely need to restart the generator.
      // Some functions do it themselves, though, so we set a flag for this.
      // It would be no problem to just restart the generator with every input, but this is better.
      bool reGen = true;

      // Variables must be initialised /before/ we enter the switch() statement.

      switch (cmd) { // The 1st character

        // Used for comments / loop names, etc..
        case ':' :
          reGen = false;
          break;

        /*
          Wipe a preset's settings, and only those. Other presets and NVS storage areas remain intact.
          "w" on its own wipes the main settings, which is useful for creating layered presets and
          other stuff.

          NOTE: Wiping a preset does NOT wipe any loop/macro data contained within the preset's
                namespace. You need to wipe loops separately (loop*=-).
        */
        case 'w' :

          xMSG = "\n Clearing ";

          if (input == "w") {

            xMSG += "default preset.";

          } else {

            xMSG += "preset " + cmdData;

            // Because it's a data wipe, we do more rigorous checks..
            switch (cmdData.length()) {
              case 3:
                if (!isDigit(cmdData.charAt(2))) doWipe = false;
                // fall through
              case 2:
                if (!isDigit(cmdData.charAt(1))) doWipe = false;
                // fall through
              case 1:
                if (!isDigit(cmdData.charAt(0))) doWipe = false;
                // The "fall through" statements tell the compiler that we /meant/ to do this (i.e. no break;s).
            }
          }

          if (doWipe) {
            if (wipePreset(iData)) {
              LastMessage = xMSG + "\n" + getFreeEntries() + "\n";
            } else{
              LastMessage = " No Such Preset";
            }
          } else {
            LastMessage = " Invalid Wipe Command!";
          }

          if (isSerial || eXi) Serial.println("\n" + LastMessage);
          return;


        /*
           Frequency
                          */
        case '0' ... '9' :
          // 1st character was a number. This must be a frequency change.
          fChange = true;
          frequencySet(input);
          break;


        /*
           Pulse Width / Duty cycle commands are 'p' followed by a numerical value, e.g. p30
                  */
        case 'p' :
          setPulseWidth(iData);
          pChange = true;
          break;

          // NOTE: sending 'p' on its own (or p<some rubbish>) gets you p0 (pulse width 0%)



          /*
            Switch PWM resolution bits..   e.g. b10

            NOTE: If your square wave now fails, this action was still a success,
                  So there will no "fail" state returned here.
                */
        case 'b' :
          PWMResBits = switchResolution(iData);
          bChange = iData;
          break;

          // NOTE: sending 'b' on its own gets you b1 (Resolution Depth = 1 bit)



        /*
          Frequency step value (used for touch / web controller up/down buttons)
                                              */
        case 'f' : // Finger! A legacy thing.
        case 's' : // 'r', 'e', 't' and others could also be re-used.
          frequencyStepSet(cmdData);
          tChange = true;
          reGen = false;
          break;

          // Send 'f' on its own to get 1Hz frequency step



        /*
          Pulse Width step size - for changes from touch/simple up/down buttons.. (1-100)
                    */
        case 'j' :
          if (setPulseStep(iData)) jChange = true;
          break;



        /*
          Potentiometer Handler (letter) / Step Accuracy (number).
          (e.g. vf or v50; in other words, 'v' followed by either a letter or a number)

          Higher accuracy means more errors (ghost changes) but faster, more fine-tuned operation.
          (1-100) 1 is best if you are using the pot for PWM/Resolution changes.

          I like this multi-use command thingie, and may re-use the idea.

                  */
        case 'v' :
          if (usePOT) {
            switch (chr2) {
              case 'p' :
                potMode = 'p';
                break;
              case 'f' :
                potMode = 'f';
                break;
              case 'b' :
                potMode = 'b';
                break;
              default:
                if (iData > 0 && iData <= 100) {
                  stepAccuracy = iData;
                  aChange = stepAccuracy;
                }
            }
          } else {
            LastMessage = "Potentiometer Not Enabled!";
            if (isSerial || eXi) Serial.printf("\n %s\n", LastMessage.c_str());
            return;
          }
          break;


        /*
          Amplitude  (Wave Scale: 1/8th, 1/4, 1/2, and full wave: a1 - a4)
                  */
        case 'a' :
          if (mode != 'r') {
           if (iData == waveAmplitude) {
             vChange = "No Change";
             reGen = false;
           } else if (setAmplitude(iData)) {
              vChange = (String)waveAmplitude;
            } else {
              vChange = "Ignored: Out-Of-Range! (1-4)";
              reGen = false;
            }
          } else {
            vChange = "Square Wave always 100%";
            reGen = false;
          }
          break;


        /*
          Presets.

          Think: MEMORY --> LOAD!

          The number of presets you can store is limited only by your available NVRAM/NVS
          We manually set a limit of 50, which is about right. You can change this in the prefs.
          (there is a built-in limit of 253 namespaces, so don't go any higher than that)

          When you save a preset (or view a saved preset's setting), the number of available
          "entries" is printed out to the console (when extended info is enabled).

          Search this sketch (for "hunners") for how to increase the amount of available NVRAM,
          to around 690 free "entries".
        */

        /*
            Save Memory Preset..
                                 */
        case 'm' :
          LastMessage = savedPreset = savePreset(iData);
          if (isSerial || eXi) Serial.printf(" %s\n", savedPreset.c_str());
          return;


        /*
            Load Memory Preset..
                                 */
        case 'l' :
          LastMessage = loadedPreset = loadPreset(iData);
          if (isSerial || eXi) Serial.printf(" %s\n", loadedPreset.c_str());
          break;



        // Print out current prefs from NVRAM, then exit loop();
        case ',' :
        // or else print out a preset's setting and exit loop();
        case 'k' :
          LastMessage = getNVRAMPresetData(iData);
          if (LastMessage == "") LastMessage = "No such preset";
          if (isSerial || eXi) Serial.printf(" %s\n", LastMessage.c_str());
          return;
          // I added this then later had that "Doh!" moment. But I keep both in because
          // I find it easier to remember that , is current defaults, and k is a presets.


        // Switch the touch handler to frequency / pulse width / resolution bit depth
        case 'h' :
          setTouchMode(chr2);
          LastMessage = "Touch handler set to " + makeHumanTouchMode(touchMode);
          if (isSerial || eXi) Serial.printf(" %s\n", LastMessage.c_str());
          prefs.putChar("h", touchMode);
          return;


        /*
          Directly set the Voltage on the DAC pin..
          Experimental feature enabling amplitude of sorts in rectangle/square wave mode.

          These changes persist beyond regular reboot ("x") but not total power-down.

          This function will not always do what you expect.
          Best to test before you deploy.

                                          */
        case '@' :
          xMSG = cmdData;
          xMSG.trim();
          LastMessage = directDACVolts(cmdData.toFloat());
          if ((isSerial || eXi) && QCommand == "") Serial.printf(" %s\n", LastMessage.c_str());
          return;

      }


      // Limits be damned! Override frequency..
      if (cmd == 'o') {
        fChange = true;
        frequencySet(cmdData, true);
      }

      // Re-start generator with new (or old) settings..
      if (reGen) startSignal("Main Loop");
    }

    // All done with user commands.


    /*
       The following code runs on *any* /input/..
                                                 */

    /*
       Populate LastMessage, to return with any subsequent status web request
       and output current values to console
                                                                                    */

    /*
        Report any changes (not already reported)
                                                                                                             */
    if (!iLooping && (fChange || mChange || pChange || bChange || tChange || jChange || vChange || aChange)) {

      // Hack to prevent the following output beginning off the edge of the console
      // if you are debugging the sine wave generator, uncomment this.
      //if (isSerial || eXi) Serial.println();

      char buffer[512]; // This is excessive as only one of these will be printed..

      String rState;
      if (mode == 'r') rState = recState();

      if (mChange) sprintf(buffer, " Generator set to: %s Wave\n", makeHumanMode(mode).c_str());
      if (fChange) {
        if (didLimit) {
          sprintf(buffer + strlen(buffer), "\n*** Frequency out-of-bounds! Auto-reset to: ");
        } else {
          sprintf(buffer + strlen(buffer), " Frequency set to: ");
        }

        sprintf(buffer + strlen(buffer), "%s%s", makeHumanFrequency(frequency).c_str(), rState.c_str());
        if (didLimit) sprintf(buffer + strlen(buffer), " ***");
        sprintf(buffer + strlen(buffer), "\n");
      }
      if (pChange) sprintf(buffer + strlen(buffer), " Pulse Width set to: %i%s\n", pulse, rState.c_str());
      if (tChange) sprintf(buffer + strlen(buffer), " Frequency Step set to: %s\n", makeHumanFrequency(fStep).c_str());
      if (jChange) sprintf(buffer + strlen(buffer), " PWM Step set to: %i\n", pStep);
      if (vChange != "") sprintf(buffer + strlen(buffer), " Amplitude set to: %s\n", vChange.c_str());
      if (aChange) sprintf(buffer + strlen(buffer), " Potentiometer Accuracy set to: %i\n", stepAccuracy);
      if (bChange) {
        char bpl;
        if (PWMResBits != 1) bpl = 's';
        sprintf(buffer + strlen(buffer), " PWM Resolution set to: %i bit%c%s", PWMResBits, bpl, rState.c_str());
      }
      if (loadedPreset != "") sprintf(buffer + strlen(buffer), " Loaded Preset %s\n", loadedPreset.c_str());
      if (savedPreset != "") sprintf(buffer + strlen(buffer), " Saved Preset %s\n", savedPreset.c_str());

      LastMessage = (String)buffer;
      if (isSerial || eXi) Serial.println(buffer);
    }

    // This will never display during a loop (isSerial == false)
    if (isSerial) Serial.println("\n Current Settings:\n");
    if (isSerial) Serial.println(getCurrentSettings());
  }

#if defined REMOTE
  // Handle any web server stuff..
  if (RemControl) server.handleClient();
#endif

  // Scheduled to reboot?
  if (!iLooping && dailyReboot && millis() > 86400000) { // 24h in milliseconds == (24 * 60 * 60 * 1000) !
    if (eXi) Serial.println("\n Scheduled Reboot..");
  }
}



/*
  Initial Setup
                */
void setup() {

  Serial.setDebugOutput(true); // pretty sure this is the default, anyway.

  Serial.begin(115200);
  // If YOU .begin Serial with any other speed I will find you and and I will hurt you. [legal note:
  // this is comedy, common in corz.org comments and in no way represents the intentions of the author.
  // It is meant merely to inform and amuse. Also, you'd be wasting you time; I spent every last penny
  // on ESP32 modules.]

  // Seriously though, anyone using a different speed is either a) ignorant of the ESP32 default
  // speed or b) an ex-Arduino user who "just forgot". Or maybe c) like things SLOW and annoying.

  Serial.println("\n ***   Welcome to ESP32 Signal Generator v" + version + "  ***");

  // Get settings from NVS..
  loadDefaultPrefs();

  // Buttons!
  Serial.print(setupPhysicalButtons(false).c_str());

  // Set CPU speed (before network!)
  setCPUSpeed(cpuSpeed, true);

  if (eXi) printBasicSketchInfo();

  // If this is the first run, setup basic prefs and allocate preset space..
  if (prefs.getChar("i", -1) == -1) {
    prefs.putChar("i", 1);
    Serial.println(" First Run: Initialising " + (String)presetMAX + " Preset NameSpaces..");
    // We do the /actual/ allocating/listing below the current settings output (about 23 lines down)
  }
  // If WiFi is enabled, the ESP32 system will eat up another 124-ish entries by itself.
  // Which is why we wait until after WiFi setup to print out the initial free NVS entries.

  // Fix out-of-bounds frequency settings..
  if (checkLimitsOnBoot) checkLimits(frequency);

  // Get the signal up
  startSignal("INIT", true);

  // Reserve memory for Big Global Strings (BGS, pronounced, "BeeJeez") that will jump around a lot.
  QCommand.reserve(4096); // If it exceeds this, it's not a problem; just reverts to the regular heap behaviour.
  LastMessage.reserve(4096); // That commands list is getting big. Also preset lists ...

  // We get the signal up first, /then/ deal with remote control..
#if defined REMOTE
  if (RemControl) startServer();
#endif

  Serial.printf("\n Current Settings:\n\n %s\n", getCurrentSettings().c_str());

  // We just do it anyway (even when it's not 1st run), to return the free entries..
  Serial.println(listPresets(true));

  // Pick up any commands that were queued before a reboot..
  String readQ = prefs.getString("q", "");
  if (readQ != "") {
    Serial.printf("\n Discovered Queued Commands: %s\n Processing..\n", readQ.c_str());
    QCommand = readQ;
    prefs.remove("q");
  }

}

// Apparently Arduino IDE used to be pretty good at generating function prototypes. *sigh*
// No honestly guv, I like having the setup right at the frackin' foot!


/*

  itstory:

  v1: Fixed existing code, got it working.

  v2: Added a sh*t-load of new features and megafun for your serial console

  v3: Added a web interface. omfg! how cool am i?!

  v4: Presets. f*ck yes. you love me long time now.

  v5: Web Console. holy moly! browse to /l5 and you load preset 5. And ALL other console commands..
      In the web! Includes a new console page at /console where you can send commands and get
      instant feedback, just like you would in the serial/UART console, except over WiFi. Oooooh!
      Suck on that, baby!

  v5.5: Major upgrades for presets - now with layering. Import/export settings and much more!

  v6: You can now export an entire setup and import it again on a different Signal Generator. Boom!

      To facilitate this, we can now also chain commands with the ";" separator, just like in C/C++/
      PHP/etc..This is some slick sh*t!

  v7: Now with loops / macros, musical notes and more. Booyah!

  Seriously though; who is down here reading this stuff?!

  btw, if that's you. I love you!


 */
