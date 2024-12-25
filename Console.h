/*
  A part of ESP32 Signal Generator

  https://corz.org/public/scripts/ESP32/SignalGenerator/

  This is a simple web page that enables you to send raw commands directly to Signal Generator with
  your web browser, just like you would over a serial connexion, except without being physically
  plugged in. Signal Generator's responses appear dynamically (with AJAX) below the inputs.

  Because all the action happens with AJAX; if you get to here from the main page; you can always
  get /back/ to the main page with a *single* click / swipe / whatever.

  NOTE: There is also a console interface on the main page - click the title.

*/
String WebConsole = R"HTML5(<!DOCTYPE html>
<html>
<head>
  <title>Web Console for ESP32 Signal Generator</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="shortcut icon" type="image/png" sizes="16x16" href="data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAQAAAC1+jfqAAAABGdBTUEAALGPC/xhBQAAAAlwSFlzAAAuIwAALiMBeKU/dgAAAAd0SU1FB+cBFRcAJyFRDxkAAAAZdEVYdENvbW1lbnQAQ3JlYXRlZCB3aXRoIEdJTVBXgQ4XAAAA3ElEQVQoz4WRP0tCcRSGH34u0ReIxoKQwBovTQ1iGPgxwqFdcg/nvoHQ3DdoqqFJh8ylMXFsMCSoIa+Pg97rn3vDZznDeTnnvOfFsgP/Y2CZpB07yZUwr13PLfmYowgAQy7Y55QqPTKo3nrsyJFFr5xuTEA/DbZVbUvm4gB9pkQAnAHvGxsCdNnhAIBD9nhLW01uAGJrXqcDG0YLu2Pxbn7DKg/ih6o98TmxueQEeAWgQ6CU2Fzy56UVf/yyaCuxuc6T2LDu7sJwRhB7b8EjX9JH5WT57e9KWFvingFTFG1S956a3gAAAABJRU5ErkJggg=="/>
<style>
  html{
    width: 100% !important;
    height: 50% !important;
    font-family: Tahoma;
    display: inline-block;
    color: #38ab45;
    margin: 0.5em 0 0 0;
  }
  div,pre,p,form {
    margin: 0;
    padding: 0;
  }
  .controller {
    margin: auto;
    width: 96%;
    height: 100%;
  }
  label {
    display:none;
  }
  .send-button, .nice-input {
    background-color: #38ab45;
    border-radius: 0.15em;
    border: none;
    color: white;
    padding: 0.1em 0.2em;
    font-size: 2.5em;
    font-weight: bold;
  }
  .send-button {
    cursor: pointer;
    width: 25%;
    float: right;
  }
  .send-button:hover {
    background-color: #00778f;
  }
  .send-button:active {
    background-color: #fff;
    color: #8cacb3;
  }
  .nice-input {
    width: 70%;
  }
  .nice-input:hover {
    background-color: #00778f;
  }
  .nice-input:active {
    background-color: #fff;
  }
  #InfoDIV {
    position: relative;
    width: 100%;
    bottom: 1em;
    top: 1.5em;
    clear: both;
    padding: 0.5em 0.5em 1em;
    overflow: scroll;
	font-family: Consolas, ProFontWindows,  'Lucida Console', 'Courier New', monospace;
    font-size: 90%;
	color: #38ab45;
  }
  #InfoDIV::-webkit-scrollbar {
    display: none;
  }

  @media screen and (min-width: 1000px) and (max-width: 2048px) {
    html {
      margin-top: 1.5em;
    }
    .controller {
      width: 75%;
      margin: 0 auto auto auto;
    }
    #InfoDIV {
      font-size: 100%;
    }
  }
</style>
</head>
<body>
<form onsubmit="return false;">
<div class="controller">

    <label for="webConsole">console input</label>

    <input id="webConsole" class="nice-input" onfocus="if (this.value=='Command') this.value = ''" onblur="if (this.value=='') this.value = 'Command'" type="text" value="Command" title="Enter commands just as you would in a serial Console.
When a response has been received, this input will reset.">

    <button onclick="sendCommand()" class="send-button" type="submit" title="Click here to send the command.
  (or hit &lt;enter&gt; from the input)">Send</button>
  <pre id="InfoDIV" title="Output goes here."><!--{insert}--></pre>
</div>
</form>

<script>

// When you *do* stuff, the "OK" response stays for a moment, then the main response is loaded..
// This defines exactly how long that "moment" is. Default is 250ms
//
// While we're here, if it wasn't clear, the input doesn't *clear* until a response is received.
// So you have a couple of levels of debugging without printing out a byte.
//
var updateDelay = 250;

// On page load, drop the cursor directly into the command input for instant typing..
window.onload = (event) => {
  document.querySelector("#webConsole").focus();
};

// Send the command..
function sendCommand() {
  var AJAX = new XMLHttpRequest();
  AJAX.onreadystatechange = function() {
    if(this.readyState == 4 && this.status == 200) {
      if (this.responseText != "") {
        // Everything after the line-break is for direct requests only.
        postMSG(this.responseText.split('\n')[0]);
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

// Pick up the Last Message..
function getCommandStatus() {
  var AJAX = new XMLHttpRequest();
  AJAX.onreadystatechange = function() {
    if(this.readyState == 4 && this.status == 200) {
      if (this.responseText) {
        postMSG(this.responseText);
      }
    }
  };
  AJAX.open("GET", "LastMessage", true);
  AJAX.send();
}

// Post information to the Info DIV..
function postMSG(message) {
  document.getElementById("InfoDIV").innerHTML = message;
}

</script>
</body>
</html>

)HTML5";
