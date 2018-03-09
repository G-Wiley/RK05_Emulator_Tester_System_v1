// ESP Sprinkler Controller
// Note: Debug messages are available via html and/or mqtt.
// The MQTT Server ID must be set in the html WiFiManager configuration to send MQTT debug messages.
// If the MQTT Server ID in the html WiFiManager configuration screen is null then MQTT debug messages are not sent.
// To use html debugging then html_debug in the following statement must be defined
// If html_debug is commented out then html debugging is not available.

#define html_debug 1

#include <FS.h>                   //this needs to be first
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
//#include <ESP8266mDNS.h>
#include <ESP8266SSDP.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
//#include <WiFiUdp.h>
//#include <Time.h>
//#include <TimeLib.h>
#include <stdio.h>
#include <WiFiManager.h>
#include <ArduinoOTA.h>
// Include the correct display library. For a connection via I2C using Wire include
#include "SSD1306.h" // alias for `#include "SSD1306Wire.h"`
#include "Sprinklerfonts.h"
#include "WeatherStationFonts.h"
//----------------------------------------------------------------------------------------

char WUCurrentTemp[5] = "00";
char WUIcon[2] = ")";
char WUDate[25] = "Refresh App";
//WundergroundClient wunderground(IS_METRIC);


//----------------------------------------------------------------------------------------


boolean mqtt_enabled = false; // enables MQTT debugging functions.

//default configuration values, if there are different values in config.json, they are overwritten.
char mqtt_server[40] = "";
char mqtt_port[6] = "";
//the following two parameters are not case sensitive. A String toLowerCase function converts them to lowercase before use.
char cp_time_zone[40] = "pacific"; //valid values are: pacific, mountain, central, eastern
char cp_auto_dst[10] = "auto"; //valid values are: auto, on, off. auto and on will automatically adjust for DST. off is for no DST.
#define max_mqtt_limit 84
#define keepalive_interval (unsigned long) 8000
#define id_debugging "debugging"

const int id_stations[] = {10, 13, 12, 14, 2, 0, 4, 5}; // stations 1 - 8 GPIO, original D12 D7 D6 D5 D4 D3 D2 D1
//const int id_stations[] = {5, 4, 0, 2, 14, 12, 13, 15}; // stations 1 - 8 GPIO modified D1 D2 D3 D4 D5 D6 D7 D8
//NodeMCU  GPIO   GPIO
//Pin      Code#
//D0       16     GPIO16 (on board LED)
//D1       5      GPIO5
//D2       4      GPIO4
//D3       0      GPIO0
//D4       2      GPIO2
//D5       14     GPIO14
//D6       12     GPIO12
//D7       13     GPIO13
//D8       15     GPIO15
//RX/D9    3      GPIO3
//TX/D10   1      GPIO1
//SD2/D11  9      GPIO9
//SD3/D12  10     GPIO10

#define switchpin 15

// Initialize the OLED display using Wire library
// D9(RX) -> SDA
// D10(TX) -> SCL
//SSD1306  display(0x3c, D3, D5); // display(i2c_Address, SDA_pin, SCL_pin)
SSD1306  display(0x3c, D9, D10); // display(i2c_Address, SDA_pin, SCL_pin) UART_RX, UART_TX

#define no_stations 8
#define onboard_ledPin 16   // GPIO16 is D0, the on-board LED
int blink_counter = 0;
int no_blinks = 3;


int current_station = 0;
int runlist[16][2];
int runitem[2];
int runpointer = 0;
int runcount = 0;

// In Auto Mode:
//   timer_station_run.secondsremaining() is the zone time remaining in seconds
//   runlist[runpointer - 1][1] is the time of the zone in seconds
//   runpointer is the zone number
// In Manual Mode:
//   DisplayStateTimer.secondsremaining() is the zone time remaining in seconds
//   manualmodetime is the time of the zone in seconds
//   manualmodezone is the zone number

//hub definition
String hubAddress;
/*******************************************************Memory Leak Prevention***********************/
int onboard_led_state = 0;           // so we can toggle the LED on and off when Compool messages are received

/****************************************************************************************************/

/********** This section defines global static variables used by the Compool Bridging Function **********/
//
WiFiClient espClient;

typedef struct stations Stations;

// definitions for the mqtt and html debuggers
//

PubSubClient client(espClient);

#ifdef html_debug
String webSite, javaScript, XML;
String debugmsg = "";
#endif

// states of individual Compool parameters
// A UDP instance to let us send and receive packets over UDP
WiFiUDP udp;

