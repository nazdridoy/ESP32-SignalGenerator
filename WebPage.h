/*
    Main ESP32 Signal Generator web page (wiki:HEREDOC)

    https://corz.org/public/scripts/ESP32/SignalGenerator/

    NOTE: You can use "number" types for the numeric inputs, which may be more convenient when using
    a phone/tablet to configure your generator, but you would loose the cool text-in-input titles,
    as well as the ability to enter "2k" for frequency/step (you could maybe leave those as "text"
    for this reason). It's just HTML (and ECMAScript, aka 'JavaScript'). Do as you will.

    "Everything" is AJAX. If you aren't familiar with AJAX, there is a decent explanation inside the
    main sketch.

    NOTE: We are using simple /url or /url?GET=parameters to set values, so you could conceivably
          use cURL/wget/whatever to set values; i.e. a shell script, or something calling one.**

    ** There is now a direct URL command interface, e.g. /p50. See the main sketch for details.

    MCU purists may prefer const char *WebPage = ...

    But you would lose that lovely String.replace(). ESP32 has gobs of RAM, and robust heap
    management - enjoy the good life!

    It's good to get into the habit of NOT putting a newline after the opening HEREDOC statement.
    Formats like SVG will fail dramatically if you do this.

    NOTE: The UP/DOWN control here is for frequency, but it would be a doddle to switch it to
    something else, e.g. Pulse Width. All the handler code is already written; you simply need to
    switch out the links on this page; inside setUP() and setDOWN(), below.

*/
String WebPage = R"HTML5(<!DOCTYPE html>
<html>
<head>
  <title>ESP32 Signal Generator</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <!--  PNG favicon image, encoded as base64 -> https://elmah.io/tools/base64-image-encoder/ -->
  <link rel="shortcut icon" type="image/png" sizes="16x16" href="data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAQAAAC1+jfqAAAABGdBTUEAALGPC/xhBQAAAAlwSFlzAAAuIwAALiMBeKU/dgAAAAd0SU1FB+cBFRcAJyFRDxkAAAAZdEVYdENvbW1lbnQAQ3JlYXRlZCB3aXRoIEdJTVBXgQ4XAAAA3ElEQVQoz4WRP0tCcRSGH34u0ReIxoKQwBovTQ1iGPgxwqFdcg/nvoHQ3DdoqqFJh8ylMXFsMCSoIa+Pg97rn3vDZznDeTnnvOfFsgP/Y2CZpB07yZUwr13PLfmYowgAQy7Y55QqPTKo3nrsyJFFr5xuTEA/DbZVbUvm4gB9pkQAnAHvGxsCdNnhAIBD9nhLW01uAGJrXqcDG0YLu2Pxbn7DKg/ih6o98TmxueQEeAWgQ6CU2Fzy56UVf/yyaCuxuc6T2LDu7sJwRhB7b8EjX9JH5WT57e9KWFvingFTFG1S956a3gAAAABJRU5ErkJggg==" />
