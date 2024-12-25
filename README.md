# ESP32-SignalGenerator
[![A hastily concocted amalgum of the three big buttons from the main Signal Generator interface, except instead of the square, triangle and sine glyphs inhabiting individual buttons, they share one single button; in that order; to resemble a sort of half-robot, half-tribal-warrior face, or perhaps a big cuboid bird coming at you. The 'button' has now grown stubby legs all round, to resemble a microchip, which gives ferocity to the incoming-bird image.](https://corz.org/img/ESP32/SignalGenerator_Icon_Logo_Flat-Green.webp "The ESP32 Signal Generator for the masses
Click here to go directly to the downloads directory")](https://corz.org/public/scripts/ESP32/SignalGenerator/)

## An ESP32 Signal Generator for Square, Sine and Triangle waves.

### With serial, web, touch, potentiometer and button control; memory presets, import, export, loops, macros, music and much more.

### Introduction:

I think most everyone needs a Signal Generator in their kit-bag. From testing circuits and devices, as well as your kid's hearing, to making ad-hoc synths, powering LEDs, lasers, motors, fans, and pretty much any low-voltage device you can think of with absolute precision (think brightness, speed, pitch, and so on), a decent Signal Generator is just _way_ too handy not to own.

Especially one that costs about the same as a coffee and you can control with your phone.

The ESP32 is the ideal candidate for this sort of malarkey. You can program one up and stick it in a box with fancy buttons and knobs for under a fiver. Adding a screen would only set you back another quid on AliExpress. Madness!

Gone are the days when you would need to etch PCBs and design actual circuits for this sort of thing. It's all happening in software now. All we need is the code..

So here is a much updated and improved version of "that German ESP32 Signal Generator that doesn't work" that works with the latest Espressif IDF.

<`insert>` Superhero Pose. Applause and strewn roses </`insert>`

I like to think of all those wonderful sketches out there as a sort of rights-of-passage for ESP32 coders; [CipherSaber](https://www.google.com/search?q=CipherSaber "I quote, 'CipherSaber is a simple symmetric encryption protocol based on the RC4 stream cipher. Its goals are both technical and political: it gives reasonably strong protection of message confidentiality, yet it's designed to be simple enough that even novice programmers can memorize the algorithm and implement it from scratch. According to the designer, a CipherSaber version in the QBASIC programming language takes just sixteen lines of code. Its political aspect is that because it's so simple, it can be reimplemented anywhere at any time, and so it provides a way for users to communicate privately even if government or other controls make distribution of normal cryptographic software completely impossible.' Many years ago I wrote a 'pure' AutoIt version, and so earned my very own CipherSaber Certificate!")\-style. This is my own, personal approach to non-working code and keeps me generally sane. Trouble is, with ESP32 et al, the underlying technologies are prone to _rapid_ development and ESP32 code, unless it's maintained, quickly goes bad.

Most ESP32 n00bs, faced with this issue, Google their fingers off and then do a lot of copying and pasting, and hoping. And posting. You can read a lot of these posts online.

This isn't a bad approach, per se, but it's usually better to consult the most recent applicable header (\*.h) files\*\* and their associated C files to find out what's _actually_ going on. The [Arduino reference](https://www.arduino.cc/reference/en/) is excellent and the [official ESP32 documentation](https://docs.espressif.com/projects/arduino-esp32/en/latest/) is also pretty good. But again, subject to major upheaval.

So, being an ESP32 n00b myself, but with a vague smattering of C/C++ from earlier decades, I did a bit of both and here present to you my ESP32 GeneratorSaber; head-and-shoulders above the rest by standing on the head and shoulders of the rest[\-+-](https://corz.org/ESP32/square-sine-triangle-wave-signal-generator/#signal-generator-the-source). Which is one advantage of being a bit late to the party. At any rate, bang goes my holiday!

So, an English version. And a **working** version. The code was broken in the latest Espressif IDF Plugin (2.0.6 - IDF 4.3), so I clumsily bodged it back together and fixed and tweaked until it worked and then worked well enough for what I needed. And learned a sh\*t-load about ESP32 in the process. This sketch working with versions through to 2.0.8 (IDF/SDK 4.4+).

The comments (always a feature inside code here at the .org - there is around 30m - 1h 30m of documentation _inside_ the sketch itself; depending on your reading speed (slow is best!); stuffed with tricks and tips. NOTE: Compilers remove comments. Just as well!) will _definitely_ be useful to ESP32 / Arduino n00bs and perhaps even more advanced users. I DO NOT give up, see.

My grappling and sometimes even mastering these usually inadequately documented technologies should hopefully provide some copy-and-paste quickness for your workflow, as well as gobs of in-situ documentation to inform and amuse.

I've tried throughout to make it easy to edit and understand; adding comments where things might be obscure (or I used the ternary operator!).

\*\* No really, the header files. Espressif has _stuffed_ these with notes.

#### And then it takes flight...

Of course, recognising the need for a complete, easy-to-use, working Signal Generator in the ESP32-sphere, I then kept going, fixing bugs, adding error-checking, features, AI..

OKAY, I got a wee bit obsessed with this thing; expending every spare moment on it; and it is now "Da Fuxn Bomba!"; web interface, presets, loops and all sorts. Most ESP32 tools I see are barely started let alone finished. That won't do for me and the boys. Expect finesse. But also no nonsense[\*\*](https://corz.org/ESP32/square-sine-triangle-wave-signal-generator/#Da_MO-MO).

It was also a lot of fun with the kids, testing their hearing limits and playing with various tones and buzzes. "Daddy. Are you STILL working on that tone thing?". Yup. Another gift to the world.

### The main web interface:

![image of the main Signal Generator Web Interface. Neat and clean and functional as can be. The background is white and the buttons are large and of a green that makes one think of nature and invokes calm feelings. The web page has been squeezed into portrait mode and Signal Generator has thoughtfully moved the signal information to the top of the page, between the mostly-opaque simple-interface and reboot buttons at either side. The button link to the console is absent in this narrow view.](https://corz.org/img/ESP32/ESP32-Signal-Generator-MAIN-Web-Interface.webp "The main ESP32 Signal Generator Web Interface")

#### Click the title for the built-in command console..

![image of the main Signal Generator Web Interface as above, except with the built-in console activated.](https://corz.org/img/ESP32/ESP32-Signal-Generator-Interface-built-in-command-console.webp "Click the title to enable the built-in command console..")

#### [Here](https://corz.org/public/scripts/ESP32/SignalGenerator/ESP32%20Signal%20Generator%20SAMPLE%20PAGE.html "ESP32 Signal Generator Main page - web sample..
it works better on your ESP32 device!") is a sample page.

### UPGRADING / UPLOADING:

Before upgrading _any_ version of this sketch, I recommend you first export everything like so..

> export all

Signal Generator will spit out a string of text you can copy and paste somewhere safe. Save.

With that string of text, you can get back all your settings and presets, loops, macros and so on at any time, anywhere, in a couple of seconds.

If you are upgrading over v6.3, definitely do an export, as some of the NVS types changed (to save space). After you upgrade, wipe the NVS and import your recent export; your old settings will be back but lean and clean, with the updated types.

Also note: uploading a sketch has undefined results on existing memory storage if you haven't restarted your device after any NVS changes, so rebooting is just a smart thing to do before you make _any_ firmware changes.

When uploading, remember to set the same partition scheme in Arduino IDE >> Tools. If you switch to a new partition scheme, your old NVS data will likely be gone.

I keep my preferred partition scheme at the _top_ of the list inside its `boards.txt` section, so I don't forget to set it.

If you have an up-to-date export of your data, none of this is an issue.

### Download:

## [Downloads And Sample Presets Are Here.](https://corz.org/public/scripts/ESP32/SignalGenerator/)

## USAGE:

To test, attach a small laser/speaker/motor/etc. (GND to one side, pin 25/26 to the other) and/or oscilloscope with one channel attached to pin 25 and another channel attached to pin 26, so you can see all waveforms<sup>*</sup>.

By default, square and sine are on pin 26, and triangle is on pin 25.

\* For single-channel oscilloscopes, you should be fine joining pins 25 and 26 together. ESP32 module GND attached to Ground anywhere on your scope, of course. [\*\*\*\*](https://corz.org/ESP32/square-sine-triangle-wave-signal-generator/#signal-generator-channel-mixing)

You can control Signal Generator via a serial connexion or remotely, via its built-in web server (point your browser at its IP address / domain name - the page is fairly self-explanatory and the HTML is in a separate file, so you can edit it easily) as well as by touch, potentiometer, etc..

The web page layouts are "responsive" and so work beautifully on any device; phone, tablet, PC, laptop; basically anything that can run a web browser.

The inputs attempt to be smart where possible. You can type in a value, hit enter: repeat; all while gazing at your oscilloscope; for example.

There is a built-in (the HTML is _inside_ this sketch, not an external file) "simple" page with only TWO HUGE BUTTONS, for frequency up and down at..

> /simple

Sample page [here](https://corz.org/public/scripts/ESP32/SignalGenerator/Simple.html "A Sample Simple Page for ESP32 Signal Generator").

**NOTE:** UP and DOWN (and LEFT and RIGHT) keyboard arrow keys work as controllers on the simple pages. The idea being you can control your signal generator with zero conscious thought. FOR SCIENCE!

There are other simple pages for pulse width (`/pwm`) and resolution bit depth (`/bits`).

Commands can be sent down the serial line, @ the plain `/URL` or via the groovy web console at `/console` (edit _that_ page in `Console.h`)

See [here](https://corz.org/ESP32/square-sine-triangle-wave-signal-generator/#Android_Serial_Control) for how to create a serial connexion from your Android device.

### Fully-featured command console at `/console` ..

![Signal Generator's stunningly simple yet fully-featured command console](https://corz.org/img/ESP32/Signal-Generator-Command-Console-with-command-output-result.webp "/console")

## Console Commands:

Send:

> c

To view a list of all the available commands ("`help`" and "`?`" also work -- though perhaps not obviously, one of those isn't going to work over the web).

#### Here is a recent version:

> s Sine Wave r Rectangle / Square Wave t Triangle / Sawtooth Wave \*\[k/m\] Frequency \[Hz/kHz/MHz\] +/-\*\[k/m\] Increase/Decrease Frequency by \*\[Hz/kHz/MHz\] b\* Resolution Bit Depth \[1-10\] p\* Pulse Width (Duty Cycle ~ percent\[0-100\]) s\*\[k/m\] Step size for Frequency \[Hz/kHz/MHz\] j\* Step size for Pulse Width (Jump!) \[1-100\] h\[f|p|b\] Touch Handler \[frequency|pulse width|res bits\] \[ \] Frequency Step DOWN / UP \\ / Pulse Width Step DOWN / UP (also - and +) z a Resolution Bit Depth Step DOWN / UP a\* Sine/Triangle wave Amplitude \* (1-4) Default is 4. \*n\[|o\] That's a literal \*! Play musical note 'n' at octave 'o' ~\[\*\] Delay for \* milliseconds (omit number to use previous time). d Overwrite Defaults (with current settings) m\* Save Current Defaults to Preset Memory \* l\* Load Preset Number \* (Think: MEMORY -> LOAD!) list List All Presets (and their settings - url: /list) lp\[e/d\] Toggle Layering of Presets \[enable/disable\] \[enter\] Print Current Settings (and restart generator) , Print Stored Default Settings (from NVS) k\* Print Stored Settings for Preset Number \* o\*\[k/m\] Override Limits, Set Frequency to \*\[Hz/kHz/MHz\] e Toggle Extended Info x Reboot Device l Load Stored Default Signal Settings sa\[e/d\] Toggle Save ALL Settings \[enable/disable\] ea\[e/d\] Toggle (Individual) Export ALL Settings \[enable/disable\] rt\[e/d\] Toggle the Reporting of Touches \[enable/disable\] up\[e/d\] Toggle Use Potentiometer Control \[enable/disable\] v\* Set the Handler/Accuracy of the Potentiometer \[p|f|b/1-100\] reset Reset to Hard-Wired Defaults (and reboot) w\[\*\] Wipe Stored Signal Settings \[for Preset \*\] wipe WIPE ENTIRE NVRAM (and reboot). Careful now! export\[\*/all\] Export Importable Settings \[for preset/all\] import \* Please Read The Fine Manual! loop\[\*\]=\[?\] Load \[?\] Commands into Loop/Macro \[number \*\] (RTFM!) loop\[\*\] Start Playing Loop/Macro \[number \*\] (RTFM!) end End the Currently Playing Queue/Loop/Macro @\* Set \* Volts directly on the DAC Pin (0 - 3.3). stop/. Stop the Currently Playing Signal (enter to restart) ll\[l\] List Loops/Macros (if available) \[single importable list\] mem Print Out Memory Usage Information cpu\* Set CPU Frequency to \*\[240/160/80\] MHz (and reboot) remote\[e/d\] Remote Control Toggle \[Enable/Disable\] wap/waa Set WiFi AP Only / Station + AP (and reboot) c Print Out Available Commands (this screen - url: /help)

#### NOTE: COMMANDS CAN BE CHAINED.

The semicolon character (";") is used to supply multiple commands at-once and for this reason CANNOT be used in preset names or anywhere else on the command-line, except for chaining commands.

So when I say, "use any character", I mean any character _except semicolon_, which is reserved for chaining commands.

#### `/URL` commands:

If you want to argue the semantics of URI vs URL, get in touch. Here, we're just gonna call it "URL" (and assume you know these are preceded by: `http://your-device`). So..

If you access your ESP32 device via _any_ URL that isn't known to Signal Generator (e.g. `/console`); rather than present a 404 page, that URL will be _translated into a command_.

If you send a request to `/100` you will set Signal Generator's frequency to 100Hz. You can use _any_ mechanism that can send a web request, to change parameters on your Signal Generator; `wget`, `cURL`, even just a tab in your regular web browser.

Hit <enter> (or click / tap the 'Send' button) when the console input is empty to see the current wave settings[<sup>+++</sup>](https://corz.org/ESP32/square-sine-triangle-wave-signal-generator/#signal-generator-awesome-web-sonsole) (not to be confused with your currently (NVS-)stored default settings ("," or "k")).

Commands are case-insensitive (everything is switched to lower case internally), so "t" is the same as "T", and easier to type. e.g.

> s

Will switch to sine wave output. `r` gets you rectangle / square wave and `t` is for triangle.

#### Frequency..

> 2000

Will change the frequency to 2000Hz (2kHz). Which is the same as (wait for it ...)

> 2k

You can use `k` and `m` to input kHz and MHz values in short format (including decimal fractions). This is fine:

> 1.25m

You can send values in Hz, kHz or MHz, so it can be as fine-grained as you need. Same applies to the frequency input in the web controller / web console (also for frequency step - basically _any_ value that is a frequency accepts any format of frequency input).

You can also nudge the frequency up and down by preceding the number with a '+' or '-'. For example..

> +1.25k

Would nudge the frequency up by 1250Hz.

#### PWM

Pulse Width Modulation is controlled with `p`, for example, to switch the pulse width to 1%, do:

> p1

The pulse width control works as expected in the square wave, just like PWM (though it will only be accurate at higher resolutions, otherwise it will be "close").

In the triangle wave generator you can use it to create sawtooth waveforms of different shapes; try it.

In the Sine wave generator it does exactly nothing (and therefore will not be reported in SINE mode).

You can set the minimum and maximum allowable frequencies for each generator inside your preferences. This is handy for preventing you shooting into crash territory, especially if you decide to fit this into a box.

#### Careful Now!

You can use "o" to override the preset maximum frequency on-the-fly (in Hz, kHz or MHz), e.g..

> o3.5m

#### Resolution

You can set the PWM resolution bit depth on-the-fly. The steps required to create a square wave at the new resolution are calculated automatically. Use any depth between 1 and 10, e.g..

> b6

This is way more fun than expected (like the touch controls). The lower the resolution, the less precise you can make the waveform's pulse width, but you can achieve higher frequencies. In other words, there is a trade-off between frequency and resolution.

A resolution of 3 bits gives us perfectly serviceable square waves right up to just over 10MHz. At 15MHz (2 bit), things are wee bit shaky, at 20MHz the wave steadies up beautifully but is losing shape.. at 40MHz (1-bit operation now) the square wave is a (quite beautiful) sine wave.

Wait a minute? A 40MHz Sine Wave??? Yup. More than one way to skin a cat!

If you don't need multi-mega frequencies, 6+ bits is recommended. Higher resolutions work better for odd frequencies, too, producing more stable waveforms, and if you need accurate pulse widths or extremely low frequencies, you need to use higher resolution (i.e. 6+ bit). At 10-bit you can produce a nice 1-second pulse (aka. darkroom / work-out (HIIT) / audible-seconds timer).

If you want fine control over a motor or laser (100Hz+ square wave works well, using pulse width to control the brightness - you can power it right off the DAC pin[<sup>+*+</sup>](https://corz.org/ESP32/square-sine-triangle-wave-signal-generator/#signal-generator-laser-control) I wouldn't go higher than around 8kHz with a pen laser; specs-wise, though I have personally tested my pen lasers at over 100kHz with no ill effects, but also no advantage. So long as it's not flickering (over 72Hz- ish), you are good to go) you will definitely want to use 10-bit operation.

See the table above the `switchResolution()` function for all the juicy numbers.

#### Info-Max!

There is "extended" information available. You can enable this in your prefs or send an 'e' on the command line to toggle the setting (it's enabled by default). When extended information is enabled, you can see any commands coming in from other sources, e.g. web console.

#### Potentiometer Control..

There is a potentiometer facility. You can attach a variable resistor and assign it to control frequency, PWM width (duty cycle) or Resolution bits. See the prefs for more details. Only enable this facility if you have actually attached a potentiometer!

As well as being useful and fun as a wave controller, it's also an excellent way to test potentiometers, especially if you have a heap of old ones you'd like to sort. Set a nice audible range, hook it up and twiddle. Dud pots are easy to find now and fixes (contact cleaner and fast twiddling) can be clearly heard; or not, if it's too far gone.

#### Button Control:

You can wire up a button (up to five buttons, and it would be easy to add more) and have them perform any command. See the button prefs for more details (and an example "Emergency Stop" button).

#### Touch Control..

There is a touch facility, to increase/decrease parameters quickly (kids love this). By default, it's frequency; UP is on GPIO Pin 33, and down on 32. You can set these in the prefs. The step value is 100Hz, but again, you can set this in your prefs, as well as on-the-fly with the 'f<number>' command (think "finger"). e.g. to set the frequency step value to 500Hz, do:

> f500

NOTE: These step values apply to _all_ controls that move a parameter up or down by a "step".

You can use kHz / MHz values, too, both in the serial console and in the web page inputs. For example, to set the step size to 2250Hz in the console you can do..

> f2.25k

In the web inputs you would simply do..

> f2.25k<enter / click "set">

You can send decimal fractions. .25k will be converted just fine to 250Hz. Though 250 is quicker to type! But you get the idea; maximum flexibility of input, everywhere.

You can also use 's' to set the step, but this is an "undocumented" feature, as it confuses some people that "s" is the command to set a sine wave and ALSO a way to set the step value. Signal Generator is smart enough to recognise an "s" with numbers after it, see. Use whichever you prefer.

### More controls:

Set the pulse width step size with 'j' (think Jump)..

> j5

Would set the Pulse Width Step (JUMP!) to 5%, which is anyway the default.

You can step the frequency up and down from the console/web console/URL with "\[" (down) and "\]" (up). Similarly, pulse width with "-" and "+", and resolution bit depth with "z" and "a".

You can switch the touch handler to Pulse Width:

> hp

Or to Resolution Bit Depth:

> hb

The default being frequency: hf or h\*anything-else. This command..

> rt

Will Toggle the reporting of touches to the console (even when extended information is enabled). See the notes next to the touch preferences for more info.

Like the other "special" (typically two character) commands, the convention is that the command itself toggles the setting; while adding an "e" enables and "d" disables, so..

> rtd

Disables reporting touches.

> rte

Enables reporting touches; for when you need to set an absolute value. This convention applies to "sa" (Save All), so "sae" enables and "sad" disables Save ALL settings, as well as "lp" (Layer Presets), so "lpe" enables, "lpd" disables, and "up" (Use Potentiometer - "upe"/"upd") and others. See help for more details.

Settings are remembered (stored in NVRAM, aka. NVS (Non-Volatile Storage)) so they will persist after a reboot or sketch upload. Use:

> reset

To reset to your defaults, as set below in the prefs (and reboot).

Send..

> k (or ,)

to see a printout of the settings currently stored in NVRAM (these should always be in sync with your current settings, except where frequency has been altered by touch, unless you have enabled storing this - see below - or loaded a preset^^).

You can send the command "wipe", to erase the NVRAM, removing all preferences (including those from any other programs which you might have had running on your device using the NVRAM for data storage). Be careful. No really, that's your entire NVRAM being wiped. Be careful.

NOTE: The wipe command can _only_ be sent over the serial connexion. Or else disable this security feature in your prefs.

If you only want to wipe the current preferences, use "w" (perhaps to start afresh for a brand new minimal preset). To wipe the settings for a specific preset, use:

> w<number of preset>

To save all your current settings, be it from a loaded preset or loaded+adjusted preset, to the current default settings (so that they remain on a reboot) use:

> d

Anything you change manually, automatically becomes the new default setting for that parameter, so it will be active after a reboot.

NOTE: I noticed the ESP32 keeps wifi connexion details in NVRAM (along with other stuff, amounting to 123+ NVS "entries"). If you have been having WiFi issues with your module, wiping the NVRAM should definitely be on your todo list. "wipe" will do it. [<sup>~~~</sup>](https://corz.org/ESP32/square-sine-triangle-wave-signal-generator/#Wonky-WiFi)

#### Wave Scaling:

Wave Amplitude for Sine and Triangle waves can be scaled to one of four levels (1/8th, 1/4, 1/2 and full wave (around 380mV, 760mV, 1.56V & 3.1V, respectively)), like so:

> a4

Level 4 (maximum) being the default. a1 is 1/8th scale (smallest amplitude; 12.5%), a2 is 1/4 (25%) and a3 is 1/2 (50%) scale. Square/Rectangle wave is always 100% (full wave ^\*).

NOTE: Sine waves scale nicely from the centre (as we are using the built-in cosine generator), whereas Triangle waves scale from 0,0. With any luck your scope knows how to compensate. Lasers, speakers and such won't care.

^\* I discovered that you can slam a voltage right onto the DAC before you setup a square wave and, in effect, set its amplitude. The result doesn't always _square_ with what's expected, but it is useful enough to have its own "experimental" setting, which is "@" + the required voltage, from 0-3.3. For example, to set a (roughly) 50% amplitude, do:

> @1.66

This _seems_ to set the minimum or mean Voltage, so the bottom of the wave raised. This has effect of decreasing the amplitude, yet potentially increasing the available current. If you are powering a laser at p100, for example (the theoretical maximum) and then slam 3.3V into the DAC (@3.3) your laser will get brighter. I'd love to know _exactly_ what's going on here!

### Presets:

You can save and restore presets, which are a snapshot of your current setting. We use the "m" and "l" commands, think: MEMORY -> LOAD!

> m3

Will save the current setup to preset number 3.

You can use any number from 1-50. When you save a preset, the available preset storage space is printed out to the console so you can see how much space you have left for storing presets. You can set the limit yourself in the prefs. For a default partition scheme, 50 is about right.

> l3

Will load that same preset. And so on. All waveform-related settings WHICH HAVE BEEN SET are stored in the preset, including the current frequency step size and touch mode.

If you want to see a big list of all your presets and their settings, do:

> list

The web interface is limited to 9 presets, numbered 1-9. In the web interface Ctrl+Click will save a preset. Click to load. This works fine on my Samsung Galaxy Tab (with keyboard-case) and various PCs/laptops, while Shift+Click does \*not\*. You could add more buttons if you need them.

By the way, you can always use "l" on its own, to load back the default settings stored in NVS (what you would get after a reboot).

If you want to see the stored settings for a particular preset, use "k<number of preset>". For example, to see the settings stored for preset 9, do:

> k9

If the chosen preset does not exist\*\*, you will be notified; otherwise you see the settings. Why "k"? Well, 'k' comes before 'l' and 'm', see. Think: "know" or "ken".

You can add a name to your preset this way..

> n<preset number><preset's new name>

For example, to set the name of preset number 7 to "Laser Control", do..

> n7Laser Control

While this might seem trivial, you have the option to have this information appear as a mouse/ pen/stylus/whatever hover on the main web page. And while you might find "3" particularly memorable, most humans like "Laser Control" better. If you have a lot of presets, names make them much easier to find in a list of presets.

\*\* The first time _any_ data is stored to a preset, its "id" is set. Until this id is set, a preset (50 (or whatever) are created at first run) is just an empty namespace and won't be shown in preset lists, exports, etc. Setting the id "activates" the preset, so to speak.

NOTE: "Laser Control" has 13 characters, which is fine. The bigger your preset names, the less presets you can store before running out of "entries". But if you only intend to have a few presets, feel free to call it, "Laser Control with 1Hz step, beginning at 5% with 10 bit resolution".

A single entry is used to store the index for the name, and another entry for every 32 characters in your name. So if your name is 31 characters long, you use 2 entries total. If your name is 32 characters long, you use 3 (there is always a null byte (terminator) added to the end and this is included in the total number of characters).

**NOTE**: If you want to use any funky characters in your name, it's probably best to set the name from the serial console, so it doesn't get caught up in the URL encoding/decoding process.

For obvious reasons, names of presets 1-9 cannot begin with a number! At least not without specifying it with an '=' character, like this..

> n1=1 way to do it

You can use the '=' construct for all your naming tasks, but it is only _required_ for naming presets 1-9 with a name beginning with a number. Otherwise the short-format works in the same way, and is quicker to type.

If you want to update a preset, simply save it again. The name will still be there (name is _never_ saved with a preset; you must name presets manually) and your settings will be whatever they were when you saved the preset. (l3 > tweak > \[d >\] m3)

On my current dev board (today it's the Wemos D1 R32) I have 630 "entries". The chip uses 123+ of these to store its own settings (Including WIFI SSID info), which seems excessive, leaving 506 free "entries". If you keep your names short, you should easily manage 40+ before the space runs out. Without names you should be able to use more than 50. A preset uses up to around 11 entries, not including the name.

When you boot up for the first time, 50 "namespaces" are created to hold your presets, using up 50 entries, _just-like-that_.

You can set the maximum in your prefs but for the standard NVS (Non Volatile Storage) partition size, 50 is good. If you need to increase the size of the available NVS / NVRAM used for preferences/presets, check out the notes below the presets preferences.

As mentioned, whenever you change something, the new setting becomes your new default for that parameter and will be stored in NVS and active on reboot, but NOTE:

Loading a preset does _not_ overwrite your currently stored default settings, which are also held in NVS. To do so would be to delete user data (your settings prior to your loading the preset, which we assume were mindfully created).

If you load a preset and want to get back to your previous settings, Send "l" on the command-line, or simply reboot. Your signal will be back in milliseconds, and your web control a couple of seconds after that (assuming your router works well). You can think of it as a sort of preset UNDO facility.

NOTE: Saving a preset saves _only the settings which have been actually set_. This is by design. "Layering" enables HUGE flexibility as well as space savings, but you need to keep your wits about you to use it! If this sounds too complex, layering can be disabled. Read more about this in the preferences (layerPresets).

#### Copying Presets..

You can copy the settings from one preset to another preset. For example, to copy all the settings from preset 4 to preset 20, do:

> copy4>20

or..

> copy 4 > 20

Spaces are optional. Everything is copied, including name (with " (COPY)" appended) as well as any loop commands stored inside the preset.

NOTE: If there is already preset data at preset number 20, it will be overwritten. No warning.

NOTE: Making a copy of a copy will get you automatic numbering. More sorta-AI.

#### Import / Export

You can load multiple signal settings at-once on the command-line, like this..

> import m=r,p=50,b=7,f=5.22k,s=100,j=5,h=p

The Space (" ") and equals ("=") characters can be replaced with anything you like, they are simply delimiters which are ignored. If you are doing this over the web, be careful to use characters which will pass through browser encoding unmolested, like space and equals will. The commas are important and cannot be switched for other characters.

Like all commands, you can send this as a plain URL and set everything-at-once from _anything_ that can send a web request (like your browser)..

> http://MyDevice/import m=r,f=100,p=10,b=10,s=1,j=5,h=p

or

> http://MyDevice/import%20m=r,f=100,p=10,b=10,s=1,j=5,h=p

> http://MyDevice/import+m=r,f=100,p=10,b=10,s=1,j=5,h=p

> http://MyDevice/import~m~r,f~100,p~10,b~10,s~1,j=5,h~p

or whatever.

HTML Encoded entities coming in over the web will be decoded before processing; maybe useful if you are scripting.

Key:  
<small>(this also functions as a guide to <em>which</em> settings are saved with a preset)</small>

> m == mode, which can be r, s or t (rectangle/sine/triangle)

> b == resolution bit depth, any number between 1 and 10

> f == frequency, in Hz, kHz or MHz; decimal fractions are fine, e.g. 2.252m (can also be a -/+ nudge)

> s == frequency step size

> p == pulse width (%), any number between 0 and 100 (note: at 0 and 100 a square wave will of course be flat)

> j == PWM step size (%) (only relevant for square and triangle waves (sawtooth position %))

> h == Touch Handler, can be f, p or b (frequency/pulse width/resolution bits)

> a == Amplitude scale. For Sine and Triangle waves (1-4). 4 (the default) is full wave.

Value pairs can be written in _any_ order, but **MUST** be separated by commas. Settings loaded in this way are saved to NVS as your new defaults (and will be active on a reboot). So it's a quick way to load all the settings for a new preset; handy when layering is enabled.

If you are performing any trimming operations (removing unused types), for optimal results be sure to put the mode _first_, as in the above examples.

You can also import settings directly into a preset (and they will _not_ become your new default settings) by appending the number to the "import" command, e.g..

> import12 m=s,f=500,p=50,b=10,s=50,j=5,a=2,h=p

or (with a space)..

> import 12 m=s,f=500,p=50,b=10,s=50,j=5,a=2,h=p

Which would import settings directly into preset number 12. Importing settings this way will _not_ save the settings to your defaults, which remain unchanged. This is also the format Signal Generator uses for _export_..

Yes, you can also have Signal Generator _create_ such a string, to copy+paste into a different generator, or at some other time; from either your current settings..

> export

Or a given preset..

> export4

or..

> export 4

Both will work. This format is also fine, if you prefer (or if you are scripting this)..

> export=4

It's conceivable that you might save a preset with a square wave and therefore save bit depth and pulse width and then later switch that preset to a sine wave, where these settings no longer apply. BUT they are still saved within your preset. Or some similar situation (if you switch back to a square wave, your old settings will be applied, is the reasoning behind this). If this isn't useful/annoys you, do (assuming we are working with preset 3..):

> l3 > w3 > d > m3 (if layering is disabled, you can omit the "d" step)

You will need to set your name again, e.g.. n3I _really_ care about space!

Once you have everything setup the way you want it, you can export _all_ your settings in one go, like so..

> export all

Which will output something like:

> w1;import1 m=r,f=100,p=25,b=10,j=5,h=p;n1=100Hz Laser Control;w2;import2 f=222;n2=222Hz Laser Control;w3;import3 f=333;n3=333Hz Laser Control;w4;import4 f=440;n4=440Hz Laser Control;w5;import5 m=r,f=1.5k,p=50,b=10,s=50,j=10,h=p;n5=1.5kHz ~ 50% PWM Laser Control;w;import m=r,f=1k,p=95,b=10,s=50,j=5,h=p;

Which is a string you can send to Signal Generator to have _all_ your current settings and presets imported all-at-once. And of course, you can send this command as a URL. See the function's comments for details.

If you have a heap of presets with non-applicable settings (e.g. bit depth saved inside Sine wave presets) you can clean them all up in one go by doing "export all" and then adding a "sad" command (Save All Disabled) to your new import string, e.g..

> wipe;import1 m=r,f=100,p=5,b=10,h=p;w ... \[other import commands here\]

becomes..

> wipe;sad;import1 m=r,f=100,p=5,b=10,h=p;w ... \[etc.\] \*+\*

Import that, and any settings which do not apply to the current waveform will be ignored. If you do a fresh "export all", you will find that it is now 100% clean!

\*+\* NOTE: Import commands beginning with "wipe" can only be performed from the Serial Console. Unless you disable this in your prefs.

NOTE: If you want this SaveALL mode-switching-clean-up smartness to work, you _must_ put your mode setting \*first\*, like the export command does (e.g. "m=r").

This is as good a place as any to tell you about the "AI" part of Signal Generator. It's a misnomer, of course; Signal Generator just tries to interpret intention as best it can and create flexibility of input.

You could export all your presets like this..

> Exporting of Allllll the presets! NOW please sir!

Quite _why_ you might want to do this, is beyond me. But Signal Generator was designed from the ground up to be intuitive to operate and forgiving in operation, so that command will work exactly as expected. This amounts to a "Sorta AI". Check out the comments around the various functions for cute shortcuts and back-doors-to-functionality. This would also work:

> exportall

IMPORTANT NOTE: If you want to chain long imports after a wipe command, keep them to under 4000 bytes. If your import is longer than 4000 bytes, perform the wipe manually and after the reboot, do your import. The maximum length of a string that can be stored in preferences is 4000 bytes.

Same applies to your loops and macros. If you need more that 4000 bytes of loop data, you will need to split it into multiple loops (chain them). Yes, loops..

#### Loops / Macros:

There is a simple looping/macro facility.

To load a loop, use "`loop=<commands to loop>`". To play, use "`loop`". Done.

> end

will end any loop/macro and/or any running queued commands immediately. Remember this one!

> stop

or

> . (point / full stop - only works over serial connexion)

will stop the currently running signal altogether. This is useful if your setup includes a speaker.

Here is an example loop which will gently pulse a laser/speaker, etc. (for _ever_)..

> loop=p10;~70;p15;~14;p20;~13;p30;~12;p40;~11;p50;~10;p60;~11;p70;~12;p80;~13;p90;~14;p95;~10;p100;~75;p95;~66;p90;~20;p80;~18;p70;~17;p60;~16;p50;~15;p40;~16;p30;~18;p20;~18;p15;~25

That's all one line, of course. Before you play the loop/macro, you will want to setup your chosen waveform and frequency. You can do that manually, or chain the commands in front of the loop commands..

> r;1k;b10;loop=p10;~70;p15; etc...

We start by setting rectangle wave, then 1kHz frequency and 10 bit resolution, then the commands to loop, here simply setting pulse width, delay (~), pulse width, delay, etc.. That's it.

Better yet. Load up a preset first..

> l4;loop=p10;~70;p15; etc...

NOTE: During loops, extended information is disabled, regardless of the current setting.

ALSO NOTE: There are only two ways to exit a loop. The first is to power off your ESP32 device. The second is with the previously mentioned "end" command. i.e. hit the following URL:

> /end

You can also end loops via the Web Console.

The "end" command can also be tagged onto the end of the actual loop, converting the loop facility into a macro playback facility. So I guess there are three ways. We call them loops because this is the default behaviour. If you want play-and-stop, you need to add an "end" command.

You can also send the "end" command over the serial line. In fact, the "end" command is the ONLY text command which will be processed during loop/macro playback; all other commands being dropped.

NOTE: If you are in the middle of a delay command, your "end" takes effect _after_ it completes. If you need a command that can break into a delay, assign it to a physical button.

NOTE: Touch commands _will_ be processed during a delay.

Once a loop has been loaded, its commands are retained in memory. If you wish to start/re-start the previously-loaded loop/macro, send the following command:

> loop

You can save more than one loop/macro, storing it in a preset's memory[\*\*\*](https://corz.org/ESP32/square-sine-triangle-wave-signal-generator/#Preset-Hijack) for example..

> loop4=p10;~70;p15;~14;p20;~13;etc..

Would save the loop commands to preset 4. If you prefer to add spaces around the initial command, this..

> loop 4 = p10;~70;p15;~14;etc..

Would also work fine. Signal Generator tries to be smart about interpreting commands.

By default your loop commands are stored to NVS, so you can keep a set of commands handy at all times.

NOTE: If no loop commands are available in default storage, Signal Generator will load the most recently-loaded loop commands, if they exist.

In other words, if you started from a clean NVS and loaded the above command (into preset 4) and then did:

> loop

The loop commands from preset 4 would run; as there are no default loop commands stored. See?

To start looping a specific set of commands stored in a specific preset, do..

> loop4

or..

> loop 4

To see a list of all your loops (the output lines are also importable), do: (Think: "List of Loops")

> ll

To get that same list as one long importable line (Think: Long List of Loops!), do:

> lll

ALL commands after the "loop=" are considered to be commands IN the loop; in other words, you can't chain other kinds of commands after a loop load command. However, you could finish off your loop commands with an "end" command, as mentioned, or even _another_ loop command.

WHAT?!? Consider this..

> loop1=p10;~200;~200;p20;~200;p30;~200;p40;~200;p50;~200;p60;~200;p70;~200;p80;~200;p90;~200;p100;loop2  
> loop2=p10;~30;p15;~9;p20;~8;p30;~7;p40;~6;p50;~5;p60;~4;p70;~3;p80;~2;p90;~1;p95;~1;p100;loop1

Now do:

> loop1

Result: loop1 will play one time, then loop2, then 1, then 2, and so on, until stopped.

If you remove the "loop1" command from the end of loop2; instead, loop1 plays once then loop2 loops until stopped.

You could use loop1 to run setup commands then switch to loop2 for the actual looping..

> loop1=: Awesome Loop setup ;r;1k;b10;loop2; loop2=: Awesome Loop ;p10;~30;p15;~9;p20;~8;p30;~7;p40;~6;p50;~5;p60;~4;p70;~3;p80;~2;p90;~1;p95;~1;p100

Or create a self-destructing macro..

> loop2=: Self-Destruct! ;p10;~30;p15;~9;p20;~8;p30;~7;p40;~6;p50;~5;p60;~4;p70;~3;p80;~2;p90;~1;p95;~1;p100;w2;end

Maximum flexibility!

Yes, you can add a name at the start of your loop/macro, so you can tell what it is at-a-glance. Use anything with a colon (":") in front of it (commands beginning with a colon are ignored).

This loop chaining facility is also handy of you have a MASSIVE (>4000 character) loop. 4000 characters is the maximum you can save in a single loop/macro, but you can split it up into multiple loops and have them call each other.

You can load multiple loops/macros on the command line. In fact, the only type of command which can come _after_ a loop loading command, is another loop loading command (anything else is considered part of the initial loop); so yes, you can load all your loops at-once.

A Signal Generator export (the loop part) looks something like this..

> loop1=:Slow pulse ;p5;~250;p90;~200; loop2=: Fast Throb ;p10;~300;p15;~9;p20;~8;p30;~7; loop3= etc..

NOTE: I put a space _after_ a name, to make it easier to read in a (web) list of presets (it will wrap after the name instead of in the middle of it). Or two spaces, one at each side. Any trailing semicolons are removed from loop data before saving.

If you have loops/macros stored, during "export all", they will be tagged onto the end of your export command. When you import this, your Serial console may go mental for a second (or a few seconds, if you uncomment those debugging lines). But it will work perfectly.

Export commands always export _all_ data from a preset's namespace; signal data, name and loops, unless you specify otherwise (in your prefs).

You can wipe a loop/macro by sending a loop load command with "-" as the loop; e.g. to wipe the loop stored at preset 12 do:

> loop12=-

If there were no other settings in the preset except the loop, the preset is also removed.

NOTE: Spaces are NOT allowed here (if you want it to work). If you do "loop12= -", you fail.

As these are essentially loop loading commands they too, can be chained together.

NOTE: If the preset had a name (and _only_ a name), that does not count as "data" and won't prevent a preset being cleared by a loop wipe. _Any_ other setting, will. If you want to keep the name, make a copy\*>\* first!

#### Musical Notes

There is a rudimentary musical note API. Send an asterisk (\*) followed by the note you wish to play. You can optionally specify an octave (from 0-8, 4 being the default) by putting a pipe after the note, followed by the chosen octave.

If octave isn't specified, it remains set to whatever was previously set. You can also specify "+" to shift the octave up by one octave and (predictably) "-" to shift it down.

> \*a

Will play "International A", which is a 440Hz tone. This can be applied to _any_ waveform; be it Square/Rectangle, Sine or Triangle/Sawtooth.

> \*a|6

would play the same note, two octaves up.

> \*a|-

would play the same note in octave 5 (if you sent the command while in octave 6).

All the usual note values are recognised: C C# D Eb E F F# G G# A Bb B, as well as technically correct alternatives such as D#.

While designed for outputting single notes at the correct musical frequency, this could certainly be used for generating music, like this rendition of Baa Baa Black Sheep which is apparently incorrect, but is the way I remember it in my head..

> loop15=\*g|4;~500;\*g;~500;\*d|+;~500;\*d;~500;\*e;~250;\*f#;~250;\*g;~250;\*e;~250;\*d;~500;.;end

NOTE: Send a delay command ("~") on its own to set the same delay as was previously set. For example; this produces the exact same genetically distinctive bovid as above, but is less code..

> loop15=\*g|4;~500;\*g;~;\*d|+;~;\*d;~;\*e;~250;\*f#;~;\*g;~;\*e;~;\*d;~500;.;end

Send _any_ other character (or simply hit <enter>) to restart the generator and print out all the _current_ settings (to print out settings and NOT restart the generator, send a ":" (colon). As mentioned, any command preceded by a colon is ignored (used for loop names/comments)). You can also send "!" (exclamation mark/point), which is ignored in a similar way; used by the web console.

Lastly, there is an "x" command, to reboot the ESP32 module.

There are probably other commands I missed here. Send "c" for the current list. Maybe using..

### The Web Console:

(the first time a new client loads this page, the entire command list is printed out)

If you navigate your browser/phone/whatever to /console you get a simple console interface where you can enter commands in exactly the same way as you can in the serial console, except you don't need to be physically attached to the ESP32 module to use it.

Almost all the commands that you can send in the serial console can be sent in the web console and you will receive feedback right there in your browser (Signal Generator stores a "Last Message" which the console page requests with AJAX after each command).

All the code for this page is inside `Console.h`, so you can edit to your heart's content (though I definitely recommend using a plain HTML test file for any actual editing, so you can see any changes without needing to recompile Signal Generator - copy/paste everything between the HEREDOC encapsulation).

This console is so good I inserted it into the main page. Click the title at any time to activate. I wouldn't use the "list" command in there (there's simply nowhere to put all that output. Besides, this data would flash by only briefly. For this stuff, use /console), but it is super handy for sending commands that aren't available on the main page; i.e. admin stuff, or renaming or accessing preset memories there aren't buttons for without opening up a whole new page, especially on devices with limited screen real estate, where switching tabs can be challenging. Also, a title is just taking up space. You know what this is.

I think that's about it.

Feel free to take this working example and do what you will. I always get a buzz when folk do that and then let me know (esp32@ the usual domain).

Enjoy!

;o)

ps. If you want to donate, go here: [https://corz.org/corz/donate.php](https://corz.org/corz/donate.php "Donate to corz.org.. Thank you!") Thank You Very Much!

### Frequency Ranges:

`Square/Rectangle ++`

`1Hz to 40MHz`

`Triangle/Sawtooth ^^`

`153Hz to 150kHz`

Pulse Width (Duty Cycle) / Sawtooth control from 0 to 100%

~~ You can easily output a 2 or 3MHz sine wave, though as you increase the frequency, the waveform loses integrity; so above around 500kHz, a sine becomes more a messy sawtooth. Of course, this might be just what you need. Proper sawtooth is also available. The lowest you can go with the sine wave is around 16Hz.

++ A 40MHz square wave is possible, if you don't mind your square wave being a sine wave. That's right, a 40MHz Sine Wave is possible! At 20MHz, the waveform is still square-ish on my scope and certainly usable. And a 10MHz square wave is pretty darned square. The lowest you can go is 1Hz (1/second - very handy as an audible timer). To go lower than this we'd need to delve into RMT.

^^ A 40kHz perfect triangle wave is easily doable, yet a 27kHz wave may be bottomed out. It was originally designed to operate up to 20kHz. Similarly, 130kHz will be bottomed out but 150kHz (the maximum - you can go higher, but it's _real_ messy) looks fine. The lowest frequency you can hit is around 153Hz. I might look more into all this. Or you might.

### CAVEATS/TIPS:

-   If you set some frequency and then switch waveforms, the frequency remains at whatever was set with the previous waveform type, but WATCH OUT!: setting a frequency in sine and triangle wave will get you an _adjusted_ value. With Sine waves at least, much more so as the frequency increases. Triangle waves can be surprisingly accurate, considering what we're up to.
    
    In other words, if you want an _exact_ 1kHz square wave, set that frequency _after_ you select square wave. All this information is reported to your serial console.
    
-   Similarly, if you set the square wave frequency to something that cannot be achieved, you can lower the bit depth and Signal Generator will try that same frequency again at the new bit depth. Repeat until you hit your required frequency.
-   If the Triangle wave is bottomed, just send \[enter\] (repeat!). Boom! Touch UP then Down the frequency will usually do the trick, too. I mean literally, you have to bang it, like an old TV set.
-   There is a certain amount of bleed between the DAC channels, so with the channels joined together you can expect a slight increase in amplitude and some noise. While this is fun for testing the code and playing with sounds with the kids, you will probably want to use a single channel only for any _real_ work.
-   If your square wave looks flat, check your pulse width and resolution bit-depth setting.
-   If your 1MHz (and above) wave looks like a bad sawtooth, check you didn't flip the x1 / x10 switch on your oscilloscope probe. There's a quarter hour of my life I won't get back.
-   If you are Googling-and-coding, ignore all comments about "String" functions being the devil incarnate. These are from folk using actual Arduino boards (as opposed to Arduino IDE (which the Espressif devs have done a sh\*t-ton of work to make their chips compatible with)) or else folk still stuck in that way of thinking (i.e. an 8-bit 16MHz board with 38kB RAM vs the ESP32 32 bit chip running at 240MHz with 520kB). I definitely recommend you get to _know_ regular C- strings, but the "String" is super-useful in the Arduino environment and with the advent of ESP32, no longer hobbled my memory / heap limitations. YOU CAN USE IT!
    
    As an example, I had a chunk of code using C-strings that was 20+ lines long. I even asked Chat GPT if it could make it smaller, because I felt that it should be. No dice. Then I switched to Strings. That same code is now two lines. Two. I shudder to think of all the extra code I would need if I constrained myself to C-strings.
    
-   Web functions on my WROOM boards can slow a bit at 20MHz, but are fine at 40MHz. If you want to play with signals around 20MHz, _first_ load up your web interface with a slower wave. Subsequent AJAX requests are far less taxing. On a WROVER board, this is not an issue. Something to do with memory limitations, then.
-   The accuracy of the timings in these wee units is pretty good, but I wouldn't use it for anything requiring _scientific_ accuracy. If I run two modules (one WROOM, one WROVER) with the exact same signal on channel one and two of my scope, the signals will drift apart over time (the WROVER will drift the most, by a large margin).
    
    At 1kHz (square wave) I takes around 264 seconds to drift 1ms, so overlapping waves now overlap the _next_ wave. Comparing my two WROOM units, the drift is only 104 seconds and this movement can by clearly seen on a scope. I would say the chances of finding two modules that match _exactly_ is slim-to-none.
    
-   \*W\* When I work with MCUs on Windows, it's a PITA. When I switch boards I'm always having to ensure I have the correct upload mode, or frequency or speed. On Linux it just works. I leave the upload speed at maximum and ignore all the other settings - the IDE-to-board communications work out everything else. I can just switch board and upload. It's totally pain-free. Just saying.
-   cpu\* sets the CPU frequency to \*MHz. e.g. cpu80
    
    If you are looking to conserve power, this is a good option.
    
    Allowed values for ESP32 are 240, 160 and 80. If you set this when remote is enabled, your device will reboot immediately so that you don't get caught in wifi-reconnexion hell. No amount of sleeping/pausing gets around this (bug) on any of my devices.
    

## TESTED ON the following ESP32 boards..

> (with Arduino IDE 1.8.19+ running on Slackware-current)
> 
> ##### WeMos D1 R32 "UNO" \[ESP32-D0WDQ5 Rev 3\] (superb)
> 
> https://www.aliexpress.com/item/4001118194342.html
> 
> The OMG! How cute is that! board. Looks like an Arduino UNO, but sports a lovely WiFi+ Bluetooth ESP32. WROOM32-based. Signal Generator was initially developed using this board (I had another Signal Generator setup on the DevKit board at the time).
> 
> This is a great board, and all the initial kinks seems to have been worked out. If it turns out I can actually slap UNO shields on it; wow, what a bonus! Also, it has a nice blue power lamp, unlike the nasty bright red one on the DevKit board (I put white-tac over most of my red LEDs (definitely anything that _stays_ on) so they are _just_ visible and no more).
> 
> When I compare the 40MHz square wave from this board with the wave on from the DevKit board, THIS board produces a smoother, more stable waveform, with less deviation. I suspect the bigger PCB size has something to do with it. Or other factors.
> 
> For touch on this board, you can switch the GPIO assignments to other pins (easy - it's in the prefs) or better yet, attach a PCB connector to those three pins (IO15/32/33) on the right side of the board. You will probably want to do this at some point anyway, so now is as good a time as any. It's literally a 2-minute job. High GPIO pins, apparently, are better than lower ones, though I haven't yet tested the differences myself.
> 
> Breaking out ALL the available pins to usable pin sockets seems like a Very Good Idea. Unless, of course, it interferes with the fitting of some mythical UNO shield you have planned.
> 
> \* I noticed with this board that the touch on pin 33 would make occasional (a few every second) random jumps to 0, tripping the frequency UP switch if a cable of more than around 100mm was attached to the pin while creating very high frequency waves (tens of MHz). This erratic behaviour is not seen on pin 15, nor on the default DOWN pin (32). Odd.
> 
> In my experience, this board also provides the most trouble-free WiFi connexions and will connect almost instantly, even when the kids are all playing Minecraft over the WiFi, unlike the DevKit board, which often enters a "fingers-crossed" state.
> 
> Sadly this board is fitted with the poorest of all the USB connectors: Micro USB. \*sigh\* There had to be one fly in the ointment.
> 
> ##### DoIt DevKit V1/3 \[ESP32-D0WDQ5 Rev 3\] (excellent)
> 
> https://www.aliexpress.com/item/1005004435118716.html
> 
> A fantastic WROOM32-based AliExpress DoIt clone. DO NOT connect the ground from the 3.3V side to the ground of the 5V side when powered by USB. You have been warned. I've never tried this, but I'm told it's fatal; for the board, that is. I'm tempted to try it as this board exhibits none of the issues I see on forums. Contrary to popular belief, Chinese manufacturers DO read the forums (where they often steal designs for gadgets they can sell) and DO upgrade their products to correct flaws. The last thing they want, is refunding.
> 
> Once this Signal Generator was better than the one I had previously running on this board, I switched development over to this board, and I'm always switching back and forth between the boards with upgrades, so I'm confident it works great on them all.
> 
> Currently (2023-1) you can get the module + expansion board delivered for under a fiver!
> 
> NOTE: I recommend getting the expansion board with the module; for just a few pennies more you get smooth voltage regulation with THREE power input options, including USB-C\*; as well as breadboard-free easy access to all the pins and independent 3.3V/5V outputs. Pennies. And of course you only need to ever buy _one_ of these; fitting various DevKit boards coming-and-going into projects ...
> 
> ![Image of the ESP32 DevKit module and accompanying expansion board](https://corz.org/public/scripts/ESP32/pics/ESP32-DevKit-Development-Board+Expansion-Board-TYPE-C-USB-CH340C.webp "The first time you get one of these boards
> I recommend you also get the exansion board..")
> 
> It's _real_ easy to mindlessly plug your USB-C cable into the /expansion board/ (everything powers up as usual) and then expect upload to work. It will not. You go, "Oops!", and plug directly into the module, instead. I've done this twice, at least. The expansion module is for power and pins only. Also note, those three power inputs are _exclusive_ and you must only use ONE AT A TIME!
> 
> Sadly the module is too wide to fit comfortably into a breadboard. You will only be able to _easily_ access the pins on one side. When I need one of these in a breadboard, I slam flat jumpers underneath to any pins I need, even directly out the side to the GND/3.3V lines. Works great. Feeding jumpers directly out the top to spare breadboard rails is even easier. So long as you ensure there will be no metal-to-metal contact when the module is pushed into place, you are good to go. It's either this or cut the breadboard, or perhaps reconfigure a breadboard to a power + 2 x main strip + power configuration, like those weird double breadboards you see on AliExpress.
> 
> There are some pics of this setup in the parent directory.
> 
> I like how, when powered from the breadboard, I can plug in the on-module USB, upload and disconnect again with no issues whatsoever. Similarly, I can slam in the breadboard power when I'm already connected via USB. I expected this sort of thing to make $h\*t go boom.
> 
> I notice that there is no "Verbose" option in the Arduino IDE menu >> Tools >> Core Debug Level for this board, however, the module _does_ produce these messages, if requested.
> 
> If you are debugging you want maximum information. Fortunately you can hack your boards file to include the extra menu item, and thereby the messages:
> 
> > esp32doit-devkit-v1.menu.DebugLevel.verbose=Verbose  
> > esp32doit-devkit-v1.menu.DebugLevel.verbose.build.code\_debug=5
> > 
> > ~/.arduino15/packages/esp32/hardware/esp32/2.x/boards.txt (or thereabouts)
> 
> \* This board has a native USB-C socket. Nice..
> 
> I once pulled a USB-C cable off a high shelf, not realising there was a Samsung Galaxy Tab attached to the other end. The thing went flying across the room, bounced and skidded across the floor, still connected. Both socket and plug have worked flawlessly ever since.
> 
> I could give you more stories like this involving USB-C connectors, and also USB-mini connectors. Many stories. All devices and connectors still working fine to this day.
> 
> But _all_ my stories involving Micro USB connectors and anything-more-than-a-gentle-wiggle invariably have unhappy endings. What a poorly designed connector. USB-C-EVERYTHING!
> 
> ##### Freenove ESP32 WROVER module (good) (came with the 'Ultimate starter kit' - a gift)
> 
> https://www.aliexpress.com/item/1005004339971186.html
> 
> Sine wave is messed up on pin 26, but fine on 25 (with a little interference) and it doesn't like to have the two DAC pins connected, or else the triangle wave is just as bad. In other words, put _everything_ on pin 25/channel 1 for this board.
> 
> ##### NodeMCU WROVER-IE (very good)
> 
> https://www.aliexpress.com/item/1005004571486357.html
> 
> Nice 38 pin board with 4MB PSRAM, internal + external (IPEX/UF.L) WiFi antennae connexion.
> 
> I'm using the standard "ESP32 WROVER Kit (all versions)" board definition for this.
> 
> This board works flawlessly (at least when not in reference to _anything else_) in all my tests, with everything at the regular defaults. It also has no issues spitting out web pages when running a 20MHz square wave, leading me to conclude that the smaller boards' memory is constrained during web tasks. This board has a HUGE amount of memory.
> 
> The UNO D1 R32 board still wins out for 40MHz wave stability, but only just.
> 
> This WROVER board is less able to process _really_ fast (1ms) loop delays. Curious. As mentioned elsewhere, when running triangle waves at least, its frequency also has a tendency to _drift_.
> 
> I have no shield for this board and it annoyingly has no pin numbers printed on the front, meaning you need to keep your wits about you working with it in a breadboard. I find it easiest to slam it into the very top so pin one matches pin one of the breadboard, then put hard-wire red and black connectors between 3.3V and the + rail, and GND to the - rail, so you can see at-a-glance where the numbers are, and count that off from a nice pic you have of the underside (with maybe 5, 10, 15, overlaid by your local image editor).
> 
> Such a pic can be had in the directory above the one containing this sketch.
> 
> Sadly, this board comes fitted with a Micro-USB connector. By habit I now give all new Micro-USB connectors a wee squeeze on first contact.
> 
> I should mention that while this board comes with an IPEX (or UF.L or whatever it's called) socket, which is attached (meaning the PCB antenna is _not_ - I have confirmed this with close-up inspection), I have been happily using it with NO antenna attached, and with no WiFi issues at all. It's connecting to a WiFi router in the same room, but still; fairly impressive, and most useful for development scenarios. Not to mention the durability issue..
> 
> For those who are unaware; even a good quality IPEX socket is only rated for about 25-30 re-insertions. Meaning it's designed to have an antenna placed there ONE TIME, during assembly. These sockets are NOT designed for regular use, like a BNC or headphone socket is. Be careful.
> 
> It should be noted that this board is a notch narrower than the DevKit board, so you can fit it into the middle of a breadboard and access _all_ the pins on _both_ edges. Nice!
> 
> ##### ESP32-CAM \[AI-Thinker\]
> 
> I didn't bother to test on this thing as it has no DAC pins (ESP32-S).

#### Board definitions:

> [https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package\_esp32\_index.json](https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json "Board definition")
> 
> [https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package\_esp32\_dev\_index.json](https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_dev_index.json "Board definition")

## Footnotes..

### Android Serial Control for your ESP32

To get a _serial_ connexion on Android, procure a quality OTG adapter for your Android device and then see here:

> [https://play.google.com/store/apps/details?id=de.kai\_morich.serial\_usb\_terminal](https://play.google.com/store/apps/details?id=de.kai_morich.serial_usb_terminal "Add elastic bands and got yourself an instant mobile rig with serial and network control. Boom!")

Go to the settings and set the speed of the serial connection (115200) and the number of macro button rows to 2 (or more\*) and whatever other settings you fancy. Plug in your ESP32 device (via OTG adapter/cable) and hit connect.

This makes for a great quick-and-dirty ad-hoc setup for portable laser controlling. Your phone can be the power source _and_ (wifi AND serial) controller all-in-one. Just add gaffer tape (or masking tape or rubber bands or whatever).

\* at least 2 because Boom! They are named L1-L10 and M1-M10; perfect for loading and saving Signal Generator presets!

#### <sup>**</sup>M.O.:

Signal Generator doesn't hold your hand. If you need this, install a dating app. If you send the command: `wipe`, your entire NVS will be wiped, without warning. Same for _any_ command.

If you plan to type random characters into your Signal Generator console, please use Signal Generator's robust import/export facilities to preserve your data!

#### <small>*** </small> Preset Memory Hijack

It doesn't need to be _related_ to the preset - we're just hijacking its namespace for storage, so we can store more than one loop without using up extra namespaces.

You could load up preset 5 and then play loop 4, or whatever you like. It's also fine to save a loop to a non-existent preset number; the preset will be created automatically to store the loop.

#### <small>-+-</small> The Source of the original source..

Some nice code in here; original generator sketch (with triangle wave) from a German store blog:

> https://www.az-delivery.de/blogs/azdelivery-blog-fur-arduino-und-raspberry-pi/funktionsgenerator-mit-dem-esp32-teil1

Which looks to have sinusoidal refinements courtesy of Helmut Weber's Sine generator..

> https://www.esp32.com/viewtopic.php?t=10321#

which is in turn based on the wonderful dac-cosine example:

> https://github.com/krzychb/dac-cosine

#### <small>+++</small> `/console` is King!

When you use the proper web console page (at `/console`), you "pick up" the last message automatically, and this response is niftily displayed below the input, like this..

![Signal Generator's fully-featured command console once again](https://corz.org/img/ESP32/Signal-Generator-Command-Console-with-command-output-result.webp "/console enables you to send all your serial commands over WiFi.
All you need is something that can do a web request, like WGET, cURL, or your regular web browser.")

These messages are created for _all_ `/URL` commands (which is what the web console uses) and most regular commands, too; and they persist until they are picked up (immediately on pickup, messages are deleted), or another command replaces them with its _own_ message.

However, direct command methods (e.g. `cURL`, _WGET_, or a tab in your web browser) do NOT "pick up" their messages, so you can still access them over the web console, by simply hitting enter or clicking "Send", right up until another command is received by your ESP32 device. This can be useful or annoying, depending on how you look at it. Hit enter/send again to get the signal details, as usual.

You can also pick up the last message; using _any_ URL-capable mechanism; at `/LastMessage`, which is what the AJAX in the console page does.

#### <small>****</small> Channel Mixing..

This is actually a decent test setup, even for a dual+-channel scope. You can then disable channel 2 and get all the readings without a 2nd channel (and its readings) taking up space on your scope's screen (if applicable - my DSO can literally (and of course, optionally) FILL the screen with readings, completely obscuring the thing it's reading!).

There will be some extra noise and I wouldn't recommend this for serious work, but still; useful.

Of course you could also use channel two for reading the other end of a circuit!

#### Just for looks:  
<small>Signal Generator working in your film sets or Sci-Fi-looking workbench..</small>

If you have more than one ESP32 device kicking around and a spare 2+ channel scope, you can set it up for "ultra-hi-tech-looking" mode by running _two_ Signal Generators simultaneously; one into channel one, the other into channel two; then flipping that dormant oscilloscope of yours into X-Y mode. Boom! You are instant-Q.

I'm not going to say too much about the settings you might use here, except to say that if one Signal Generator is running at X Hz, you will want the other generator to be running at \*some\* multiple of that (1x is good, or 1.5 or 2 and so on) except \*just\* off (so XxX.0002 instead of XxX.0000). A slightly higher frequency will animate in one direction; slightly lower, the other.

The greater the difference between the two frequencies (or multiples thereof), the faster the animation will run. Signal Generator enables .000 decimal frequency settings, so you can create some superbly attention-grabbing displays, suitable for film or stage use, or just having your kids and friends think you are next-level.

If anyone actually \*asks\* what's going on, you can simply say you are monitoring the quantum flux variations, or some shit.

For accuracy and variation of patterns I recommend 2 x Triangle wave generators. When I'm not using my scope for anything else, I generally have two tabs in my browser doing this. I actually find these animations calming and you might, too. A certain amount of optical illusion is in play; three dimensions created from two. Perhaps it's the mental interpolation which produces the effect. Who knows?

For minimal timing drift I recommend 2 x UNO or 2 x DevKit ESP32 boards. WROVER will drift (though it seems to drift less when it's fallen flat on my workbench! - I may run some tests). See [above](https://corz.org/ESP32/square-sine-triangle-wave-signal-generator/#Module_Testing "test Test TEST!") for board details.

#### <small>+*+</small> Laser Control..

Tested with a 650nm 5mW Red pen-type laser. One of these..

![](https://corz.org/public/scripts/ESP32/pics/Power%20this%205mW%20650nM%20laser%20with%20your%20ESP32%20Signal%20Generator.jpg)

Around a quid on AliExpress.

#### <small>*!*</small> Portable Operation..

You could power your ESP32 + laser (or whatever) contraption from a well-charged 18650 battery shield - I love these things..

![Promotional image of three 18650 battery shields in all their glory. 1x, 2x and 4x 18650 variants are shown, single 18650 in the top-left, 2x 18650 in the middle-right, and the big-boy at the front-left, though extending beyond half-way in both directions. All their USB-A connexions point South-East. The background is white.](https://corz.org/public/scripts/ESP32/pics/18650-battery-shileds-ROAM-FREE.webp "18650 Battery Shields. Until 21700 Battery Shields become a thing, this is the thing.")

> Note: Round the _other_ side, is where the power _IN_ goes. The 2 x 18650 and 4 x 18650 units support Micro USB and USB-C inputs. The 1 x 18650 variant has _NO USB-C_. It is Micro USB only.
> 
> There may be other 1 x 18650 units kicking around with USB-C, but I haven't seen them yet.

#### <sup>~~~</sup> WiFi Wonkiness..

If your workspace is near a wall, I definitely recommend hammering a small panel pin (nail) into that wall, somewhere with a direct line-of-sight to your router, if possible, and hanging your module from that pin (or a more elegant solution!). The antenna on these things are tiny and "many a" wifi issue has been solved by simply moving the module to a location with interference (both physical and otherwise)-free radio communications. It also makes it easier to hit the reset button. However, as mentioned, my WROVER unit works great with NO antenna.

#### Easy-osie board definitions..

NOTE: While it is possible to use the generic "Dev Module" board setting (as is often advised), I see lots of slowness and undefined sh\*t happening when I do this.

When I use the correct board definition, everything works perfectly.

#### Note for Dervishes..

This was coded with Terry Riley's Descending Moonshine Dervishes playing in a loop, and while I'm sure this isn't required for positive results, it _definitely_ helps.

If you listen to this album long enough, you may just find yourself seeking out the song, "We gotta get out of this place", by the Animals; an excellent song; because its main melody is contained _within_ this album, except s-l-o-w-l-y. I have no idea if this was intentional.

#### About spaces..

I'm warming to them as a tabbing structure for C.

Also, you will notice when you run this, _all_ serial output is indented by a single space. Though there are many and groovy ways to do serial comms in Linux, I invariably find myself opening up the console in Arduino IDE and all that text slammed hard against the side of the window gives me the Heebie-Jeebies.

I use hard-left only for debug output so it's annoying, the way it should be.

#### Final Words..

After all this, when troubleshooting, I have come to understand that the level of complexity in these devices is such that sometimes \*shit happens\*, and I need to pause.

For example: I messed up the breadboard power wiring on my shiny new WROVER unit and even once I'd wired it the correct way it simply failed to connect over USB; wouldn't register itself at all; I even rebooted my laptop in desperation. Shit, I even tried it on Windows! Disconnected the thing a good few times and still; nada: Sulking. Bloody thing.

So I paused. Went off. Disconnected myself for a few minutes, came back, gave the chip a good talking-to, then reconnected; Boom! Everything works again.

LX7 will be worse; you mark my words\*.

\* But still, if a chip with Dual LX7 + 12 bit DACs appears; well worth having! Or ARM, even better!