//flag for saving data
bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback() {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

//Display state variables
int displaystate = 0;
int manualmodezone = 0;
int manualmodetime = 30;
boolean switchedge = false;
boolean autorun = false;
int switchvalue = 0;
int pswitchvalue = 0;


//-------------------------------------Timer Definitions Begin---------------------------------
// Timer Definitions
//
class EventTimer {
  private:
    boolean timer_enabled = false;
    unsigned long expiration_time = 0;
    unsigned long retrigger_time = 0;
    int minutesleft = 0;
  public:
    void starttimer(unsigned long, boolean);
    void stoptimer() {
      timer_enabled = false;
    }
    boolean processtimer();
    int minutes();
    int secondsremaining();
    boolean timerisrunning() {
      return (timer_enabled);
    }
};

// Start a timer event:
//   void starttimer(unsigned long timeout, boolean retrig)
//   timeout is the timer duration in milliseconds
//   retrig is TRUE to make the timer retriggerable so it is a periodic event, and FALSE for a one-time event
//
void EventTimer::starttimer(unsigned long timeout, boolean retrig) {
  expiration_time = millis() + timeout; // next expiration time is the present time plus the time parameter
  if (retrig)
    retrigger_time = timeout; // if retriggerable then the retrigger time is also the time parameter
  else
    retrigger_time = 0; // if not retriggerable then this is remembered by setting the retrigger time to zero
  timer_enabled = true; // enable the timer
}

// Stop a timer that is currently running:
//  void stoptimer()
//

// Evaluate a timer to determine whether it is expired
//   boolean processtimer()
//   return true if the timer has expired, or false if the timer has not yet expired
//
boolean EventTimer::processtimer() {
  if (timer_enabled) {
    if (millis() > expiration_time) { // if the time has expired
      if (retrigger_time == 0) { // if zero then not retriggerable
        timer_enabled = false; // disable the timer and return true
        return (true);
      }
      else { // here if the time has expired and the timer is retriggerable
        expiration_time += retrigger_time; // next expiration time is increased by the retrigger time
        return (true); // and return true because the time expired
      }
    }
    else
      return (false);
  }
  return (false); // fall through to here and return false if the timer is not enabled
}

// Return the number of minutes remaining until expiration of the timer, or zero is the timer is not enabled
//  int minutes()
//
int EventTimer::minutes() {
  if (timer_enabled) {
    if (millis() < expiration_time) { // if the time has expired
      minutesleft = (int) (((expiration_time - millis()) / 60000));
    }
    else {
      minutesleft = 0;
    }
  } else {
    minutesleft = 0;
  }
  return minutesleft;
}

// Return the number of seconds remaining until expiration of the timer, or zero is the timer is not enabled
//  int secondsremaining()
//
int EventTimer::secondsremaining() {
  if (timer_enabled) {
    return ((expiration_time - millis()) / 1000);
  }
  else
    return (0);
}

// Determine whether a timer is currently running:
//  boolean timerisrunning()
//
//-------------------------------------Timer Definitions End---------------------------------

#define retriggerable true
#define not_retriggerable false
EventTimer timer_mqtt_keepalive, timer_intro_message, timer_station_run, timer_blinkInterval, timer_blink;
EventTimer DisplayStateTimer, DisplayUpdateTimer, SwitchDebounceTimer;

void blinkinternal(int times) {
  debugger ("blink " + String(times));

  //  for (int i = 0; i < times; i++) {
  toggle_onboard_led_state();
  //    delay(500);
  //    toggle_onboard_led_state();
  //  }
}
void handleBlinkInterval() {
  if (blink_counter != no_blinks * 2) {
    timer_blink.starttimer(200, not_retriggerable);
    blink_counter++;
  }

}


void onboard_led_on() {
  onboard_led_state = 1;
  digitalWrite(onboard_ledPin, LOW);   // Turn the onboard LED on by setting the voltage LOW
}

// Turn OFF the onboard LED
//
void onboard_led_off() {
  onboard_led_state = 0;
  digitalWrite(onboard_ledPin, HIGH);   // Turn the onboard LED off by setting the voltage HIGH
}

// Toggle the state of the onboard LED
//
void toggle_onboard_led_state()
{
  if (onboard_led_state == 0) {
    onboard_led_on();
    //    onboard_led_state = 1;

  }  else {
    onboard_led_off();
    //    onboard_led_state = 0;
  }
}
/********** This section defines the JSON objects and http parameters for SmartThings Control **********/
// This section was copied and pasted from Brian's code, with minor edits to link to the Compool Bridge code.
//
String jsonString;

ESP8266WebServer server(80);
//int debug_led_state = 1;
//#define debug_ledPin 14 //GPIO14 is D5, the debug LED used for http command testing
// the debug LED is now defined in the global static section above
// call debug_led_on() to turn on the LED, or call debug_led_off() to turn off the LED -gw

// This is where we will check the status of the compool and put the data into the json object
String buildJson() {
  //
  // Step 1: Reserve memory space
  //
  StaticJsonBuffer<1000> jsonBuffer;
  //
  // Step 2: Build object tree in memory
  //
  JsonObject& root = jsonBuffer.createObject();
  for (int i = 0; i < no_stations; i++) {
    JsonObject& station = root.createNestedObject(String(i + 1));
    station["mode"] = "off";
  }


  JsonObject& debugging = root.createNestedObject(id_debugging);
  debugging["freemem"] = 0;
  debugging["runseconds"] = 0;
  root.prettyPrintTo(jsonString);
  return jsonString;
}

#ifdef html_debug // html debugging code

void buildWebsite() {
  buildJavascript();
  webSite = "<!DOCTYPE HTML>\n";
  webSite += javaScript;
  webSite += "<BODY onload='process()'>\n";
  webSite += "<BR>This is the ESP website ...<BR>\n";
  webSite += "<BR><textarea id='Text1' cols='100' rows='25'></textarea><BR>";
  webSite += "<BR><label><input type = 'checkbox' id='checkbox'>Freeze</label><BR>";
  webSite += "Debug Messages...<BR><A id='runtime'></A>\n";
  webSite += "</BODY>\n";
  webSite += "</HTML>\n";
}

void buildJavascript() {
  javaScript = "<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.1.1/jquery.min.js'></script>";

  javaScript += "<SCRIPT>\n";
  javaScript += "var xmlHttp=createXmlHttpObject();\n";

  javaScript += "function createXmlHttpObject(){\n";
  javaScript += " if(window.XMLHttpRequest){\n";
  javaScript += "    xmlHttp=new XMLHttpRequest();\n";
  javaScript += " }else{\n";
  javaScript += "    xmlHttp=new ActiveXObject('Microsoft.XMLHTTP');\n";
  javaScript += " }\n";
  javaScript += " return xmlHttp;\n";
  javaScript += "}\n";

  javaScript += "function process(){\n";
  javaScript += " if(xmlHttp.readyState==0 || xmlHttp.readyState==4){\n";
  javaScript += "   xmlHttp.timeout = 5000;\n";
  //javaScript+="   xmlHttp.ontimout = 'process()'";
  javaScript += "   xmlHttp.open('PUT','xml',true);\n";
  javaScript += "   xmlHttp.onreadystatechange=handleServerResponse;\n"; // no brackets?????
  javaScript += "   xmlHttp.send(null);\n";
  javaScript += " }\n";
  javaScript += " setTimeout('process()',1000);\n";
  javaScript += "}\n";

  javaScript += "function handleServerResponse(){\n";
  javaScript += " if(xmlHttp.readyState==4 && xmlHttp.status==200){\n";
  javaScript += "   xmlResponse=xmlHttp.responseXML;\n";
  javaScript += "   xmldoc = xmlResponse.getElementsByTagName('response');\n";
  javaScript += "   message = xmldoc[0].firstChild.nodeValue || '';\n";
  javaScript += "   document.getElementById('runtime').innerHTML=message;\n";
  javaScript += "   if (!document.getElementById('checkbox').checked){\n";
  javaScript += "   $('#Text1').val($('#Text1').val() + message.trimLeft());\n";
  javaScript += "   var ta = document.getElementById('Text1')\n";
  javaScript += "   ta.scrollTop = ta.scrollHeight\n";
  javaScript += " }\n";
  javaScript += " }\n";
  javaScript += "}\n";
  javaScript += "</SCRIPT>\n";
}
void buildXML() {
  XML = "<?xml version='1.0'?>";
  XML += "<response>";
  XML += debugmsg;
  XML += "</response>";
}
void handleXML() {
  buildXML();
  server.send(200, "text/xml", XML);
  debugmsg = " ";
}
void handleWebsite() {
  debugmsg = "";
  buildWebsite();
  server.send(200, "text/html", webSite);
}

void htmlDebug(String dmsg) {
  String tempmsg = debugmsg;
  debugmsg = tempmsg + millis2time() + ": " + dmsg + "\n" ;
}

String millis2time() {
  String Time = "";
  unsigned long ss;
  byte mm, hh, dd;
  ss = millis() / 1000;
  dd = ss / 86400;
  hh = (ss - dd * 86400) / 3600;
  mm = (ss - dd * 86400 - hh * 3600) / 60;
  ss = (ss - dd * 86400 - hh * 3600) - mm * 60;
  Time += (String)dd + ":";
  if (hh < 10)Time += "0";
  Time += (String)hh + ":";
  if (mm < 10)Time += "0";
  Time += (String)mm + ":";
  if (ss < 10)Time += "0";
  Time += (String)ss;
  return Time;
}

#endif
//root.prettyPrintTo(jsonString);
//
/************************************************** End **************************************************/


/************************  This section contains the handlers for the JSON objects ************************/
//
void handleJson() { //check the json object and send it to the server
  server.send(200, "text/json", jsonString);
}

void handleRoot() {
  handlePoll();
}
void handleBlink() {
  debugger ("blinks set to " + server.arg("number"));
  no_blinks = server.arg("number").toInt();
  server.send(200, "text/json", jsonString);
}
void handleOn() {
  timer_station_run.stoptimer();
  StaticJsonBuffer<1000> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(jsonString);
  for (int i = 0; i < no_stations; i++) {
    if (root[String(i + 1)]["mode"] == "on") {
      turn_off_device(i + 1);
    }
    root[String(i + 1)]["mode"] = "off";
  }
  jsonString = "";
  root["debugging"]["freemem"] = ESP.getFreeHeap(); //doing this at the top in case any code below reduces the heapsize
  root["debugging"]["runseconds"] = millis2time();
  root.prettyPrintTo(jsonString);
  runcount = 0;
  memset(runlist, 0, sizeof(runlist));
  for (int i = 0; i < server.args(); i++) {
    debugger("handleOn arg = " + server.argName(i) + ", " + server.arg(i));
    if (server.argName(i).substring(0, 6) == "device") {
      runcount ++;
      int dnumb = server.argName(i).substring(6).toInt();
      debugger("device number " + String(dnumb) + " = " + server.arg(i));
      runlist[dnumb - 1][0] = server.arg(i).toInt();
    }
    if (server.argName(i).substring(0, 4) == "time") {
      int dnumb = server.argName(i).substring(4).toInt();
      debugger("time number " + String(dnumb) + " = " + server.arg(i));
      runlist[dnumb - 1][1] = server.arg(i).toInt();
    }
  }
  autorun = true;
  if (displaystate >= 2) {
    if (manualmodezone > 0)
      turn_off_device(manualmodezone);
    displaypaintOff();
  }
  displaystate = 1;

  runpointer = 1;
  turn_on_device();
  server.send(200, "text/json", jsonString);

}

void handleOff() {
  String device = server.arg("device");
  turn_off_device(device.toInt());
  debugger(device + " off");
  server.send(200, "text/json", jsonString);

  //  autorun = false;
  displaypaintOff();
  displaystate = 0;
}

void handleAlloff() {
  timer_station_run.stoptimer();
  //  for (int i = 1; i <= no_stations; i++) {
  turn_off_all();
  //   debugger(String(i) + " off");
  // }
  server.send(200, "text/json", jsonString);
  for (int i = 1; i <= no_stations - 1; i++) {
    runlist[i][0] = 0;
    runlist[i][1] = 0;
  }
  runpointer = 0;
  runcount = 0;

  autorun = false;
  displaypaintOff();
  displaystate = 0;
}

void handleSubscribe() {
  debugger("handleSubscribe");
  debugger(server.arg("address"));
  hubAddress = server.arg("address");
}

void handlePoll() {
  debugger("icon server arugment: " + server.arg("icon"));
  if (server.arg("icon") != "") {
    server.arg("icon").toCharArray(WUIcon, 2);
  }
  if (server.arg("temp") != "") {
    server.arg("temp").toCharArray(WUCurrentTemp, 5);
  }
  if (server.arg("date") != "") {
    server.arg("date").toCharArray(WUDate, 25);
  }
  displaypaintOff();
  debugger(WUIcon);
  debugger(WUCurrentTemp);
  debugger(server.arg("date"));
  debugger(String(ESP.getFreeSketchSpace()));
  StaticJsonBuffer<1000> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(jsonString);
  // logic for handlePoll is different because there is no resulting Packet sent to Compool
  // the server response can be prepared and immediately returned to SmartThings
  root["debugging"]["freemem"] = ESP.getFreeHeap(); //doing this at the top in case any code below reduces the heapsize
  root["debugging"]["runseconds"] = millis2time();
  jsonString = "";
  root.prettyPrintTo(jsonString);
  debugger("handlePoll");
  server.send(200, "text/json", jsonString);
}

void handleNotFound() {
  server.send(404, "text/html", "<html><body>Error! Page Not Found!</body></html>");
  debugger("Error! Page Not Found! in handleNotFound");
}

void handleReset() {
  //server.sendHeader("Location", String("/debug"), true);
  server.send (200, "text/plain", "Connect to Irrigation Controller via wifi to set up Sprinklers");
  debugger("what's going on?");
  WiFiManager wifiManager;
  wifiManager.resetSettings();
  ESP.reset();
}


//
/************************************************** End **************************************************/


/*******  This section contains functions to control the Compool Bridge that are called by JSON object handlers *******/

// turn ON the devices specified by the bitmask
// if the device is already on then don't send a command to the Compool
// if the device is off and needs to be turned on then build the Compool packet and queue
// the message so it can be sent to the Serial handler which happend in the main loop
//
boolean turn_on_device() {
  boolean retval;
  String queue = "Queue: ";
  //  debugger(String(device));
  //  debugger(String(device_time));
  StaticJsonBuffer<1000> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(jsonString);
  int count = runpointer - 1;
  while (runlist[count][0]) {
    queue += String(runlist[count][0]) + " ";
    digitalWrite(id_stations[runlist[count][0] - 1], HIGH);
    if (runlist[count][1] > 0) {
      root[String(runlist[count][0])]["mode"] = "q";
    } else {
      root[String(runlist[count][0])]["mode"] = "off";
    }
    count++;
  }
  debugger("turning on " + String(runlist[runpointer - 1][0]));
  debugger(queue);
  debugger("turn_on_device digitalWrite, GPIO = " + String(id_stations[runlist[runpointer - 1][0] - 1]));
  digitalWrite(id_stations[runlist[runpointer - 1][0] - 1], LOW);
  root[String(runlist[runpointer - 1][0])]["mode"] = "on";


  jsonString = "";
  root["debugging"]["freemem"] = ESP.getFreeHeap(); //doing this at the top in case any code below reduces the heapsize
  root["debugging"]["runseconds"] = millis2time();
  root.prettyPrintTo(jsonString);
  debugger(hubAddress);
  HTTPClient http;
  http.begin("http://" + hubAddress + "/notify");
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  http.POST(jsonString);
  //http.writeToStream(&Serial);
  http.end();


  timer_station_run.starttimer(runlist[runpointer - 1][1] * 1000, not_retriggerable);
  //  autorun = false;
  return (retval);
}

// turn OFF the devices specified by the bitmask
// if the device is already off then don't send a command to the Compool
// if the device is on and needs to be turned off then build the Compool packet and queue
// the message so it can be sent to the Serial handler which happend in the main loop
//

boolean turn_off_device(int device) {
  boolean retval;
  StaticJsonBuffer<1000> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(jsonString);
  if (root[String(device)]["mode"] == "q") {
    root[String(device)]["mode"] = "off";
    int count = runpointer - 1;
    int found = false;
    while (runlist[count][0]) {
      if (runlist[count][0] == device || found) {
        found = true;
      }
      if (found) {
        runlist[count][0] = runlist[count + 1][0];
        runlist[count][1] = runlist[count + 1][1];
      }
      count++;
    }
    runlist[count][0] = 0;
    runlist[count][1] = 0;

    runcount = runcount - 1;

  } else {
    digitalWrite(id_stations[device - 1], HIGH);
    root[String(device)]["mode"] = "off";
    autorun = false;
    if (runpointer < runcount) {
      timer_station_run.stoptimer();
      timer_station_run.starttimer(1, not_retriggerable);
    }
  }

  root["debugging"]["freemem"] = ESP.getFreeHeap(); //doing this at the top in case any code below reduces the heapsize
  root["debugging"]["runseconds"] = millis2time();
  jsonString = "";
  root.prettyPrintTo(jsonString);

  HTTPClient http;
  http.begin("http://" + hubAddress + "/notify");
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  http.POST(jsonString);
  //http.writeToStream(&Serial);
  http.end();
  return (retval);
}

boolean turn_off_all() {
  boolean retval;
  StaticJsonBuffer<1000> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(jsonString);
  for (int i = 1; i <= no_stations; i++) {
    digitalWrite(id_stations[i - 1], HIGH);
    root[String(i)]["mode"] = "off";
  }
  root["debugging"]["freemem"] = ESP.getFreeHeap(); //doing this at the top in case any code below reduces the heapsize
  root["debugging"]["runseconds"] = millis2time();
  jsonString = "";
  root.prettyPrintTo(jsonString);
  for (int i = 1; i <= no_stations - 1; i++) {
    runlist[i][0] = 0;
    runlist[i][1] = 0;
  }
  runpointer = 0;
  runcount = 0;

  HTTPClient http;
  http.begin("http://" + hubAddress + "/notify");
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  http.POST(jsonString);
  //http.writeToStream(&Serial);
  http.end();
  autorun = false;
  return (retval);
}


// Debugging function that outputs debug messages via mqtt and the html debug page
//
void debugger(String debugmessage) {
  char dmsg[max_mqtt_limit + 6];

  // only publish to mqtt if mqtt_enabled is true.

  String sdmsgtimed;
  if (mqtt_enabled) {
    sdmsgtimed = String(millis()) + "; " + debugmessage;
    sdmsgtimed.toCharArray(dmsg, max_mqtt_limit + 5);
    client.publish("debug/sprinkler", dmsg);
  }

#ifdef html_debug
  htmlDebug(debugmessage);
#endif
}
void configModeCallback (WiFiManager *myWiFiManager) {
  display_setup();
  //display.init();
  //display.flipScreenVertically();
  //display.setContrast(255);
  String softIp =  String(WiFi.softAPIP()[0]) + "." + String(WiFi.softAPIP()[1]) + "." + String(WiFi.softAPIP()[2]) + "." + String(WiFi.softAPIP()[3]);
  display.clear();
  display.setFont(ArialMT_Plain_16);
  display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
  display.drawString(DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2 - 10, "Config Mode\n");
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
  display.drawString(DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2 + 2 , "IP: " + softIp + "\nSSID: " + myWiFiManager->getConfigPortalSSID());
  display.display();
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
}

void setup_wifi() {
  IPAddress ip_ofESP;


  delay(10);
  // We start by connecting to a WiFi network

  populate_config_from_fs();
  WiFiManager wifiManager;
  wifiManager.setAPCallback(configModeCallback);
  // id/name, placeholder/prompt, default, length
  WiFiManagerParameter config_mqtt_server("mqttserver", "mqtt server", mqtt_server, 40);
  wifiManager.addParameter(&config_mqtt_server);
  WiFiManagerParameter config_mqtt_port("mqttport", "mqtt port", mqtt_port, 10);
  wifiManager.addParameter(&config_mqtt_port);
  WiFiManagerParameter config_time_zone("timezone", "time zone", cp_time_zone, 40);
  wifiManager.addParameter(&config_time_zone);
  WiFiManagerParameter config_auto_dst("autodst", "auto DST", cp_auto_dst, 40);
  wifiManager.addParameter(&config_auto_dst);

  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  //wifiManager.resetSettings(); //to reset for debugging purposes
  if (!wifiManager.autoConnect("Irrigation Controller")) {

    debugger("failed to connect and hit timeout");
    ESP.reset();
    delay(1000);
  }



  debugger("setting blinks to 1");
  no_blinks = 1;
  if (shouldSaveConfig) {
    Serial.println("Saving Config");
    strcpy(mqtt_server, (char *) config_mqtt_server.getValue());
    strcpy(mqtt_port, (char *) config_mqtt_port.getValue());
    strcpy(cp_time_zone, (char *) config_time_zone.getValue());
    strcpy(cp_auto_dst, (char *) config_auto_dst.getValue());
    SaveConfig();
  }
  else
    Serial.println("Not Saving Config");

  // Serial.println("\nWiFi connected\nIP address: ");
  ip_ofESP = WiFi.localIP();
  // Serial.println(ip_ofESP);
}

void populate_config_from_fs() {
  Serial.println("Populate config from FS");
  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/compoolconfig.json")) {
      //file exists, reading and loading
      //Serial.println("reading config file");
      File configFile = SPIFFS.open("/compoolconfig.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        Serial.println("configFile size = " + size);
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer cpconfig_jsonBuffer;
        JsonObject& cpconfig_json = cpconfig_jsonBuffer.parseObject(buf.get());
        cpconfig_json.printTo(Serial);
        if (cpconfig_json.success()) {
          Serial.println("\nparsed json");

          strcpy(mqtt_server, cpconfig_json["mqtt_server"]);
          strcpy(mqtt_port, cpconfig_json["mqtt_port"]);
          strcpy(cp_time_zone, cpconfig_json["cp_time_zone"]);
          strcpy(cp_auto_dst, cpconfig_json["cp_auto_dst"]);

        }
        else {
          Serial.println("failed to load json config");
        }
      }
    }
  } //else {
  //Serial.println("failed to mount FS");
  //}
  //end read

}