<style>
  /* once you open a style tag, CSS comment rules apply */
  html {
    width: 100% !important;
    height: 100% !important;
    font-family: Tahoma;
    display: inline-block;
    color: #38ab45;
    margin: 0;
  }
  div, pre {
    margin: 0;
  }
  p, form {
    margin: 0;
  }
  button, .nice-input, #head, .preset-button {
    border-radius: 0.15em;
  }
  .controller {
    margin: 1.75em auto auto auto;
    width: 96%;
  }
  label {
    display: none;
  }
  .right {
    text-align: right;
    padding-right: 0.25em;
    float: right;
  }
  #version {
    font-size: 80%;
  }
  #head {
    background-color: #38ab45;
    padding: 0.25em 0 0.25em 0.25em;
    color: white;
    font-size: 2em;
    font-weight: bold;
    margin-top: 1.5em;
    z-index: 100;
  }
  #frequencyVAL, #WaveModeVAL, #DOWN, #UP, #status, #send-button,
   #simple, #power, #webconsoleButt, #waves, #waver, #wavet, .set-button, .preset-button {
    cursor: pointer;
  }
  .button, .step-button, .nice-input, .set-button, #status, .preset-button, #send-button  {
    background-color: #38ab45;
    border: none;
    color: white;
    padding: 0.1em 0.2em;
    font-size: 2.5em;
    font-weight: bold;
  }
  .button, .step-button, .nice-input, .set-button, #status, #send-button {
    margin: 0.175em auto
  }
  .button:hover, .step-button:hover, .set-button:hover, #status:hover, .preset-button:hover {
    background-color: #00778f;
  }
  .button:active, .step-button:active, .set-button:active, #status:active, .preset-button:active {
    background-color: #fff;
    color: #8cacb3;
  }
  .nice-input:hover {
    background-color: #00778f;
  }
  .nice-input:active {
    background-color: #fff;
  }
  .button {
    width: 100%;
  }
  .step-button {
    width: 40%;
    text-align: center;
  }
  #UP {
    float: right;
  }
  #status {
    width: 16%;
    margin-left: 1.2%;
  }
  .nice-input {
    width: 74%;
  }
  .set-button, #send-button {
    width: 20%;
    float: right;
    padding: 0.1em 0;
  }
  #live-values {
    column-count: 2;
    font-size: 1.5em;
    font-weight: bold;
    margin: 0.15em 0;
    padding: 0 0.05em;
  }
  #frequencyVAL {
    padding-left: 0.15em;
  }
  #waveforms {
    padding-right: 0.15em;
    column-count: 3;
  }
  #waves, #waver, #wavet {
    height: 1.7em;
    font-size: 400%;
  }
  #simple, #power, #webconsoleButt {
	opacity: 0.25;
    z-index: 100;
  }
  #simple {
    position: fixed;
    top: 5px;
    left: 5px;
    border-radius: 4px;
  }
  #power {
    position: fixed;
    top: 5px;
    right: 5px;
    border-radius: 16px;
  }
  #webconsoleButt {
    display: none;
  }
  #simple:active, #power:active, #webconsoleButt:active {
    background-color: #38ab45;
  }
  #power:hover, #simple:hover, #webconsoleButt:hover {
	opacity: 1;
  }
  #credit, #credit a {
    position: fixed;
    font-size: 0.8em;
    color: #38ab45;
    bottom: 0.5em;
    right: 0.25em;
  }
  #InfoDIV {
	font-family: Consolas, ProFontWindows,  'Lucida Console', 'Courier New', monospace;
    font-size: 80%;
	color: #38ab45;
    position: fixed;
    padding: 0;
    left: 9%;
    top: 0.5em;
    width: 85%;
    height: 5em;
/*    background-color: red;*/
    column-count: 3; /* Thank God for CSS3 */
    z-index: -1;
  }
  #presets {
    column-count: 9;
    margin: 0.2em auto;
  }
  .preset-button {
    text-align: center;
  }
  #webConsole, #send-button {
    margin: 0;
    font-size: 90%;
    background-color: white;
    color: #38ab45;
    line-height: 1.4em; /* omfg! someone please fix this CSS! lol */
  }
  #webConsole {
  }
  #send-button {
    margin-right: 0.2em;
  }
  #send-button:hover {
    background-color: #38ab45;
    color: white;
  }
  #send-button:active {
    background-color: white;
    color: #38ab45;
  }
  /*
  If you add this stuff, your CSS is "Responsive", like magic..
  Crucially, we define the stuff for *any* browser *first*, _then_ /add/ capabilities.
  Or just move stuff about, like this..
                                                                */
  @media screen and (min-width: 1000px) and (max-width: 2048px) {
    .controller {
      width: 50%;
      margin: 1em auto auto auto;
    }
    #head {
      margin-top: 0;
    }
    #simple {
      left: 5px;
    }
    #power {
      right: 0.24em;
    }
    #simple, #power, #webconsoleButt {
      opacity: 0.5;
    }
    #InfoDIV {
      column-count: 1;
      height: auto;
/*      width: auto;*/
      max-width: 25%;
      padding: 0 1em 1em 0.2em;
      left: 3em;
      top: 5px;
      font-size: 95%;
    }
    .nice-input {
      width: 75%;
     }
    #webconsoleButt {
      position: fixed;
      top: 42px;
      left: 5px;
      border-radius: 4px;
      display: inline-block;
    }
  }