void SaveConfig() {
  Serial.println("SaveConfig() function");
  DynamicJsonBuffer cpconfig_jsonBuffer;
  JsonObject& cpconfig_json = cpconfig_jsonBuffer.createObject();
  cpconfig_json["mqtt_server"] = mqtt_server;
  cpconfig_json["mqtt_port"] = mqtt_port;
  cpconfig_json["cp_time_zone"] = cp_time_zone;
  cpconfig_json["cp_auto_dst"] = cp_auto_dst;
  Serial.println(mqtt_server);
  Serial.println(mqtt_port);
  Serial.println(cp_time_zone);
  Serial.println(cp_auto_dst);
  File configFile = SPIFFS.open("/compoolconfig.json", "w");
  if (!configFile)
    Serial.println("failed to open config file for writing");

  cpconfig_json.printTo(Serial);
  if (configFile) {
    cpconfig_json.printTo(configFile);
    configFile.close();
  }
  //end save
}

//-------------------------------------Display Functions Begin---------------------------------
//
void serialdebug(String debugstring) {
  //Serial.print(debugstring + "\n");
  debugger(debugstring);
}

int readswitch() {
  if (digitalRead(switchpin) == HIGH) return (1);
  else return (0);
}

void process_switch() {
  SwitchDebounceTimer.processtimer();
  switchvalue = readswitch();
  if (SwitchDebounceTimer.timerisrunning() != true) {
    if (pswitchvalue != switchvalue) {
      if (switchvalue == 1) {
        switchedge = true;
        serialdebug("Switch Edge");
        serialdebug("displaystate = " + String(displaystate) + "; manualmodezone = " + String(manualmodezone));
      }
      SwitchDebounceTimer.starttimer(100, false);
      if (switchvalue == 1) serialdebug("Switch down");
      else serialdebug("Switch up");
    }
  }
  pswitchvalue = switchvalue;
}

String sec2time(unsigned long ss) {
  String Time = "";
  byte mm;
  mm = ss / 60;
  ss = ss - mm * 60;
  Time += (String)mm + ":";
  if (ss < 10)Time += "0";
  Time += (String)ss;
  return Time;
}

void display_setup() {
  // Initialising the UI will init the display too.
  display.init();

//  display.flipScreenVertically();// flipped
  display.setFont(ArialMT_Plain_16);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  displaystate = 0;
  manualmodezone = 0;
}

void displaypaintOff() {
  display.clear();
  display.setFont(ArialMT_Plain_16);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  //display.drawXbm(5, 0, 50, 50, partly_cloudy_day_bits);
  display.setFont(Meteocons_Plain_42);
  //String weatherIcon = WUIcon;
  debugger("display icon: " + String(WUIcon));
  //int weatherIconWidth = display.getStringWidth(String(WUIcon));
  display.drawString(DISPLAY_WIDTH / 4, 0, String(WUIcon));
  display.setFont(Roboto_Medium_42);
  display.setTextAlignment(TEXT_ALIGN_RIGHT);
  display.drawString(DISPLAY_WIDTH + 2, -6, String(WUCurrentTemp) + "°");
//  display.setFont(ArialMT_Plain_16);
//  display.setTextAlignment(TEXT_ALIGN_CENTER);
//  String WUday = String(WUDate).substring(0,3);
//  WUday.toUpperCase();
//  display.drawString(DISPLAY_WIDTH - tempwidth/2, 30, WUday);
  //  display.drawString(DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2 - 10, "Ready\n");
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.drawString(DISPLAY_WIDTH / 2, DISPLAY_HEIGHT - 19, WUDate);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.drawString(DISPLAY_WIDTH / 2, DISPLAY_HEIGHT - 10, WiFi.localIP().toString());
    display.setTextAlignment(TEXT_ALIGN_LEFT);

  display.display();
}