</style>
</head>
<body>
  <div class="controller"><!-- You can switch out {tokens} for live data at page-load -->
    <div id="head" onclick="createWebConsole()" title="Click here to activate the Web Console"><span id="header">ESP32 Signal Generator</span>&nbsp;<span id="version">v{Version}</span>
    </div>
    <div id="live-values">
      <p id="frequencyVAL" onclick="getFrequency()" title="Click here to update with the current frequency.">Frequency: <span id="FreqValue"></span></p>
      <p id="WaveModeVAL" onclick="getWaveForm()" class="right" title="Click here to update with the current waveform.">Wave: <span id="WaveForm"></span></p>
    </div>
    <div>
      <label for="DOWN">DOWN</label>
      <button id="DOWN" onclick="setDOWN()" class="step-button" title="Move frequency DOWN one step.">DOWN</button>
      <label for="status">info</label>
      <button id="status" onclick="getStatus()" title="get current status info
(which may have been altered from elsewhere)">?</button>
      <label for="UP">UP</label>
      <button id="UP" onclick="setUP()" class="step-button" title="Move frequency UP one step.">UP</button>
    </div>
    <div>
      <!-- we create actual form elements so <enter> can set values -->
      <form onsubmit="return false;">
        <label for="frequency">Frequency</label>
        <input id="frequency" class="nice-input" name="frequency" onfocus="if (this.value=='Frequency') this.value = ''" onblur="if (this.value=='') this.value = 'Frequency'" type="text" value="Frequency" title="Input your desired frequency here, in Hz, kHz(k), or MHz(m). e.g. 2.34m">
        <button onclick="setFreqency()" class="set-button" type="submit" title="Click here to set the frequency.
(or hit &lt;enter&gt; from the input)">set</button>
      </form>
    </div>
    <div>
      <form onsubmit="return false;">
        <label for="step">Step Size</label>
        <input id="step" class="nice-input" name="step" onfocus="if (this.value=='Step Size') this.value = ''" onblur="if (this.value=='') this.value = 'Step Size'" type="text" value="Step Size" title="Enter the value of your desired step size, in Hz, kHz(k), or MHz(m). e.g. 1k">
        <button onclick="setStep()" class="set-button" title="Click here to set the frequency step size.
(or hit &lt;enter&gt; from the input)">set</button>
      </form>
    </div>
    <div>
      <form onsubmit="return false;">
        <label for="pulse">Pulse Width (%)</label>
        <input id="pulse" class="nice-input" name="pulse" onfocus="if (this.value=='Pulse Width (%)') this.value = ''" onblur="if (this.value=='') this.value = 'Pulse Width (%)'" type="text" value="Pulse Width (%)" title="enter the value for the pulse width percent (0-100).
note: this also sets sawtooth angle in the triangle generator">
        <button onclick="setPulseWidth()" class="set-button" title="Click here to set the pulse width.
(or hit &lt;enter&gt; from the input)">set</button>
      </form>
    </div>
    <div>
      <form onsubmit="return false;">
        <label for="bits">Resolution (1-10)</label>
        <input id="bits" class="nice-input" name="bits" onfocus="if (this.value=='Resolution (1-10)') this.value = ''" onblur="if (this.value=='') this.value = 'Resolution (1-10)'" type="text" value="Resolution (1-10)" title="enter your desired square wave resolution bit depth (1-10)">
        <button onclick="setBitDepth()" class="set-button" title="click here to set the resolution bit depth
(or hit &lt;enter&gt; from the input)">set</button>
      </form>
    </div>
    <div id="waveforms">
      <div>
        <label for="waver">Square Wave</label>
        <button id="waver" onclick="setSquare()" class="button" title="Select Square/Rectangle Wave.">&SquareIntersection;</button><!--&EmptySmallSquare;-->
      </div>
      <div>
        <label for="waves">Sine Wave</label>
        <button id="waves" onclick="setSine()" class="button" title="Select Sine Wave.">&#8767;</button>
      </div>
      <div>
        <label for="wavet">Triangle Wave</label>
        <button id="wavet" onclick="setTriangle()" class="button" title="Select Triangle/Sawtooth wave.">&bigtriangleup;</button><!-- &utri; -->
      </div>
    </div>
    <div id="presets" title="Load/Save presets..