void displaypaintAuto(int displayzone, int timeremaining, int totaltime) {
  //display.flipScreenVertically();//flipper
  display.setFont(ArialMT_Plain_16);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.clear();
  if (displayzone != 0) {
    display.drawProgressBar(0, 4, 84, 10, (timeremaining) * 100 / totaltime);
    display.setFont(ArialMT_Plain_16);
    display.drawString(92, 0, sec2time(timeremaining));

    display.setFont(Roboto_Medium_48);
    display.drawString(72, 16, String(displayzone));
    display.setFont(ArialMT_Plain_24);
    display.drawString(0, 24, "ZONE");
    display.setFont(ArialMT_Plain_16);
    display.drawString(0, 48, "Auto");
  }
  display.display();
}

int displaypaintManualOn(int displayzone) {
  //  display.init();
//  display.flipScreenVertically();//flipper
  display.setFont(ArialMT_Plain_16);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.clear();
  display.fillRect(0, 0, 128, 24);
  display.setFont(ArialMT_Plain_24);
  display.setColor(BLACK);
  display.drawString(20, 0, "SELECT");
  display.setColor(WHITE);

  if (displayzone == 0) {
    display.setFont(Roboto_Medium_48);
    display.drawString(24, 16, "OFF");
  }
  else {
    display.setFont(Roboto_Medium_48);
    display.drawString(72, 16, String(displayzone));
    display.setFont(ArialMT_Plain_24);
    display.drawString(0, 24, "ZONE");
  }
  display.display();
}