(Ctrl+Click to save a new preset with the current settings.)">
      <div id="preset_1" title="{1}" onclick="loadPreset(1)" class="preset-button">1</div>
      <div id="preset_2" title="{2}" onclick="loadPreset(2)" class="preset-button">2</div>
      <div id="preset_3" title="{3}" onclick="loadPreset(3)" class="preset-button">3</div>
      <div id="preset_4" title="{4}" onclick="loadPreset(4)" class="preset-button">4</div>
      <div id="preset_5" title="{5}" onclick="loadPreset(5)" class="preset-button">5</div>
      <div id="preset_6" title="{6}" onclick="loadPreset(6)" class="preset-button">6</div>
      <div id="preset_7" title="{7}" onclick="loadPreset(7)" class="preset-button">7</div>
      <div id="preset_8" title="{8}" onclick="loadPreset(8)" class="preset-button">8</div>
      <div id="preset_9" title="{9}" onclick="loadPreset(9)" class="preset-button">9</div>
    </div>
  </div>
  <div id="InfoDIV" title="Information appears here.
Click the '?' button to put the current signal settings here."></div>
  <img alt="reboot button" id="power" onclick="rebootDevice()" title="Reboot your ESP32 device." alt="reboot link" src="data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAACAAAAAgCAMAAABEpIrGAAAACXBIWXMAAA7EAAAOxAGVKw4bAAAAB3RJTUUH5wEUBQsAwLFzwgAAAS9QTFRFAAAAnL6djYuN2tnatLW0WKhbcZlyfreAh5mHX7BiYLFiYLJjQ5NGhJ2FU6NVWo9chbeGfrmAZJtmX7Bifrh/TahRT6lTh7yIiLyJS61PTK1QfLZ+S65PZbJoZahnh7yIT6ZSUK9UTaNQTqdRbrRxUa1VjL2OTaVQkr+Tkr+Uk7+Uk8CUVahYWrBdUq9WUqpVT61TV6daW69eUqpVX7FiZrJoUq5VTapRTqpSTK9QZLNnabJrVKlXTqpSTapRSqpOSqdOX65hWbFdWLFbU61XUqxVV61aWKdaWKdbWq9dS65PWrJdV7FaTq1SWLFbTK9QTa9RTa1RTK5QVK5XUq5VUbBVY7BmZLJnX7BhUK5UVK1XTK1QTq1RUrBVTK9QZLJnTK9QT61TTK9QTa1RTa9RV8m/+AAAAGJ0Uk5TAAEFBggKCgoKCwsLFBQWGx0eHyInKioqKjAwMTw9UVJTU1dXWlxcZmdnZ2dqbHN3fX+DhoaNjpCRmZmZm56ipKqrrLW4u7u8vL/BxcbJy8zN1trf4+bn7/D09PX4+/z8/v6FBQ+kAAABZ0lEQVQYGZXBh1YTUQBF0UtvgjSx0JUyZFBAFMVC7y2EpnQ4+P/f4H2TGDMRXIu99UivT07e6D9aL+GyVQ/rx/r1sAiLdK9XPRVShEVSRc9LpTUswF6dxrAx1e3BQoNKNG5hg4qxWIPYVqP+WiMYUAbLaIBgTUUTBJtSjMXSJsGEClrOsdlqKcIiqXoWO29R3mdsRZbBMrIV7JMSVWfA6RNZhEWyp6fAWZWCPmxGQYzFCmawPgUfsW4FI9iIgm7sg4Il4FqJTqxTiWtgScE2kFXe1NXVlPKywLaCHSCrgspKFWSBHQXLwA/94yewrOAL1qYybdhXBUPYW5V5hw0paL4AjuqVUn8EXDQrMYdNK2Uam1Peszvg16hKjGJ3XSr4jt1Mquj9DfZNf9QcEuSGa2W1wzmCwxoVdRyTuN3dWN+9JXHcoRLPDyhz8EIpTYukLDapXO8qRau9uk/7+Px+Lrc/P96uR/gNGtKGbTbWpfEAAAAASUVORK5CYII=" />
<!-- The code for this simple page is within SignalGenerator.ino >> sendSimplePage()
 A small child could alter this quick-button to point instead to /pwm or /bits
       right here ===> V        -->
  <a id="simple" href="/simple" title="A simple two-button frequency controller.