int displaypaintManualBlink(int displayzone) {
  display.clear();
  display.setFont(ArialMT_Plain_24);
  display.drawString(20, 0, "SELECT");

  if (displayzone == 0) {
    display.setFont(Roboto_Medium_48);
    display.drawString(24, 16, "OFF");
  }
  else {
    display.setFont(Roboto_Medium_48);
    display.drawString(72, 16, String(displayzone));
    display.setFont(ArialMT_Plain_24);
    display.drawString(0, 24, "ZONE");
  }
  display.display();
}

void displaypaintManualRun(int displayzone, int timeremaining, int totaltime) {
  display.clear();
  display.drawProgressBar(0, 4, 84, 10, (timeremaining) * 100 / totaltime);
  display.setFont(ArialMT_Plain_16);
  display.drawString(92, 0, sec2time(timeremaining));

  display.setFont(Roboto_Medium_48);
  display.drawString(72, 16, String(displayzone));
  display.setFont(ArialMT_Plain_24);
  display.drawString(0, 24, "ZONE");
  display.setFont(ArialMT_Plain_16);
  display.drawString(0, 48, "Manual");
  display.display();
}

void displaystatemachine() {
  if (autorun) {
    displaystate = 1;
    displaypaintAuto(runlist[runpointer - 1][0], timer_station_run.secondsremaining(), runlist[runpointer - 1][1]);
  }
  else {
    switch (displaystate) {
      case 0:
        if (switchedge) {
          manualmodezone = 0;
          displaypaintManualOn(manualmodezone);
          DisplayStateTimer.starttimer(2000, false);
          displaystate = 2;
          switchedge = false;
          serialdebug("displaystatemachine switchedge active, 0 -> 2");
        }
        break;
      case 1:
        displaypaintAuto(runlist[runpointer - 1][0], timer_station_run.secondsremaining(), runlist[runpointer - 1][1]);
        if (autorun != true) {
          displaystate = 0;
          displaypaintOff();
        }
        break;
      case 2:
      case 3:
      case 4:
      case 5:
      case 6:
      case 7:
        if (switchedge) {
          if (++manualmodezone > no_stations) manualmodezone = 0;
          displaypaintManualOn(manualmodezone);
          DisplayStateTimer.starttimer(2000, false);
          displaystate = 2;
          switchedge = false;
          serialdebug("displaystatemachine switchedge active, 2 to 7 -> 2");
        }
        break;
      case 8:
        if (switchedge) {
          // insert command to turn off station
          if (manualmodezone > 0)
            turn_off_device(manualmodezone);
          if (++manualmodezone > no_stations) manualmodezone = 0;
          displaypaintManualOn(manualmodezone);
          DisplayStateTimer.starttimer(2000, false);
          displaystate = 2;
          switchedge = false;
          serialdebug("displaystatemachine switchedge active, 8 -> 2");
        }
        else
          displaypaintManualRun(manualmodezone, DisplayStateTimer.secondsremaining(), manualmodetime);
        break;
      default:
        displaystate = 0;
        //displaypaintOff();
        break;
    }
  }
}