Be sure to first set your preferred step size."><img alt="link to simple 2-button interface" src="data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAACAAAAAgCAMAAABEpIrGAAAACXBIWXMAAAB4AAAAeAHq8mr2AAAAGXRFWHRTb2Z0d2FyZQB3d3cuaW5rc2NhcGUub3Jnm+48GgAAASZQTFRFAAAAAP8AgICAVapVQL9AM5lmVapVQJ9gVapVTbNNRrlGVapVTrFOSbZJUK9QR6pVUa5RTbNNSapVUa5RSqpVR61STrFOTLNMSa1ST7BPSq1SSa9QTrFOTKxTSq5RTaxTTK5RTq1STK9QS7FOTa9QTLBPTa9QTa9QTLBPS65RTa5RTK9QTK5RTbBPTK5RTK9QS7BPTa5RTK9QTK9QS7BPTK9QTa5RTLBPTK9QTa9QTK9QTK5RTK9QTbBQTK5RTa9QTK5RTK9QTK9QTK9QTK9QTK9QTK9QS7BPTa9QTK9QTK5RTK9QTK9QTK9QTK9QTK9QTK9QTK9QTK9QTK9QTK9QTK9QTK9QTa9QTK9QTK9QTK9QTK9QTLBPTK9QTa9QTK9QTK9QTK9QJtpfvAAAAGF0Uk5TAAECAwQFBggJCgsMDQ4QEhMUFRYYGRobHB0fIyQlJigvO0BBSVFTVmFibm9ydHV2d3h8f4GGiIqNkp2hqKqrrbG1uLu9v8PEyMzR1tjZ3eHj6Onq6+3v8PLz9Pb3+Pr7/bA1FIoAAADzSURBVDjLjdNnM0NBFIfxJ1wu0SJK9N7rFb2sFiRBRJAoQfy//5fwmslZnrfnN3N2d2bh76aOM0ZuEqDlTWaVJmBQnrp/g7JzVS9Yhl0fKIbJ1sSzByywH5G2QSEY+qh0xUsmmONQ2mbJAvnYeE2qpprvDHASXUnSWZSxr1nvoQZ8IAEERXteiAGk1neMVnsBGE5bYKMPIHywV9wAjPgOGQdGfaDtBziKcpJ0Gl0aIMvEl/TaE94bQDM4aZM1a4WuG8ZqTx3tjybQPAcrbMkGt0F/mHzxAC3Cnnyg5Ny7F9R7qM5Pe15uBJi9yBudT//j738DuS0dcsRyMoAAAAAASUVORK5CYII=" /></a>
  <a id="webconsoleButt" href="/console" title="A proper console, but wireless.