void processdisplaytimer() {
  if (DisplayStateTimer.processtimer()) {
    switch (displaystate) {
      case 0:
      case 1:
        break;
      case 2:
        DisplayStateTimer.starttimer(500, false);
        displaypaintManualBlink(manualmodezone);
        displaystate = 3;
        serialdebug("DisplayStateTimer 2 -> 3");
        break;
      case 3:
        DisplayStateTimer.starttimer(1000, false);
        displaypaintManualOn(manualmodezone);
        displaystate = 4;
        serialdebug("DisplayStateTimer 3 -> 4");
        break;
      case 4:
        DisplayStateTimer.starttimer(500, false);
        displaypaintManualBlink(manualmodezone);
        displaystate = 5;
        serialdebug("DisplayStateTimer 4 -> 5");
        break;
      case 5:
        DisplayStateTimer.starttimer(1000, false);
        displaypaintManualOn(manualmodezone);
        displaystate = 6;
        serialdebug("DisplayStateTimer 5 -> 6");
        break;
      case 6:
        DisplayStateTimer.starttimer(500, false);
        displaypaintManualBlink(manualmodezone);
        displaystate = 7;
        serialdebug("DisplayStateTimer 6 -> 7");
        break;
      case 7:
        if (manualmodezone > 0) {
          DisplayStateTimer.starttimer(manualmodetime * 1000, false);
          displaypaintManualRun(manualmodezone, DisplayStateTimer.secondsremaining(), manualmodetime);
          displaystate = 8;
          serialdebug("DisplayStateTimer 7 -> 8");
          // insert command to turn on station
          //         digitalWrite(id_stations[manualmodezone - 1], LOW);
          //root[String(runlist[manualmodezone - 1][0])]["mode"] = "on";
          runlist[0][0] = manualmodezone;
          runlist[0][1] = manualmodetime;
          for (int i = 1; i <= no_stations - 1; i++) {
            runlist[i][1] = 0;
          }
          autorun = false;
          runpointer = 1;
          runcount = 1;
          turn_on_device();


        }
        else {
          displaypaintOff();
          displaystate = 0;
          serialdebug("DisplayStateTimer 7 -> 0");
        }
        break;
      case 8:
        displaypaintOff();
        displaystate = 0;
        serialdebug("DisplayStateTimer 8 -> 0");
        // insert command to turn off station
        if (manualmodezone > 0)
          turn_off_device(manualmodezone);
        break;
    }
  }
}

//
//-------------------------------------Display Functions End---------------------------------

// Turn ON the onboard LED
//

void setup()
{
  digitalWrite(2, HIGH);
  digitalWrite(0, HIGH);

  for (int i = 0; i < no_stations; i++) {
    pinMode(id_stations[i], OUTPUT);
    digitalWrite(id_stations[i], HIGH);
  }
  pinMode(onboard_ledPin, OUTPUT);
  digitalWrite(onboard_ledPin, LOW);

  //NodeMCU  GPIO   GPIO
  //Pin      Code#
  //D8       15     GPIO15
  pinMode(switchpin, INPUT); // initialize the pushbutton switch GPIO

  display_setup();
  //display.init();
  //display.flipScreenVertically();
  //display.setContrast(255);
  display.clear();
  display.setFont(ArialMT_Plain_16);
  display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
  display.drawString(DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2 - 10, "Booting");
  display.display();

  delay(100);
  jsonString = buildJson();
  Serial.begin(9600);     // opens serial port, sets data rate to 9600 bps
  //Serial.swap();  // Serial is remapped to GPIO15 (TX) and GPIO13 (RX) by calling Serial.swap() after Serial.begin
  display_setup();
  display.clear();
  display.setFont(ArialMT_Plain_16);
  display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
  display.drawString(DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2 - 10, "WIFI Setup");
  display.display();
  setup_wifi();
  displaypaintOff();

  //debugger ("icon" + wunderground.getTodayIcon());
  if (mqtt_server[0] == '\0') // if mqtt_server is a null string then disable mqtt
    mqtt_enabled = false;
  else
    mqtt_enabled = true;


  /***********************************arduino setup*************/
  /********************************arduinoOTA*/

  ArduinoOTA.onStart([]() {
    debugger("Start updating ");
    Serial.println("Start updating ");
    display.clear();
    display.setFont(ArialMT_Plain_16);
    display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
    display.drawString(DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2 - 10, "OTA Update");
    display.display();

  });
  ArduinoOTA.onEnd([]() {
    debugger("End");
    Serial.println("\nEnd");
    display.clear();
    display.setFont(ArialMT_Plain_16);
    display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
    display.drawString(DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2 - 10, "Restart");
    display.display();

  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    debugger("Progress: %u%%\r" + (progress / (total / 100)));
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    display.drawProgressBar(4, 32, 120, 8, progress / (total / 100) );
    display.display();

  });
  ArduinoOTA.onError([](ota_error_t error) {
    debugger("Error[%u]: " + error);
    //    Serial.printf("Error[%u]: ", error);
    //    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    //    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    //    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    //    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    //    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });



  ArduinoOTA.begin();
  //  Serial.println("Ready");
  //  Serial.print("IP address: ");
  //  Serial.println(WiFi.localIP());

  //displaypaintOff();
  int mqtt_port_int = String(mqtt_port).toInt(); // take char array to String and then to int;
  if (mqtt_enabled)
    client.setServer(mqtt_server, mqtt_port_int);

  timer_intro_message.starttimer((unsigned long) 500, not_retriggerable); // wait 500 msec after setup before putting a welcome banner to debugger topic
  timer_mqtt_keepalive.starttimer(keepalive_interval, retriggerable); // start the mqtt keepalive periodic timer
  timer_blinkInterval.starttimer(5000, retriggerable);

  server.on("/", handleRoot);
  server.on("/on", handleOn);
  server.on("/off", handleOff);
  server.on("/subscribe", handleSubscribe);
  server.on("/poll", handlePoll);
  server.on("/json", handleJson);
  server.on("/alloff", handleAlloff);
  server.on("/blink", handleBlink);
#ifdef html_debug
  server.on("/debug", handleWebsite);
  server.on("/xml", handleXML);
  server.on("/reset", handleReset);
#endif

  server.on("/esp8266.xml", HTTP_GET, []() {
    SSDP.schema(server.client());
  });

  server.onNotFound(handleNotFound);

  server.begin();

  SSDP.setSchemaURL("esp8266.xml");
  SSDP.setHTTPPort(80);
  //SSDP.setName("ESP8266 Basic Switch");
  SSDP.setName("Sprinkler Controller with Display");
  SSDP.setSerialNumber("0112358132143");
  SSDP.setURL("index.html");
  SSDP.setModelName("Sprinkler Controller with Display");
  SSDP.setModelNumber("1");
  SSDP.setModelURL("https://github.com/SchreiverJ/SmartThingsESP8266/");
  SSDP.setManufacturer("bfindley");
  SSDP.setManufacturerURL("https://github.com/SchreiverJ/SmartThingsESP8266/");

  SSDP.begin();

  // Display Setup items
  //
  //serialdebug("Initializing Display");
  //display_setup();
}

void loop() {
  ArduinoOTA.handle();
  IPAddress ip_ofESP;
  unsigned long ip_long;
  int i;
  // buildjson();
  server.handleClient();
  // display the intro messages after things are initialized completely
  String sourcename;

  if (timer_intro_message.processtimer()) {
    debugger(String("ESP Sprinkler Controller starting..."));
    sourcename = String(__FILE__);
    debugger(sourcename.substring(sourcename.lastIndexOf("Compool")));
    debugger(String("Connected to ") + String(WiFi.SSID()));
    ip_ofESP = WiFi.localIP();
    ip_long = ip_ofESP;
    debugger(String("WiFi connected, IP address: ") + String(ip_long & 0xff) + "." + String((ip_long >> 8) & 0xff)
             + "." + String((ip_long >> 16) & 0xff) + "." + String((ip_long >> 24) & 0xff));
    debugger(String("mqtt_server: ") + String(mqtt_server));
    debugger(String("mqtt_port: ") + String(mqtt_port));
    debugger(String("cp_time_zone: ") + String(cp_time_zone));
  }

  if (timer_station_run.processtimer()) {
    //    debugger("done");
    debugger("turning off " + String(runlist[runpointer - 1][0]));
    turn_off_device(runlist[runpointer - 1][0]);
    runpointer += 1;
    if (runpointer <= runcount) {
      debugger("Pointer: " + String(runpointer));
      autorun = true;
      displaystate = 1;

      turn_on_device();
      server.send(200, "text/json", jsonString);

      //      turn_on_device();
    } else {
      for (int i = 1; i <= no_stations - 1; i++) {
        runlist[i][1] = 0;
      }
      runpointer = 0;
      runcount = 0;

    }
  }

  if (timer_station_run.minutes() >= 0) {
    no_blinks = timer_station_run.minutes() + 1;
    //debugger("blinks: " + String(no_blinks));
  }

  if (timer_blinkInterval.processtimer()) {
    //debugger("blink timer");
    onboard_led_off();
    blink_counter = 0;
    handleBlinkInterval();
  }

  if (timer_blink.processtimer()) {
    toggle_onboard_led_state();
    handleBlinkInterval();
  }
  // detect changes in any of the devices
  // Process items necessary to update the display
  process_switch();
  displaystatemachine();
  processdisplaytimer();
}