(note: you can also click the main title to activate the built-in web console)"><img alt="console link" src="data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAACAAAAAgCAMAAABEpIrGAAABbmlDQ1BpY2MAACiRdZG9S0JhFIcftSjKcKghKsLBokEhCqK2ssFFQswgq0WvX4Efl3uVkNagpUFoiFr6GvoPag1aC4KgCCLa2vtaQm7negUl9Fzeex5+7/kdzj0X7MGsktM75iGXL2rhgN+9Gl1zd71jYxQncwzHFF1dCIWCtI2fR6mWePCZvdrXtYzeRFJXwNYtPKOoWlFYpiG4VVRN3hMeUDKxhPCJsFeTAYVvTT1u8ZvJaYu/TNYi4UWwmz3d6SaON7GS0XLCE8KeXLak1Ocxv8SZzK8sSx6SM4JOmAB+3MQpsUmWIj7JedlZa99kzbdEQTyKvFXKaOJIkxGvV9SSdE1KTomelCdL2dz7/33qqekpq7vTD52vhvE5Bl37UK0Yxu+pYVTPwPEC1/mGvyB7mv0WvdLQPMfg2oHLm4YWP4CrXRh8VmNarCY55NhTKfi4gL4o9N9Dz7q1q/o9508Q2ZZfdAeHRzAu9a6NP0b2aCqm0YZgAAAACXBIWXMAAA7DAAAOwwHHb6hkAAABUFBMVEUAAAAA/wB/f3+AgIBVqlVAv0AzmWZVqlVJtklAn2BVqlVNs01GuUZOsU5JtklEqlVQr1BHqlVOsU5HrVJMs0xJrVJPsE9Jr1BOsU5KrlFNrFNLrlFNsk1KsE9MrlFNrVJKsE9NrVJMrlFLsE9Mr1BNrlFMr1BOrlFNr1BKrlFNr1BLrVJNrlFMsE9LsU5NrlFMr1BMsE9LrlFMrlFLr1BNr1BLrlFMrlFMrlFMr1BLr1BNrlFMr1BMr1BNrlFNr1BMsE9Mr1BMsE9MrlFLr1BMsE9NsFBMr1BMr1BNsE9Mr1BMrlFMr1BMr1BMsE9Nr1BMr1BMr1BMr1BMr1BMsE9Mr1BMr1BMr1BMr1BMr1BMrlFMsE9Mr1BMr1BMr1BMr1BMr1BMr1BMr1BMr1BMr1BMr1BMr1BMsE9Mr1BNr1BMr1BMrlBMr1BMr1BMr1BMr1AhhxTqAAAAb3RSTlMAAQICAwQFBgcICQoLDQ4PEBIXGRscHSMkJigpKy0vMjc4OT1AQkNFRkhJS0xNTk9QUVVbXGBiaHV8fX5/g4eMkZOXm5ykqqy2t7y+v8bHyMnN0tbX2dvg4eLk5OXo6uvs7u/y9PX29/j6+/z8/f7fjEH7AAABhklEQVQ4y3WTZ1/CMBDGrwoU6sItbkDcAxcq7r1wD7SK4ELUJPf933kpVWkJz6sm9/89N3oBANC6Z+dViuhgqXYfURSFTl12WMAa/2a2uIs4rZLAA0XowCXh9ghRvF7eH42vFIQCmCQgSM7vvQCJDywH4jZQGAAD5t6wrAobYOKqRzNg5rXM4xdgeNtHxNQLVgIYpvuJGM0hF2qAoRmlOgYzTg8L4DaRiRERNZG7AWGPEbMjYGjhNFYCGD5P6wZEchVSSOKmyW9AqiKAtxGfAeEnoeyC4hed1OmgiUwN4FGrh6b54pjEfwqOe43egLaQV/wLtOJbNbrfm2SCoapNzld9fj2wQVvDlQ5fi1rA17CLnNNickui1AHvgrqn+RBZqXgJwN+GIXTmjBeJ3xpE9uDeHbeIvy4Idm4KneR2FQEaxOd6bMihWOJRdiSBOmLx3FvtcQqSZIET8uGY5HjdCG5tSqBdfq3InKnkskNLO3lKcaxJwL+NKgkuTtpst5axuEpd8un+ANwROIoDnq4XAAAAAElFTkSuQmCC" /></a>
  <div id="credit"><a href="https://corz.org/" target="_blank">from corz.org</a></div>

<script>

// When you *do* stuff, the info DIVs update with the full wave information after this time (ms)..
var updateDelay = 777;

// When you send commands with the console interface, the info DIV updates with the response after
// this time, then again with the current status in 2 * this time.

// There is no way back. If you really must read the title again, refresh the page.
var goneConsole = false;


// you could call the functions directly right here, but I like this better..
window.onload = (event) => {
  getStatus();
};

// If you add buttons, increase this number to match
var buttonMax = 9;


/*
   Web Console
               */

// Click the title to activate the Web Console..
var WebConsolePage = "<form onsubmit=\"return false;\"><div><input id=\"webConsole\" class=\"nice-input\" onfocus=\"if (this.value=='Command') this.value = ''\" onblur=\"if (this.value=='') this.value = 'Command'\" type=\"text\" value=\"Command\" title=\"Enter commands just as you would in a serial console.\nWhen a response has been received, this input will reset.\"><button onclick=\"sendCommand()\" id=\"send-button\" type=\"submit\" title=\"Click here to send the command. (or hit &lt;enter&gt; from the input)\">Send</button></div></form>";

function createWebConsole() {
  if (goneConsole == false) {
    headerTXT = "" + document.querySelector("#head").value;
    document.querySelector("#head").innerHTML = WebConsolePage;
    goneConsole = true;
    window.setTimeout( function() {
      document.querySelector("#webConsole").focus();
    }, updateDelay);
  }
}

// Send the command..
function sendCommand() {
  var AJAX = new XMLHttpRequest();
  AJAX.onreadystatechange = function() {
    if(this.readyState == 4 && this.status == 200) {
      if (this.responseText != "") {
        postMSG(this.responseText);
      }
      document.querySelector("#webConsole").focus();
      document.querySelector("#webConsole").value = "";
      window.setTimeout( function() {
        getCommandStatus();
      }, updateDelay);
    }
  };
  var myCommand = document.querySelector("#webConsole").value;
  myCommand = myCommand.replaceAll('%', '%25');
  if (myCommand == "") { myCommand = "!"; }
  console.log("Sending Command: " + myCommand);
  AJAX.open("GET", myCommand, true);
  AJAX.send();
}


// Get the command status..
function getCommandStatus() {
  var AJAX = new XMLHttpRequest();
  AJAX.onreadystatechange = function() {
    if(this.readyState == 4 && this.status == 200) {
      if (this.responseText) {
        /* Rather than force the user to reload the page for updated button titles, we can check for
           rename commands and switch the button title right here in JavaScript, using browser CPU cycles.

           This only works if namePreset() outputs *exactly* THIS text: " ↓ ↓ ↓ ↓ ↓ ↓ ↓ ↓ ↓ ↓ ↓ ↓ ↓ ↓ ↓ ↓ ↓ " */

        if (this.responseText.indexOf("Set name of preset") != -1 ) { // Set name of preset [x] to "Name"
          var regex = /preset \[(.+)\] to \"([^"]+)\"/
          var tData = this.responseText.match(regex);
          if ( parseInt(tData[1]) <= buttonMax ) { // only apply to buttons 1-buttonMax (no need to check for <1)
            document.getElementById("preset_" + tData[1]).title = tData[2]; // Yeah, JavaScript is alright.
          }
        }
        // Similarly, we can clear the titles of wiped presets..
        if (this.responseText.indexOf("Clearing") != -1 ) {
          var regex = /Clearing preset\ (.+)/
          var tData = this.responseText.match(regex);
          if ( parseInt(tData[1]) <= buttonMax ) {
            document.getElementById("preset_" + tData[1]).title = "";
          }
        }

        postMSG(this.responseText);
        window.setTimeout( function() {
          delayedStatus();
          }, updateDelay * 2);
      }
    }
  };
  AJAX.open("GET", "LastMessage", true);
  AJAX.send();
}


/*
    Text Inputs
                  */

// Frequency Change..
function setFreqency() {
  var AJAX = new XMLHttpRequest();
  AJAX.onreadystatechange = function() {
    if(this.readyState == 4 && this.status == 200) {
      postMSG(this.responseText);
      delayedStatus();
      document.querySelector("#frequency").focus();
      document.querySelector("#frequency").value = "";
    }
  };
  var f = "frequency="+document.querySelector("#frequency").value;
  console.log("Sending frequency: " + f);
  AJAX.open("GET", "setFreqency?"+f, true);
  AJAX.send();
}

// Frequency Touch Step Change..
function setStep() {
  var AJAX = new XMLHttpRequest();
  AJAX.onreadystatechange = function() {
    if(this.readyState == 4 && this.status == 200) {
      postMSG(this.responseText);
      delayedStatus();
      document.querySelector("#step").focus();
      document.querySelector("#step").value = "";
    }
  };
  var f = "step="+document.querySelector("#step").value;
  console.log("sending step size: " + f);
  AJAX.open("GET", "setStep?"+f, true);
  AJAX.send();
}

// Resolution Bit Depth Change..
function setBitDepth() {
  var AJAX = new XMLHttpRequest();
  AJAX.onreadystatechange = function() {
    if(this.readyState == 4 && this.status == 200) {
      postMSG(this.responseText);
      delayedStatus();
      document.querySelector("#bits").focus();
      document.querySelector("#bits").value = "";
    }
  };
  var f = "bits="+document.querySelector("#bits").value;
  console.log("sending bit depth: " + f);
  AJAX.open("GET", "setBitDepth?"+f, true);
  AJAX.send();
}

// Pulse Width Change..
function setPulseWidth() {
  var AJAX = new XMLHttpRequest();
  AJAX.onreadystatechange = function() {
    if(this.readyState == 4 && this.status == 200) {
      postMSG(this.responseText);
      delayedStatus();
      document.querySelector("#pulse").focus();
      document.querySelector("#pulse").value = "";
    }
  };
  var f = "pulse="+document.querySelector("#pulse").value;
  console.log("sending pulse width: " + f);
  AJAX.open("GET", "setPulseWidth?"+f, true);
  AJAX.send();
}



// UP Button..
function setUP() {
  var AJAX = new XMLHttpRequest();
  AJAX.onreadystatechange = function() {
    if(this.readyState == 4 && this.status == 200) {
      postMSG(this.responseText);
      delayedStatus();
    }
  };
  AJAX.open("GET", "setUP", true); // use "pulseUP" for pulse width UP, use "resUP" for resolution bits UP
  AJAX.send();
}

// Down Button..
function setDOWN() {
  var AJAX = new XMLHttpRequest();
  AJAX.onreadystatechange = function() {
    if(this.readyState == 4 && this.status == 200) {
      postMSG(this.responseText);
      delayedStatus();
    }
  };
  AJAX.open("GET", "setDOWN", true); // use "pulseDOWN" for pulse width DOWN, "resDOWN" for resolution bits DOWN
  AJAX.send();
}


// Set Waveforms..
//
function setSquare() {
  var AJAX = new XMLHttpRequest();
  AJAX.onreadystatechange = function() {
    if(this.readyState == 4 && this.status == 200) {
      postMSG(this.responseText);
      delayedStatus();
    }
  };
  AJAX.open("GET", "setSquare", true);
  AJAX.send();
}

function setSine() {
  var AJAX = new XMLHttpRequest();
  AJAX.onreadystatechange = function() {
    if(this.readyState == 4 && this.status == 200) {
      postMSG(this.responseText);
      delayedStatus();
    }
  };
  AJAX.open("GET", "setSine", true);
  AJAX.send();
}

function setTriangle() {
  var AJAX = new XMLHttpRequest();
  AJAX.onreadystatechange = function() {
    if(this.readyState == 4 && this.status == 200) {
      postMSG(this.responseText);
      delayedStatus();
    }
  };
  AJAX.open("GET", "setTriangle", true);
  AJAX.send();
}


// Info (?) button..
function getStatus() {
  var AJAX = new XMLHttpRequest();
  AJAX.onreadystatechange = function() {
    if(this.readyState == 4 && this.status == 200) {
      var foo = this.responseText
      foo = foo.replaceAll("\t", "<div>");  // Aaargh! An old pirate trick!
      foo = foo.replaceAll("\n", "</div>"); // Re-usable status data (console+AJAX+direct web access)
      postMSG(foo);
      getFrequency();
      getWaveForm();
    }
  };
  AJAX.open("GET", "status", true);
  AJAX.send();
}

// Get current values from server..
function getFrequency() {
  var AJAX = new XMLHttpRequest();
  AJAX.onreadystatechange = function() {
    if(this.readyState == 4 && this.status == 200) {
      document.getElementById("FreqValue").innerHTML = this.responseText;
    }
  };
  AJAX.open("GET", "frequency", true);
  AJAX.send();
}
function getWaveForm() {
  var AJAX = new XMLHttpRequest();
  AJAX.onreadystatechange = function() {
    if(this.readyState == 4 && this.status == 200) {
      document.getElementById("WaveForm").innerHTML = this.responseText;
    }
  };
  AJAX.open("GET", "wave", true);
  AJAX.send();
}


// Reboot your ESP32 device.. (slightly smarter version)
function rebootDevice() {
  postMSG("Rebooting..<br>(auto-reload in 3s)");
  var AJAX = new XMLHttpRequest();
  AJAX.onreadystatechange = function() {
    if(this.readyState == 4 && this.status == 200) {
      window.setTimeout( function() {
        window.location.reload();
      }, 3000);
    }
  };
  AJAX.open("GET", "reboot", true);
  AJAX.send();
}


// Load/Save a Presets..
function loadPreset(presetNumber) {
  var type = "Loading";
  if (event.ctrlKey == true) { type = "Saving"; }
  var AJAX = new XMLHttpRequest();
  AJAX.onreadystatechange = function() {
    if(this.readyState == 4 && this.status == 200) {
      postMSG(type + " Preset:<br>" + this.responseText);
      delayedStatus();
    }
  };
  var p = "preset="+presetNumber;
  if (type == "Saving") {
    console.log("Saving " + presetNumber);
    AJAX.open("GET", "savePreset?"+p, true);
  } else {
    console.log("Loading " + presetNumber);
    AJAX.open("GET", "loadPreset?"+p, true);
  }
  AJAX.send();
}

function delayedStatus() {
    window.setTimeout( function() {
    getStatus();
  }, updateDelay);
}


// Post information to the Info DIV..
function postMSG(message) {
  document.getElementById("InfoDIV").innerHTML = message;
}

</script>
</body>
</html>
)HTML5";
