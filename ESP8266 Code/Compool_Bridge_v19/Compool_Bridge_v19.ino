// Compool Bridge - Compool/MQTT packet Packet Bridge

// Note: Debug messages are available via html and/or mqtt.
// The MQTT Server ID must be set in the html WiFiManager configuration to send MQTT debug messages.
// If the MQTT Server ID in the html WiFiManager configuration screen is null then MQTT debug messages are not sent.
// To use html debugging then html_debug in the following statement must be defined
// If html_debug is commented out then html debugging is not available.

// * Author: Brian Findley, George Wiley
// * Date: 04/19/2020
// * Version 19


#define html_debug 1

#include <FS.h>                   //this needs to be first
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266SSDP.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <WiFiUdp.h>
#include <Time.h>
#include <TimeLib.h>
#include <stdio.h>

#include <WiFiManager.h>
#include <ArduinoOTA.h>

boolean mqtt_enabled = false; // enables MQTT debugging functions.

//default configuration values, if there are different values in config.json, they are overwritten.
char mqtt_server[40] = "";
char mqtt_port[6] = "";
//the following two parameters are not case sensitive. A String toLowerCase function converts them to lowercase before use.
char cp_time_zone[40] = "pacific"; //valid values are: pacific, mountain, central, eastern
char cp_auto_dst[10] = "auto"; //valid values are: auto, on, off. auto and on will automatically adjust for DST. off is for no DST.

#define max_mqtt_limit 84
#define keepalive_interval (unsigned long) 8000

#define id_spa  "spa"
#define id_pool "pool"
#define id_aux1 "aux1"
#define id_aux2 "aux2"
#define id_aux3 "aux3"
#define id_aux4 "aux4"
#define id_aux5 "aux5"
#define id_aux6 "aux6"
#define id_heaterstate "heaterstate"
#define id_solarstate "solarstate"
#define id_servicemode "servicemode"
#define id_debugging "debugging"

#define bit_spa  0x01
#define bit_pool 0x02
#define bit_aux1 0x04
#define bit_aux2 0x08
#define bit_aux3 0x10
#define bit_aux4 0x20
#define bit_aux5 0x40
#define bit_aux6 0x80

//hub definition
String hubAddress;
/*******************************************************Memory Leak Prevention***********************/
String sdmsg;

/****************************************************************************************************/

/********** This section defines global static variables used by the Compool Bridging Function **********/
//
WiFiClient espClient;

int serial_msg_binary[30]; // buffer to receive a serial port message from Compool
int serial_msg_binary_index = 0;

// state variables related to parsing the Basic Status Packet from Compool
int message_state = 0; // Compool message parsing state
// keeps track of remaining number of serial port bytes to receive after receiving the message length in Byte 5
int length_countdown = 0;
int msg_OpCode = 0;

// server_response_state, after an http command is received we must send a command to Compool,
// then receive an ACK from Compool, then receive a Basic Acknowledge Packet from Compool,
// and then we can send an http response with json structure back to SmartThings that contains the device status
#define resp_state_0 0 // no response is queued
#define resp_state_1 1 // received an http command processed by handler, now waiting for Compool ACK
#define resp_state_2 2 // received ACK from Compool, now waiting for Basic Acknowledge Packet from Compool
// when Basic Acknowledge Packet from Compool is received and in resp_state_2 then immediately send
// http response with json structure back to SmartThings and return to resp_state_0
int server_response_state = resp_state_0;

// definitions for the mqtt and html debuggers
//
PubSubClient client(espClient);

#ifdef html_debug
String webSite, javaScript, XML;
String debugmsg = "";
#endif


String cp_status_msg; // string containing the Compool status message so it can be output to the mqtt debugger

#define baseintmessagelength 17 // total message length of the command message to Compool
// baseintmessage contains fields that are common to all Basic Command Packets.
// All Basic Command Packets are built from this basic packet structure.
char baseintmessage[baseintmessagelength] = {0xff, 0xaa, 0x00, 0x01, 0x82, 0x09, 0x01, 0x01, 0, 0, 0, 0, 0, 0, 0, 0, 0};
char messagebuffer_int[max_mqtt_limit + 5];
int messagelength_int;
boolean messagequeued_int = false;

// states of individual Compool parameters
int compool_devices_state = 0;
int pcompool_devices_state = compool_devices_state;

int Water_Temperature = 65, Desired_Pool_Temp = 75, Desired_Spa_Temp = 95;
int pWater_Temperature = Water_Temperature, pDesired_Pool_Temp = Desired_Pool_Temp, pDesired_Spa_Temp = Desired_Spa_Temp;

int Pool_Heatmode = 0, Spa_Heatmode = 0;
int pPool_Heatmode = Pool_Heatmode, pSpa_Heatmode = Spa_Heatmode;

boolean Service_Mode = false, Heater_State = false, Solar_State = false;
boolean pService_Mode = Service_Mode, pHeater_State = Heater_State, pSolar_State = Solar_State;

int Solar_Temperature;
int CP_Hours, CP_Minutes;
int Spa_Water_Temp, Spa_Solar_Temp, Air_Temp, Equip_Sensor_Stat; // defined but not used

// GPIO pin and state definitions
#define onboard_ledPin 16   // GPIO16 is D0, the on-board LED
int onboard_led_state = 0;           // so we can toggle the LED on and off when Compool messages are received
#define serialdriverPin 5   // GPIO5 is D1, controls the MAX485 active high Driver Enable & active low Receiver Enable
#define debug_ledPin 14 //GPIO14 is D5, the debug LED used for http command testing
int debug_led_state = 0;  // so we can toggle the debug LED for testing http commands
#define safe_serial_gpioPin 4 //GPIO4 is D2, just a testpoint used to observe the variable: flag_safe_after_serial_msg

// Serial Port status
boolean       flag_safe_after_serial_msg = false;
#define       limit_safe_after_serial_msg (unsigned long)2450
int           limit_serial_msg_tx = 0;
int           keepalive_count = 0;

// UDP and NTP static variables and state definitions
unsigned int localPort = 2390;      // local port to listen for UDP packets
IPAddress timeServerIP; // time.nist.gov NTP server address
const char* ntpServerName = "time.nist.gov";
const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets
int cb;
#define st_inactive 0
#define st_idle 1
#define st_send_ntp_packet 2
#define st_wait_ntp_response 3
int settimestate = st_inactive;
int ntpwaitcount = 0;
boolean dstenable = true;
int GMToffset = -8; // GMT offset for Pacific Time, not corrected for DST
time_t ts;
int p_hour = -1;
boolean CPtimehasbeenset = false;

// A UDP instance to let us send and receive packets over UDP
WiFiUDP udp;

//flag for saving data
bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback() {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}


// Timer Definitions
class EventTimer {
  private:
    boolean timer_enabled = false;
    unsigned long expiration_time = 0;
    unsigned long retrigger_time = 0;
  public:
    void starttimer(unsigned long, boolean);
    void stoptimer() {
      timer_enabled = false;
    }
    boolean processtimer();
};

// Start a timer event
//   tmr is a pointer to an EventTimer structure
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

// Stop a timer that is currently running
//   tmr is a pointer to an EventTimer structure
//
//void stoptimer(EventTimer *tmr){
//  tmr->timer_enabled = false; // disable the timer to stop it
//}

// Evaluate a timer to determine whether it is expired
//   return TRUE if the timer has expired, or false if the timer has not yet expired
//   tmr is a pointer to an EventTimer structure
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

#define retriggerable true
#define not_retriggerable false
EventTimer timer_safe_after_serial_msg, timer_serial_msg_tx, timer_mqtt_keepalive, timer_intro_message;
EventTimer timer_holdoff_after_status_packet, timer_ntp_server_response_wait, timer_between_ntp_req;

//
/************************************************** End ************************************************/


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
  JsonObject& aux1 = root.createNestedObject(id_aux1);
  aux1["mode"] = "on";
  JsonObject& aux2 = root.createNestedObject(id_aux2);
  aux2["mode"] = "on";
  JsonObject& aux3 = root.createNestedObject(id_aux3);
  aux3["mode"] = "on";
  JsonObject& aux4 = root.createNestedObject(id_aux4);
  aux4["mode"] = "on";
  JsonObject& aux5 = root.createNestedObject(id_aux5);
  aux5["mode"] = "on";
  JsonObject& aux6 = root.createNestedObject(id_aux6);
  aux6["mode"] = "on";
  JsonObject& pool = root.createNestedObject(id_pool);
  pool["mode"] = "on";
  pool["settemp"] = 77;
  pool["currenttemp"] = 70;
  pool["heatmode"] = "off";
  JsonObject& spa = root.createNestedObject(id_spa);
  spa["mode"] = "on";
  spa["settemp"] = 90;
  spa["currenttemp"] = 70;
  spa["heatmode"] = "off";
  JsonObject& heaterstate = root.createNestedObject(id_heaterstate);
  heaterstate["mode"] = "off";
  JsonObject& solarstate = root.createNestedObject(id_solarstate);
  solarstate["mode"] = "off";
  JsonObject& servicemode = root.createNestedObject(id_servicemode);
  servicemode["mode"] = "off";
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
  //String tempmsg = debugmsg.substring(0, 2500); // new line added by Brian to keep the string from growing too large
  debugmsg = tempmsg + millis2time() + ": " + dmsg + "\n" ;
  //debugmsg = millis2time() + ": " + dmsg + "\n";
  //String tempmsg = dmsg + "<br>" + debugmsg;
  //debugmsg = tempmsg;
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

void handleColor() {
  int color = server.arg("color").toInt();
  String device = "aux1";
  turn_on_device(identify_device(device));
  for (int i = 0; i < color; i++) {
    turn_off_device(identify_device(device));
    delay(1000);
    turn_on_device(identify_device(device));
    delay(1000);
  }
  server.send(200, "text/json", jsonString);

}

void handleOn() {
  String device = server.arg("device");

  debug_led_on(); // Turn ON the debug LED when any device is turned on, for debugging
  debugger(device + " on"); // inserted debugger statement -gw
  if (turn_on_device(identify_device(device))) // turn on the device specified by the device string
    server_response_state = resp_state_1; // if successful, now wait for Compool ACK
  else {
    server_response_state = resp_state_0; // if not successful, immediately send response to ST, nothing to queue up
    prepare_server_response();
    server.send(200, "text/json", jsonString);
  }
}

void handleOff() {
  String device = server.arg("device");

  debug_led_off(); // Turn OFF the debug LED when any device is turned off, for debugging
  debugger(device + " off");
  if (turn_off_device(identify_device(device))) // turn off the device specified by the device string
    server_response_state = resp_state_1; // if successful, now wait for Compool ACK
  else {
    server_response_state = resp_state_0; // if not successful, immediately send response to ST, nothing to queue up
    prepare_server_response();
    server.send(200, "text/json", jsonString);
  }
}

// /settemp?device=pool&temp=XX
//
void handleSetTemp() {
  String device = server.arg("device");
  String tempvalue = server.arg("temp");

  debugger("SetTemp " + device);
  //root["spa"]["settemp"] = device; // looks strange, but the set temperature is in the device field
  if (device == id_spa) {
    update_spa_settemp(tempvalue.toInt());
    Desired_Spa_Temp = tempvalue.toInt();
    server_response_state = resp_state_1; // ST http command processed by this handler, now waiting for Compool ACK
  }
  if (device == id_pool) {
    update_pool_settemp(tempvalue.toInt());
    Desired_Pool_Temp = tempvalue.toInt();
    server_response_state = resp_state_1; // ST http command processed by this handler, now waiting for Compool ACK
  }
}

// /setheatmode?device=pool&mode=XX
// mode ("XX" above) can be: off, heater, solarpriority, solaronly
// if no mode parameter is specified then we change to the next heatmode
//
void handleHeatMode() {
  String device = server.arg("device");
  String heatmodevalue = server.arg("mode");

  debug_led_on(); // Turn ON the debug LED when heatmode is pressed, for debugging
  debugger("SetHeatMode " + device + ", Mode: " + heatmodevalue);

  if (heatmodevalue == "")
    next_heatmode_device(identify_device(device)); // cycle to next heatmode on device specified by the device string
  else
    heatmode_device(identify_device(device), identify_heatmode(heatmodevalue));

  server_response_state = resp_state_1; // ST http command processed by this handler, now waiting for Compool ACK
  // use the following to ----disable_heatmode----
  //server_response_state = resp_state_2; // sending command packet was disabled for testing, so wait for status packet
}

void handleSubscribe() {
  debugger("handleSubscribe");
  debugger(server.arg("address"));
  if (server.arg("address")){
  hubAddress = server.arg("address");
  }
  debugger(hubAddress);
}

void handlePoll() {
  // logic for handlePoll is different because there is no resulting Packet sent to Compool
  // the server response can be prepared and immediately returned to SmartThings

  server_response_state = resp_state_0; // immediately send response to ST, nothing to queue up
  debugger("handlePoll");
  prepare_server_response();
  server.send(200, "text/json", jsonString);
}

void handleNotFound() {
  server.send(404, "text/html", "<html><body>Error! Page Not Found!</body></html>");
  debugger("Error! Page Not Found! in handleNotFound");
}

void handleReset() {
  //server.sendHeader("Location", String("/debug"), true);
  server.send (200, "text/plain", "Connect to CompoolBridge via wifi to set up Compool Bridge");
  debugger("what's going on?");
  WiFiManager wifiManager;
  wifiManager.resetSettings();
  ESP.reset();
}
void handleSoftReset() {
  //server.sendHeader("Location", String("/debug"), true);
  server.send (200, "text/plain", "Resetting ESP8266");
  debugger("Soft Reset");
  ESP.reset();
}

void prepare_server_response() {
  StaticJsonBuffer<1000> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(jsonString);

  update_JsonObject_with_CP_devices_status(root);

  jsonString = "";
  root.prettyPrintTo(jsonString);

  int jsonlength, lencount;
  jsonlength = jsonString.length();
  debugger("Prepare response to ST, jsonString length = " + String(jsonlength));
  //for(lencount = 0; lencount < jsonlength; lencount += 70){
  //  if((lencount + 70) < jsonlength)
  //    debugger("  " + jsonString.substring(lencount, lencount + 70));
  //  else
  //    debugger("  " + jsonString.substring(lencount, jsonlength));
  //}

  // output the abbreviated jsonString info to the debugger
  String CP_time_temp;
  if (CP_Minutes < 10)
    CP_time_temp = CP_Hours + String(":0") + CP_Minutes;
  else
    CP_time_temp = CP_Hours + String(":") + CP_Minutes;

  debugger(String("  Pri: 0x") + String(compool_devices_state, HEX) + String(", Water T = ") + Water_Temperature +
           String("F, Solar T = ") + Solar_Temperature + String("F,") +
           " HS=" + String(b2i(Heater_State)) + " SS=" + String(b2i(Solar_State)) + " SM=" + String(b2i(Service_Mode)) +
           String(", CP time = ") + CP_time_temp);
  debugger(String("  Pool set T = ") + Desired_Pool_Temp + String("F, Pool HM = ") + convert_heatmode2str(Pool_Heatmode) +
           String(", Spa set T = ") + Desired_Spa_Temp + String("F, Spa HM = ") + convert_heatmode2str(Spa_Heatmode));
  debugger(String("Free=") + ESP.getFreeHeap());
}
//
/************************************************** End **************************************************/


/*******  This section contains functions to control the Compool Bridge that are called by JSON object handlers *******/
//
// function update the jsonObject fields based on the values presented from the Compool status message
//
void update_JsonObject_with_CP_devices_status(JsonObject& ptr_root) {
  StaticJsonBuffer<1000> jsonBuffer;
  //JsonObject& root = jsonBuffer.parseObject(jsonString);

  ptr_root[id_debugging]["freemem"] = ESP.getFreeHeap(); //doing this at the top in case any code below reduces the heapsize
  ptr_root[id_debugging]["runseconds"] = millis2time(); //so we can see how long since a reboot

  if ((compool_devices_state & bit_spa) == 0)
    ptr_root[id_spa]["mode"] = "off";
  else
    ptr_root[id_spa]["mode"] = "on";

  ptr_root[id_spa]["settemp"] = Desired_Spa_Temp;
  ptr_root[id_spa]["currenttemp"] = Water_Temperature;
  // as discussed, it appears that a single value, Water_Temperature, is used for both pool and spa and
  // is the pool or spa based on whether the system is in pool or spa mode
  ptr_root[id_spa]["heatmode"] = convert_heatmode2str(Spa_Heatmode);

  if ((compool_devices_state & bit_pool) == 0)
    ptr_root[id_pool]["mode"] = "off";
  else
    ptr_root[id_pool]["mode"] = "on";

  ptr_root[id_pool]["settemp"] = Desired_Pool_Temp;
  ptr_root[id_pool]["currenttemp"] = Water_Temperature; // not sure if it's the right parameter, best fit based on name
  ptr_root[id_pool]["heatmode"] = convert_heatmode2str(Pool_Heatmode);
  //debugger(String("Pool Heatmode: ") + convert_heatmode2str(Pool_Heatmode) + String(", Spa Heatmode: ") +
  //  convert_heatmode2str(Spa_Heatmode));
  // defined parameters from Compool that are not yet filled into the JsonObject Structure:
  // Solar_Temperature, Spa_Water_Temp, Spa_Solar_Temp, Air_Temp, Equip_Sensor_Stat;
  // The following bytes are zero in the Compool status report:
  //   Byte 13, Spa_Water_Temp
  //   Byte 14, Spa_Solar_Temp
  //   Byte 17, Air_Temp

  if ((compool_devices_state & bit_aux1) == 0)
    ptr_root[id_aux1]["mode"] = "off";
  else
    ptr_root[id_aux1]["mode"] = "on";

  if ((compool_devices_state & bit_aux2) == 0)
    ptr_root[id_aux2]["mode"] = "off";
  else
    ptr_root[id_aux2]["mode"] = "on";

  if ((compool_devices_state & bit_aux3) == 0)
    ptr_root[id_aux3]["mode"] = "off";
  else
    ptr_root[id_aux3]["mode"] = "on";

  if ((compool_devices_state & bit_aux4) == 0)
    ptr_root[id_aux4]["mode"] = "off";
  else
    ptr_root[id_aux4]["mode"] = "on";

  if ((compool_devices_state & bit_aux5) == 0)
    ptr_root[id_aux5]["mode"] = "off";
  else
    ptr_root[id_aux5]["mode"] = "on";

  if ((compool_devices_state & bit_aux6) == 0)
    ptr_root[id_aux6]["mode"] = "off";
  else
    ptr_root[id_aux6]["mode"] = "on";

  if (Heater_State)
    ptr_root[id_heaterstate]["mode"] = "on";
  else
    ptr_root[id_heaterstate]["mode"] = "off";

  if (Solar_State)
    ptr_root[id_solarstate]["mode"] = "on";
  else
    ptr_root[id_solarstate]["mode"] = "off";

  if (Service_Mode)
    ptr_root[id_servicemode]["mode"] = "on";
  else
    ptr_root[id_servicemode]["mode"] = "off";
}

// function identify_device()
// is given the device string that describes the device (such as: spa, pool, aux1, etc.)
// and returns the bitmask for the device specified in the device_string
// The bit numbers for each device are defined in the "ComPool protocol.pdf" document
// the definitions of bit_spa, bit_pool, bit_aux1, etc. are near the top of this file
// example: bit 0 is spa, bit 1 is pool, bit 2 is aux1, etc.
//
int identify_device(String device_string) {
  if (device_string == id_spa) return (bit_spa);
  else if (device_string == id_pool) return (bit_pool);
  else if (device_string == id_aux1) return (bit_aux1);
  else if (device_string == id_aux2) return (bit_aux2);
  else if (device_string == id_aux3) return (bit_aux3);
  else if (device_string == id_aux4) return (bit_aux4);
  else if (device_string == id_aux5) return (bit_aux5);
  else if (device_string == id_aux6) return (bit_aux6);
  else {
    debugger("bad device string: " + device_string);
    return (0);
  }
}

// function identify_heatmode(int heatmodevalue)
// is given the heatmode string that describes the device (such as: off, heater, solar, solarpriority)
// and returns the 2-bit code for the heatmode specified in the device_string.
// The heatmode values for each device are defined in the "ComPool protocol.pdf" document
//
int identify_heatmode(String heatmodevalue) {
  if (heatmodevalue == "off") return (0);
  else if (heatmodevalue == "heater") return (1);
  else if (heatmodevalue == "solarpriority") return (2);
  else if (heatmodevalue == "solaronly") return (3);
  else {
    debugger("bad heat source parameter: " + heatmodevalue);
    return (-1);
  }
}

// update spa settemp
//
void update_spa_settemp(long int new_spa_temp) {
  debugger(String("change Spa temp to: ") + new_spa_temp);
  if (messagequeued_int == false)
    buildintmessage(); // build the basic message and then fill in the relevant fields
  messagebuffer_int[14] |= 0x40; // set the Byte Use Enable bit for Desired Spa Temp
  messagebuffer_int[12] = convert_f2c(new_spa_temp, 4); // set the Desired Spa Temp field;
  computechecksum(messagebuffer_int, baseintmessagelength);
  messagequeued_int = true;
}

// update pool settemp
//
void update_pool_settemp(long int new_pool_temp) {
  debugger(String("change Pool temp to: ") + new_pool_temp);
  if (messagequeued_int == false)
    buildintmessage(); // build the basic message and then fill in the relevant fields
  messagebuffer_int[14] |= 0x20; // set the Byte Use Enable bit for Desired Pool (Water) Temp
  messagebuffer_int[11] = convert_f2c(new_pool_temp, 4); // set the Desired Pool Temp field;
  computechecksum(messagebuffer_int, baseintmessagelength);
  messagequeued_int = true;
}

// change to next heatmode, this is the old handler that responds immediately to advance the heatmode
// to the next value when the heatmode button is pressed.
//
void heatmode_device(int device_bitmask, int heatmode_value) {
  if (heatmode_value < 0) {
    debugger(String("bad heatmode value, setting heatmode to 'off'"));
    heatmode_value = 0;
  }
  if (device_bitmask == bit_spa) {
    Spa_Heatmode = heatmode_value & 0x3;
    debugger(String("Change Spa heatmode to: ") + Spa_Heatmode);
  }
  if (device_bitmask == bit_pool) {
    Pool_Heatmode = heatmode_value & 0x3;
    debugger(String("Change Pool heatmode to: ") + Pool_Heatmode);
  }
  if (messagequeued_int == false)
    buildintmessage(); // build the basic message and then fill in the relevant fields
  messagebuffer_int[14] |= 0x10; // set the Byte Use Enable bit for Change Heat Source
  messagebuffer_int[10] = ((Spa_Heatmode & 0x3) << 6) | ((Pool_Heatmode & 0x3) << 4);
  computechecksum(messagebuffer_int, baseintmessagelength);
  // comment out the following to ----disable_heatmode----
  messagequeued_int = true;
}

// change to next heatmode, this is the old handler that responds immediately to advance the heatmode
// to the next value when the heatmode button is pressed.
//
void next_heatmode_device(int device_bitmask) {
  if (device_bitmask == bit_spa) {
    Spa_Heatmode = ++Spa_Heatmode & 0x3;
    debugger(String("Next heatmode, change Spa heatmode to: ") + Spa_Heatmode);
  }
  if (device_bitmask == bit_pool) {
    Pool_Heatmode = ++Pool_Heatmode & 0x3;
    debugger(String("Next heatmode, change Pool heatmode to: ") + Pool_Heatmode);
  }
  if (messagequeued_int == false)
    buildintmessage(); // build the basic message and then fill in the relevant fields
  messagebuffer_int[14] |= 0x10; // set the Byte Use Enable bit for Change Heat Source
  messagebuffer_int[10] = ((Spa_Heatmode & 0x3) << 6) | ((Pool_Heatmode & 0x3) << 4);
  computechecksum(messagebuffer_int, baseintmessagelength);
  // comment out the following to ----disable_heatmode----
  messagequeued_int = true;
}

// turn ON the devices specified by the bitmask
// if the device is already on then don't send a command to the Compool
// if the device is off and needs to be turned on then build the Compool packet and queue
// the message so it can be sent to the Serial handler which happend in the main loop
//
boolean turn_on_device(int device_bitmask) {
  boolean retval;
  if (Service_Mode) {
    debugger("Compool in service mode");
    retval = false; // return false if the device is already off and no message was queued
    return (retval);
  }
  if ((compool_devices_state & device_bitmask) == 0) { // if the device is OFF then build and queue a message to turn it ON
    //debugger(String("turning on device, mask = 0x") + String(device_bitmask, HEX));
    if (messagequeued_int == false)
      buildintmessage(); // build the basic message and then fill in the relevant fields
    messagebuffer_int[14] |= 0x04; // set the Byte Use Enable bit for Primary Equipment
    messagebuffer_int[8] = device_bitmask; // set the bit in the Primary Equipment field to toggle the desired device
    computechecksum(messagebuffer_int, baseintmessagelength);
    messagequeued_int = true;
    if (flag_safe_after_serial_msg)
      debugger(String("queued ON command, mask = 0x") + String(device_bitmask, HEX) + String(", safe = TRUE"));
    else
      debugger(String("queued ON command, mask = 0x") + String(device_bitmask, HEX) + String(", safe = FALSE"));
    retval = true; //return true if a message was queued
  }
  else { // else if the device is already ON, then don't do anything
    debugger(String("device is already on, mask = 0x") + String(device_bitmask, HEX));
    retval = false; // return false if the device is already on and no message was queued
  }
  return (retval);
}

// turn OFF the devices specified by the bitmask
// if the device is already off then don't send a command to the Compool
// if the device is on and needs to be turned off then build the Compool packet and queue
// the message so it can be sent to the Serial handler which happend in the main loop
//
boolean turn_off_device(int device_bitmask) {
  boolean retval;
  if (Service_Mode) {
    debugger("Compool in service mode");
    retval = false; // return false if the device is already off and no message was queued
    return (retval);
  }

  if ((compool_devices_state & device_bitmask) != 0) { // if the device is ON then build and queue a message to turn it OFF
    //debugger(String("turning off device, mask = 0x") + String(device_bitmask, HEX));
    if (messagequeued_int == false)
      buildintmessage(); // build the basic message and then fill in the relevant fields
    messagebuffer_int[14] |= 0x04; // set the Byte Use Enable bit for Primary Equipment
    messagebuffer_int[8] = device_bitmask; // set the bit in the Primary Equipment field to toggle the desired device
    computechecksum(messagebuffer_int, baseintmessagelength);
    messagequeued_int = true;
    if (flag_safe_after_serial_msg)
      debugger(String("queued OFF command, mask = 0x") + String(device_bitmask, HEX) + String(", safe = TRUE"));
    else
      debugger(String("queued OFF command, mask = 0x") + String(device_bitmask, HEX) + String(", safe = FALSE"));
    retval = true; //return true if a message was queued
  }
  else { // else if the device is already OFF, then don't do anything
    debugger(String("device is already off, mask = 0x") + String(device_bitmask, HEX));
    retval = false; // return false if the device is already off and no message was queued
  }
  return (retval);
}

// after the Compool packet is built, then compute the checksum and store it in the last 2 bytes of the packet
//
void computechecksum(char *chkbuf, int clength) {
  int i, csum;

  csum = 0;
  for (i = 0; i < (clength - 2); i++)
    csum += chkbuf[i];
  chkbuf[clength - 2] = (csum >> 8) & 0xff; // save the high byte in the second to last byte of the message
  chkbuf[clength - 1] = csum & 0xff; // save the low byte in the last byte of the message
}

// Build the "boilerplate" stuff in the Compool packet by copying the packet template into the message buffer
// Specific packet fields will be updated after the basic packet fields are filled in using this function
//
void buildintmessage() {
  int i;

  for (i = 0; i < baseintmessagelength; i++)
    messagebuffer_int[i] = baseintmessage[i];
}
/************************************************** End **************************************************/


/****************************  This section contains the Compool Bridge  code ****************************/

// Debugging function that outputs debug messages via mqtt and the html debug page
//
void debugger(String debugmessage) {
  char dmsg[max_mqtt_limit + 6];

  // only publish to mqtt if mqtt_enabled is true.

  String sdmsgtimed;
  if (mqtt_enabled) {
    sdmsgtimed = String(millis()) + "; " + debugmessage;
    sdmsgtimed.toCharArray(dmsg, max_mqtt_limit + 5);
    client.publish("debug/compool", dmsg);
  }

#ifdef html_debug
  htmlDebug(debugmessage);
#endif
}

// eventlog() outputs debug messages via mqtt using the "eventlog/compool" topic. eventlog() outputs
// messages that are of higher importance than debugger. eventlog() messages might be stored in a log file in a server.
//
void eventlog(String eventmessage) {
  char emsg[max_mqtt_limit + 1];

  if (mqtt_enabled) {
    eventmessage.toCharArray(emsg, max_mqtt_limit);
    client.publish("eventlog/compool", emsg);
  }
}

// Start a timer and declare that is it safe to transmit a Compool packet for a little less than 2.6 seconds
// after a Compool status message is received from the LX3xxx unit that is near the pool equipment
//
void start_safe_after_serial_msg() {
  timer_safe_after_serial_msg.starttimer(limit_safe_after_serial_msg, not_retriggerable);
  flag_safe_after_serial_msg = true;
  //debugger(String("start_safe_after_serial_msg(), flag_safe_after_serial_msg = true;"));
  digitalWrite(safe_serial_gpioPin, HIGH);  // Set the safe_serial_gpioPin HIGH
}

// Declare that it is now unsafe to transmit a Compool packet because it might collide with a
// Compool status message sent by the LX3xxx unit that is near the pool equipment
//
void clear_safe_after_serial_msg() {
  timer_safe_after_serial_msg.stoptimer();
  flag_safe_after_serial_msg = false;
  //debugger(String("clear_safe_after_serial_msg(), flag_safe_after_serial_msg = false;"));
  digitalWrite(safe_serial_gpioPin, LOW);  // Set the safe_serial_gpioPin LOW
}

// Place the MAX485 chip into transmit mode for slightly longer than the duration of transmission of the Compool packet.
// We leave the MAX485 in transmit mode for about 1 msec longer than is required to send the serial message.
//
void setserial_transmitmode(int tx_time_chars) {
  digitalWrite(serialdriverPin, HIGH);  // set the MAX485 to transmit mode
  // start a timer that will expire at the end of transmission over the serial port
  // 1.0417 msec/char, so add 1 msec for some overlap, example: 17 chars = 17 * 1.0417 msec = 17.708 msec for the message.
  // We create the pulse length to be 17 + 1 = 18 msec.
  // This simple method fails if we transmit messages longer than 23 bytes.
  // Fortunately, Compool Basic Command Packets always have a length of 17 bytes total.
  timer_serial_msg_tx.starttimer((unsigned long) tx_time_chars + 4, not_retriggerable);
}

// Place the MAX485 chip in receive mode
//
void setserial_receivemode() {
  digitalWrite(serialdriverPin, LOW);  // set the MAX485 to receive mode
}

// Queue up a message so it can be transmitted to the LX3xxx unit over the serial port
//
void queue_serial_message_int(int msg_length) { // queue up a message to be transmitted over the serial port
  // this simple queue holds only one message
  messagelength_int = msg_length;
  messagequeued_int = true;
}

// initialize the ESP8266 GPIO pins
//
void initialize_gpio_pins() { // set the mode and state of the GPIO pins, see below
  pinMode(onboard_ledPin, OUTPUT);
  digitalWrite(onboard_ledPin, HIGH);  // Turn the LED off by setting the voltage HIGH
  pinMode(serialdriverPin, OUTPUT);
  digitalWrite(serialdriverPin, LOW);  // set the MAX485 to receive mode
  pinMode(debug_ledPin, OUTPUT);
  digitalWrite(debug_ledPin, HIGH);  // Turn the debug LED off by setting the voltage HIGH
  pinMode(safe_serial_gpioPin, OUTPUT);
  digitalWrite(safe_serial_gpioPin, LOW);  // Set the safe_serial_gpioPin LOW
}

// Initialize the WiFi in the ESP8266
//

void setup_wifi() {
  IPAddress ip_ofESP;

  debug_led_off();
  onboard_led_off();

  delay(10);
  // We start by connecting to a WiFi network

  populate_config_from_fs();
  WiFiManager wifiManager;
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
  wifiManager.autoConnect("CompoolBridge");

  while (WiFi.status() != WL_CONNECTED) {
    onboard_led_on();
    delay(250);
    onboard_led_off();
    delay(250);
    // Serial.print(".");
  }
  debug_led_on();
  onboard_led_on();

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

// function to convert the heatmode numeric value to a string describing the heatmode for the json object
//
String convert_heatmode2str(int heatmodeval) {
  String retval;

  switch (heatmodeval) {
    case 0:
      retval = "off";
      break;
    case 1:
      retval = "heater";
      break;
    case 2:
      retval = "solarpriority";
      break;
    case 3:
      retval = "solaronly";
      break;
    default:
      retval = "Undefined_heatmode";
      break;
  }
  return (retval);
}

// function boolean to integer for use with the String functions
//
int b2i(boolean torf) {
  if (torf)
    return (1);
  else
    return (0);
}

// function to convert temperature in integer degrees F to fractional degrees C.
// This is for receiving temperature data from SmartThings to build commands for Compool.
// Some Compool temperatures are presented in units of 0.25 degrees C, and others in units of 0.5 degrees C.
// parameter temp_deg_f is the temperature value in integer degrees F
// parameter temp_frac is either 4 or 2, to specify units of quarter or half degrees C, respectively
// The result is returned as a temperature in fractional degrees C, as presented to Compool in the command packet
//
int convert_f2c(int temp_deg_f, int temp_frac) {
  long int tempcalc;

  // Hoping the compiler doesn't optimize this. To divide by 1.8 we multiply by 5 first, then divide by 9.
  // This will prevent loss of precision
  tempcalc = temp_deg_f - 32; // subtract the 32 degree F offset
  tempcalc = tempcalc * 5 * 36; // divide by 1.8 which is 9/5, scale up 36x, include the fractional degrees C in the result
  tempcalc = (tempcalc + 10) / (9 * 9); // part of 1.8x and remove 36x scaling
  // We now have accurate degrees C * 4 in tempcalc.
  // The next step is to convert this to the same value produced by the CP3800 controller.
  // This is less accurate but temperatures on the ST app GUI will match temperatures on the CP3800
  tempcalc = (tempcalc + 2) & 0xfffffffe; // add 2 and truncate the LSB. Yes, it's strange, but copies CP3800 behavior.
  if (temp_deg_f == 104) // probably a CP3800 bug, unknown why 104 deg F produces 160 instead of 162
    tempcalc = 160;
  if (temp_frac == 2) // if temp_frac is 2 then return value in units of 0.5 deg C, otherwise return value in units of 0.25 deg C
    tempcalc = tempcalc / 2;
  return ((int) tempcalc);
}

// function to convert temperature in fractional degrees C to integer degrees F.
// This is for receiving temperature data from Compool to display it in SmartThings.
// Some Compool temperatures are presented in units of 0.25 degrees C, and others in units of 0.5 degrees C.
// parameter temp_deg_c is the temperature value in fractional degrees C, as presented in the Compool status packet
// parameter temp_frac is either 4 or 2, to specify units of quarter or half degrees C, respectively
// The result is returned as a temperature in degrees F without any fractional part
//
int convert_c2f(int temp_deg_c, int temp_frac) {
  long int tempcalc;

  // Hoping the compiler doesn't optimize this. To multiply by 1.8 we multiply by 9 first, then divide by 5.
  // This will prevent loss of precision
  //tempcalc = temp_deg_c * 36; // increase precision so we can add precisely 0.5 deg F or 0.2777777 deg C in the next step
  // the following statement rounds to the nearest degree F,
  // might need to disable rounding so our temp in degrees F will match the Compool controller
  //tempcalc = tempcalc + (10 * temp_frac); // add 0.5 deg F or 0.2777777 deg C to round up the deg F result
  //tempcalc = tempcalc * 9; // multiply by 1.8 which is 9/5
  //tempcalc = tempcalc / (5 * 36); // part of 1.8x and bring the scaling back down by removing the x36 factor
  //tempcalc = tempcalc / temp_frac; // remove the fractional degrees C from the result
  //tempcalc = tempcalc + 32; // add the 32 degree F offset

  // the following is not the most accurate but matches the algorithm in the CP3800 controller
  tempcalc = temp_deg_c * 9;
  tempcalc = tempcalc / (5 * temp_frac);
  tempcalc = tempcalc + 32;
  return ((int) tempcalc);
}

// converts an ASCII hex character to a 4-bit nibble value
//
int char2nibble(char charval) { // converts a single hex character to a nibble
  int retval;

  if ((charval >= '0') && (charval <= '9'))
    retval = charval - '0';
  else if ((charval >= 'A') && (charval <= 'F'))
    retval = charval - 'A' + 0x0a;
  else if ((charval >= 'a') && (charval <= 'f'))
    retval = charval - 'a' + 0x0a;
  else
    retval = 0;
  return (retval);
}

// function to reconnect to the mqtt server, only used for mqtt debug output
//
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    //Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "Compool-ESPClient-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    // If you do not want to use a username and password, change next line to
    if (client.connect(clientId.c_str())) {
      // if (client.connect(clientId.c_str(), mqtt_user, mqtt_password)) {
      //Serial.println("connected");
      debugger(String("mqtt connected"));
    } else {
      //Serial.print("failed, rc=");
      //Serial.print(client.state());
      //Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

// send an NTP request to the time server at the given address
unsigned long sendNTPpacket(IPAddress& address) {
  debugger(String("filling NTP packet buffer"));
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  // we don't need much accuracy for Compool, choosing a lower stratum so more servers are available
  packetBuffer[1] = 2;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}

// initializeNTP()
//
void initializeNTP() {
  String s_auto_dst, s_time_zone;

  udp.begin(localPort);
  timer_between_ntp_req.starttimer(500, not_retriggerable); //
  settimestate = st_idle;

  s_auto_dst = String(cp_auto_dst);
  s_auto_dst.toLowerCase();
  if (s_auto_dst == "auto") dstenable = true;
  else if (s_auto_dst == "on") dstenable = true;
  else if (s_auto_dst == "off") dstenable = false;

  GMToffset = -8;
  s_time_zone = String(cp_time_zone);
  s_time_zone.toLowerCase();
  if (s_time_zone == "pacific") GMToffset = -8;
  else if (s_time_zone == "mountain") GMToffset = -7;
  else if (s_time_zone == "central") GMToffset = -6;
  else if (s_time_zone == "eastern") GMToffset = -5;
}

// processNTP() ensures that the Compool time is accurate
//
void processNTP() {
  switch (settimestate) {
    case st_inactive:
      break;
    case st_idle:
      if (timer_between_ntp_req.processtimer()) {
        settimestate = st_send_ntp_packet;
        ntpwaitcount = 1;
      }
      break;
    case st_send_ntp_packet:
      //get a random server from the pool
      debugger("Send NTP Packet, " + String(ntpServerName));
      WiFi.hostByName(ntpServerName, timeServerIP);
      //debugger("NTP after hostByName");
      sendNTPpacket(timeServerIP); // send an NTP packet to a time server
      //debugger("NTP after sendNTPpacket");
      // wait to see if a reply is available
      timer_ntp_server_response_wait.starttimer(5000, not_retriggerable);
      settimestate = st_wait_ntp_response;
      break;
    case st_wait_ntp_response:
      cb = udp.parsePacket();
      if (!cb) {
        if (timer_ntp_server_response_wait.processtimer()) { //if no response after 5 seconds
          debugger(String("Error, timed out waiting for NTP response") + ntpwaitcount);
          eventlog(String("Error, timed out waiting for NTP response") + ntpwaitcount);
          if (++ntpwaitcount <= 3) { //try 3 times
            settimestate = st_send_ntp_packet; // set the state to try again by sending another NTP packet
          }
          else { // give up after 3 tries and try again in an hour
            settimestate = st_idle; // set the state to try again by sending another NTP packet
            timer_between_ntp_req.starttimer(3600 * 1000, not_retriggerable); //
            debugger(String("NTP server response timeout failed 3 times"));
            eventlog(String("NTP server response timeout failed 3 times"));
          }
        }
      }
      else {
        debugger(String("NTP Response Packet received, length = ") + String(cb));
        // We've received a packet, read the data from it
        udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer

        //the timestamp starts at byte 40 of the received packet and is four bytes,
        // or two words, long. First, esxtract the two words:

        unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
        unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
        // combine the four bytes (two words) into a long integer
        // this is NTP time (seconds since Jan 1 1900):
        unsigned long secsSince1900 = highWord << 16 | lowWord;
        //debugger(String("Seconds since Jan 1 1900 = ") + secsSince1900);

        // now convert NTP time into everyday time:
        // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
        const unsigned long seventyYears = 2208988800UL;
        // subtract seventy years:
        unsigned long epoch = secsSince1900 - seventyYears;
        //unsigned long epoch = 1489312784; // 3-12-2017 01:59:44 test
        //unsigned long epoch = 1509872384; // 11-5-2017 01:59:44 test
        //unsigned long epoch = 1268560784; // 3-14-2010 01:59:44 test
        //unsigned long epoch = 1289120384; // 11-7-2010 01:59:44 test
        //unsigned long epoch = 1236506384; // 3-8-2009 01:59:44 test
        //unsigned long epoch = 1257065984; // 11-1-2009 01:59:44 test
        setTime(epoch + (GMToffset * 3600));
        // print Unix time:
        debugger(String("NTP Response Packet received, Unix time = ") + epoch);
        eventlog(String("NTP Response Packet received, Unix time = ") + epoch);
        settimestate = st_idle;
        if (!CPtimehasbeenset) // Only set the CP time if it has never been set since boot. Best accuracy is on hour-change.
          buildsettimepacket();
        timer_between_ntp_req.starttimer(2 * 24 * 3600 * 1000, not_retriggerable); // will request NTP update 2 days from now
        //timer_between_ntp_req.starttimer(2*60*1000, not_retriggerable); // for testing request NTP update 2 min from now
      }
      break;
    default:
      debugger(String("Error, invalid settimestate = ") + settimestate);
      settimestate = st_idle;
      break;
  }
}

// checkdst returns true if the current time is in DST, and false if the current time is not DST
// if the global dstenable is false then return false indicating that we're not in DST
//
boolean checkdst(time_t stime) {
  int datechk;

  if (dstenable) {
    if ((month(stime) > 3) && (month(stime) < 11)) return (true);
    datechk = day(stime) - (weekday(stime) - 1);
    if (datechk <= 0) datechk += 7;
    if ((month(stime) == 11) && (day(stime) < datechk)) return (true);
    if ((month(stime) == 11) && (day(stime) == datechk) && (hour(stime) < 1)) return (true);
    datechk = day(stime) - (weekday(stime) - 1);
    if (datechk <= 7) datechk += 7;
    if ((month(stime) == 3) && (day(stime) > datechk)) return (true);
    if ((month(stime) == 3) && (day(stime) == datechk) && (hour(stime) >= 2)) return (true);
    return (false);
  }
  else
    return (false);
}

// process_setclock() sets the Compool with the current time
void process_setclock() {
  int m11;

  if (timeStatus() != timeNotSet) { // only update the CP clock if the time has been set by NTP
    ts = now();
    if (month(ts) >= 11) m11 = 0;
    else m11 = 1;
    if ((hour(ts) == (m11 + 1)) && (p_hour == m11))
      buildsettimepacket(); // setting time right on the hour change improves accuracy of the CP3800 clock
    p_hour = hour(ts);
  }
}

// buildsettimepacket() creates a packet to set the CP time
//
void buildsettimepacket() {
  int houradj;

  ts = now();
  if (checkdst(ts)) houradj = hour(ts) + 1;
  else houradj = hour(ts);
  if (houradj >= 24) houradj -= 24;
  if (minute(ts) < 10) debugger(String("Set the Compool clock to ") + houradj + String(":0") + minute(ts));
  else debugger(String("Set the Compool clock to ") + houradj + String(":") + minute(ts));
  if (messagequeued_int == false)
    buildintmessage(); // build the basic message and then fill in the relevant fields
  messagebuffer_int[14] |= 0x03; // set the Byte Use Enable bit for Minutes and Hours
  messagebuffer_int[6] = minute(ts); // set the Minutes field;
  messagebuffer_int[7] = houradj; // set the Hours field;
  computechecksum(messagebuffer_int, baseintmessagelength);
  messagequeued_int = true;
  CPtimehasbeenset = true;
}

// a soft reset function in case we need it later
// in case there are memory leak issues, can detect that free mem is low and reset
//
//void software_Reset() // Restarts program from beginning but does not reset the peripherals and registers
//{
//  asm volatile ("  jmp 0");
//}

// Turn ON the onboard LED
//
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
  if (onboard_led_state == 0)
    onboard_led_on();
  else
    onboard_led_off();
}

// Turn ON the debug LED. This is the LED between the ESP8266 and RS485 boards.
//
void debug_led_on() {
  debug_led_state = 1;
  digitalWrite(debug_ledPin, LOW);   // Turn the debug LED on by setting the voltage LOW
}

// Turn OFF the debug LED. This is the LED between the ESP8266 and RS485 boards.
//
void debug_led_off() {
  debug_led_state = 0;
  digitalWrite(debug_ledPin, HIGH);   // Turn the debug LED off by setting the voltage HIGH
}

// Toggle the state of the debug LED. This is the LED between the ESP8266 and RS485 boards.
//
void toggle_debug_led_state()
{
  if (debug_led_state == 0)
    debug_led_on();
  else
    debug_led_off();
}

// Capture a nibble in the Compool message string, for debugging only.
//
void cap_nibble(int nib)
{
  char ascii_nibble;

  if (nib < 10)
    ascii_nibble = nib + '0';
  else
    ascii_nibble = nib - 10 + 'A';
  cp_status_msg = cp_status_msg + ascii_nibble;
}

// Capture a byte in the serial port message buffer,
// and also in the Compool message string.
//
void capture_byte(int m_byte)
{
  serial_msg_binary[serial_msg_binary_index++] = m_byte;
  cap_nibble(0xf & (m_byte >> 4)); // high nibble
  cap_nibble(0xf & m_byte);  // low nibble
  cp_status_msg = cp_status_msg + " "; // then a space
}

// Capture the timestamp in the Compool message string, for debugging only.
//
void capture_timestamp(unsigned long timeword)
{
  cap_nibble(0xf & (int)(timeword >> 28)); // 8th high nibble
  cap_nibble(0xf & (int)(timeword >> 24)); // 7th high nibble
  cap_nibble(0xf & (int)(timeword >> 20)); // 6th high nibble
  cap_nibble(0xf & (int)(timeword >> 16)); // 5th high nibble
  cap_nibble(0xf & (int)(timeword >> 12)); // 4th high nibble
  cap_nibble(0xf & (int)(timeword >> 8)); // 3rd high nibble
  cap_nibble(0xf & (int)(timeword >> 4)); // high nibble
  cap_nibble(0xf & timeword);  // low nibble
  cp_status_msg = cp_status_msg + " "; // then a space
}

void parse_message(int msg_byte)
{
  char temphexvals[20];
  //Serial.print(message_state);
  //Serial.print(" ");
  //Serial.println("msg_byte");

  switch (message_state) {
    case 0:   // search for FF in sync byte 0
      if (msg_byte == 0xff) {
        cp_status_msg = ">";
        serial_msg_binary_index = 0;
        //capture_timestamp(millis());
        capture_byte(msg_byte);
        message_state = 1;
      }
      else
        message_state = 0;
      break;
    case 1:   // confirm AA in sync byte 1
      if (msg_byte == 0xff) {
        cp_status_msg = ">";
        serial_msg_binary_index = 0;
        //capture_timestamp(millis());
        capture_byte(msg_byte);
      }
      else if (msg_byte == 0xaa) {
        capture_byte(msg_byte);
        message_state = 2;
      }
      else
        message_state = 0;
      break;
    case 2:   //capture byte 2
      capture_byte(msg_byte);
      message_state = 3;
      break;
    case 3:   //capture byte 3
      capture_byte(msg_byte);
      message_state = 4;
      break;
    case 4:   //capture byte 4
      capture_byte(msg_byte);
      message_state = 5;
      msg_OpCode = msg_byte;
      break;
    case 5:   // this is the Length Byte, byte 5. Capture byte 5 and initialize the byte countdown
      length_countdown = msg_byte + 2; //add 2 to length to capture the checksum at the end
      capture_byte(msg_byte);
      message_state = 6;
      break;
    case 6:   //receive packet bytes through the last checksum byte
      capture_byte(msg_byte);
      length_countdown--;
      if (length_countdown <= 0) {
        //        debugger(cp_status_msg);

        message_state = 0;
        toggle_onboard_led_state();
        if (msg_OpCode == 0x02) {
          timer_holdoff_after_status_packet.starttimer(5, not_retriggerable); // wait 5 msec before receiving to reject junk chars from LX3xxx
          serial_msg_binary_index = 0;
          CP_Minutes = serial_msg_binary[6];
          CP_Hours = serial_msg_binary[7];
          compool_devices_state = serial_msg_binary[8]; // capture Byte 8 which contains the Primary Equipment state
          Service_Mode = (serial_msg_binary[9] & 0x01) != 0 ? true : false; //update Service Mode from secondary equipment byte
          Heater_State = (serial_msg_binary[9] & 0x02) != 0 ? true : false; //update Heater State from secondary equipment byte
          Solar_State = (serial_msg_binary[9] & 0x04) != 0 ? true : false; //update Solar State from secondary equipment byte
          Pool_Heatmode = (serial_msg_binary[10] >> 4) & 0x3; // comment out to ----disable_heatmode----
          Spa_Heatmode = (serial_msg_binary[10] >> 6) & 0x3; // comment out to ----disable_heatmode----
          Water_Temperature = convert_c2f(serial_msg_binary[11], 4);
          Solar_Temperature = convert_c2f(serial_msg_binary[12], 2);
          Spa_Water_Temp = convert_c2f(serial_msg_binary[13], 4);
          Spa_Solar_Temp = convert_c2f(serial_msg_binary[14], 2); // protocol doc says only CP3830 does this
          Desired_Pool_Temp = convert_c2f(serial_msg_binary[15], 4);
          Desired_Spa_Temp = convert_c2f(serial_msg_binary[16], 4);
          Air_Temp = convert_c2f(serial_msg_binary[17], 2);
          Equip_Sensor_Stat = serial_msg_binary[20];

          if (server_response_state == resp_state_2) {
            prepare_server_response();
            server.send(200, "text/json", jsonString);
            server_response_state = resp_state_0;
          }
        }
        if ((msg_OpCode == 0x01) && (serial_msg_binary[6] == 0x82)) { // it's an ACK & ACK'ed msg is Basic Ack Packet
          if (server_response_state == resp_state_1) {
            server_response_state = resp_state_2; //received ACK , now wait for Basic Acknowledge Packet
            debugger("[server_response_state] Received ACK, now wait for Basic Acknowledge Packet");
          }
          else {
            debugger("[server_response_state] received ACK, but no ST response queued: " + String(server_response_state));
            server_response_state = resp_state_2; //must be CP system ACK , now wait for Basic Acknowledge Packet
            // something in CP must have changed because of unsolicited ACK, so we should report it to SmartThings
          }
        }
      }
      else {
        if (cp_status_msg.length() > max_mqtt_limit) {
          debugger(cp_status_msg);
          cp_status_msg = ".";
          //capture_timestamp(millis());
        }
      }
      break;
    default:   // should never get here, but if it happens set the message_state to 0
      message_state = 0;
      debugger(String("Compool Message parsing state error"));
  }
}

// AnyDeviceChange() returns true if any of the devices have a change in any parameter.
// This is so the hub can be notified of a state change. Return true if there's a change, false for no change.
//
boolean AnyDeviceChange() {
  boolean retval = false;

  if (pcompool_devices_state != compool_devices_state) {
    debugger("compool_devices_state changed from 0x" + String(pcompool_devices_state) + " to 0x" + String(compool_devices_state));
    retval = true;
  }
  if (pWater_Temperature != Water_Temperature) {
    debugger("Water_Temperature changed from " + String(pWater_Temperature) + " to " + String(Water_Temperature));
    retval = true;
  }
  if (pDesired_Pool_Temp != Desired_Pool_Temp) {
    debugger("Desired_Pool_Temp changed from " + String(pDesired_Pool_Temp) + " to " + String(Desired_Pool_Temp));
    retval = true;
  }
  if (pDesired_Spa_Temp != Desired_Spa_Temp) {
    debugger("Desired_Spa_Temp changed from " + String(pDesired_Spa_Temp) + " to " + String(Desired_Spa_Temp));
    retval = true;
  }
  if (pPool_Heatmode != Pool_Heatmode) {
    debugger("Pool_Heatmode changed from " + String(pPool_Heatmode) + " to " + String(Pool_Heatmode));
    retval = true;
  }
  if (pSpa_Heatmode != Spa_Heatmode) {
    debugger("Spa_Heatmode changed from " + String(pSpa_Heatmode) + " to " + String(Spa_Heatmode));
    retval = true;
  }
  if ((pService_Mode != Service_Mode) || (pHeater_State != Heater_State) || (pSolar_State != Solar_State)) {
    debugger("The indicators changed from: HS=" + String(b2i(pHeater_State)) + " SS=" + String(b2i(pSolar_State)) +
             " SM=" + String(b2i(pService_Mode)) + " to: HS=" + String(b2i(Heater_State)) +
             " SS=" + String(b2i(Solar_State)) + " SM=" + String(b2i(Service_Mode)));
    retval = true;
  }
  pcompool_devices_state = compool_devices_state;
  pWater_Temperature = Water_Temperature;
  pDesired_Pool_Temp = Desired_Pool_Temp;
  pDesired_Spa_Temp = Desired_Spa_Temp;
  pPool_Heatmode = Pool_Heatmode;
  pSpa_Heatmode = Spa_Heatmode;
  pService_Mode = Service_Mode;
  pHeater_State = Heater_State;
  pSolar_State = Solar_State;

  return (retval);
}

void display_keepalive_debugger_stuff() {
  char dmsg[max_mqtt_limit + 5];
  String CP_time_temp;

  sprintf(dmsg, "keepalive%d", keepalive_count);
  if (mqtt_enabled)
    client.publish("bitbucket", dmsg);

  if (CP_Minutes < 10)
    CP_time_temp = CP_Hours + String(":0") + CP_Minutes;
  else
    CP_time_temp = CP_Hours + String(":") + CP_Minutes;
  debugger(String("dbg") + keepalive_count++ + String(", Pri: 0x") + String(compool_devices_state, HEX) +
           String(", Water T=") + Water_Temperature + String("F, Solar T=") + Solar_Temperature +
           String("F, Pool time: ") + CP_time_temp + String(", free=") + ESP.getFreeHeap() + String(", mqtt=") +
           String(mqtt_enabled));
  keepalive_count &= 0x7;
}

void setup()
{
  initialize_gpio_pins();
  setserial_receivemode();
  onboard_led_off();
  debug_led_off();
  CPtimehasbeenset = false;

  jsonString = buildJson();
  Serial.begin(9600);     // opens serial port, sets data rate to 9600 bps
  //Serial.swap();  // Serial is remapped to GPIO15 (TX) and GPIO13 (RX) by calling Serial.swap() after Serial.begin
  setup_wifi();
  if (mqtt_server[0] == '\0') // if mqtt_server is a null string then disable mqtt
    mqtt_enabled = false;
  else
    mqtt_enabled = true;


  /***********************************arduino setup*************/
  /********************************arduinoOTA*/
  ArduinoOTA.onStart([]() {
    Serial.println("Start updating ");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  int mqtt_port_int = String(mqtt_port).toInt(); // take char array to String and then to int;
  if (mqtt_enabled)
    client.setServer(mqtt_server, mqtt_port_int);

  delay(1000); // for debugging, delay to allow any characters in the UART buffer to be output before the pins are swapped
  Serial.swap();  // Serial is remapped to GPIO15 (TX) and GPIO13 (RX) by calling Serial.swap() after Serial.begin

  timer_intro_message.starttimer((unsigned long) 500, not_retriggerable); // wait 500 msec after setup before putting a welcome banner to debugger topic

  timer_mqtt_keepalive.starttimer(keepalive_interval, retriggerable); // start the mqtt keepalive periodic timer

  serial_msg_binary_index = 0;
  message_state = 0; // initialize message parsing state to parse serial message from Compool
  messagequeued_int = false; // clear serial message queued to transmit

  server_response_state = resp_state_0; // set SmartThings server response state to no response queued

  server.on("/", handleRoot);
  server.on("/on", handleOn);
  server.on("/off", handleOff);
  server.on("/settemp", handleSetTemp);
  server.on("/setheatmode", handleHeatMode);
  server.on("/subscribe", handleSubscribe);
  server.on("/poll", handlePoll);
  server.on("/json", handleJson);
#ifdef html_debug
  server.on("/debug", handleWebsite);
  server.on("/xml", handleXML);
  server.on("/reset", handleReset);
  server.on("/softreset", handleSoftReset);
  server.on("/color", handleColor);
#endif

  server.on("/esp8266.xml", HTTP_GET, []() {
    SSDP.schema(server.client());
  });

  server.onNotFound(handleNotFound);

  server.begin();

  SSDP.setSchemaURL("esp8266.xml");
  SSDP.setHTTPPort(80);
  //SSDP.setName("ESP8266 Basic Switch");
  SSDP.setName("Compool");
  SSDP.setSerialNumber("0112358132134");
  SSDP.setURL("poll");
  SSDP.setModelName("Compool Bridge");
  SSDP.setModelNumber("1");
  SSDP.setModelURL("https://github.com/SchreiverJ/SmartThingsESP8266/");
  SSDP.setManufacturer("Jacob Schreiver");
  SSDP.setManufacturerURL("https://github.com/SchreiverJ/SmartThingsESP8266/");
//  SSDP.setDeviceType("urn:schemas-upnp-org:device:Compool:1");
    SSDP.setDeviceType("urn:schemas-upnp-org:device:Compool:1");
  SSDP.begin();
}

void loop()
{
  ArduinoOTA.handle();
  char dmsg[max_mqtt_limit + 5];
  //String sdmsg; //memory leak testing
  int incomingByte = 0;   // for incoming serial data
  IPAddress ip_ofESP;
  unsigned long ip_long;
  int i;

  if (!client.connected() && mqtt_enabled) {
    reconnect();
  }
  if (mqtt_enabled)
    client.loop();

  // buildjson();
  server.handleClient();
  // delay(10);

  // Capture any serial data from Compool if it's available, and send data only when you receive data.
  if (Serial.available() > 0) {
    // when any packet arrives from Compool, prevent transmission of a packet until the entire message is received
    clear_safe_after_serial_msg();
    // read the incoming byte
    incomingByte = Serial.read();
    //debugger(String(incomingByte,HEX)); // statement used to monitor the serial data
    parse_message(incomingByte); // parse the Compool packet
  }

  // Prevent transmission for ~5 msec after a packet is received.
  // This is to ignore junk characters following the Basic Status Packet after the Compool LX3xxx changes
  // its RS-485 transceiver from transmit back to receive mode. Otherwise, the junk characters will fool us
  // into thinking that a Basic Status Packet is being received, which will perform clear_safe_after_serial_msg();
  if (timer_holdoff_after_status_packet.processtimer()) {
    start_safe_after_serial_msg();
  }

  // prevent transmission just before the next 2.6 sec status message arrives
  if (timer_safe_after_serial_msg.processtimer()) {
    flag_safe_after_serial_msg = false;
    //debugger(String("safe serial timer expired, flag_safe_after_serial_msg = false;"));
    digitalWrite(safe_serial_gpioPin, LOW);  // Set the safe_serial_gpioPin LOW, for debugging
  }

  // timer to control the driver & receiver enables of the MAX485
  if (timer_serial_msg_tx.processtimer()) {
    setserial_receivemode();
  }

  // an 8 second periodic ping, both helpful to know the debugger is working, and also useful for mqtt keepalive
  if (timer_mqtt_keepalive.processtimer()) { // a recurring timer event, every 8 seconds, using the value "keepalive_interval"
    //debugger(String("Free=") + ESP.getFreeHeap());
    display_keepalive_debugger_stuff();
    // The following statement is an easy way to test the debug led. Normally comment this out.
    //toggle_debug_led_state();
  }

  // display the intro messages after things are initialized completely
  String sourcename;
  if (timer_intro_message.processtimer()) {
    debugger(String("Compool Bridge starting..."));
    eventlog(String("Compool Bridge starting..."));
    sourcename = String(__FILE__);
    debugger(sourcename.substring(sourcename.lastIndexOf("Compool")));
    debugger(String("Connected to ") + String(WiFi.SSID()));
    ip_ofESP = WiFi.localIP();
    ip_long = ip_ofESP;
    debugger(String("WiFi connected, IP address: ") + String(ip_long & 0xff) + "." + String((ip_long >> 8) & 0xff)
             + "." + String((ip_long >> 16) & 0xff) + "." + String((ip_long >> 24) & 0xff));
    debugger(String("mqtt_server: ") + String(mqtt_server));
    debugger(String("mqtt_port: ") + String(mqtt_port));
    if (mqtt_enabled) debugger(String("MQTT is enabled"));
    else debugger(String("MQTT is disabled"));
    debugger(String("cp_time_zone: ") + String(cp_time_zone));
    debugger(String("cp_auto_dst: ") + String(cp_auto_dst) + String(", Auto DST=") + String(dstenable));
    initializeNTP();
  }

  // If a Compool message is queued and it is safe to send it, then send message over serial port to Compool
  //if(messagequeued_int){ // debug version, don't check flag_safe_after_serial_msg
  if (messagequeued_int && flag_safe_after_serial_msg) {
    clear_safe_after_serial_msg();
    messagequeued_int = false;
    debugger(String("Output to the Serial Port, int"));
    sdmsg = "";
    for (i = 0; i < baseintmessagelength; i++) {
      sprintf(dmsg, "%2x ", messagebuffer_int[i]);
      sdmsg = sdmsg + String(dmsg);
    }
    debugger(sdmsg);
    setserial_transmitmode(baseintmessagelength); // set the MAX485 transmit mode for the proper time, ~1 millisecond per char at 9600 baud
    delay(2);
    Serial.write(messagebuffer_int, baseintmessagelength); // send the message to the UART to be sent to Compool
  }

  processNTP();
  process_setclock();

  // detect changes in any of the devices
  if (AnyDeviceChange()) {
    prepare_server_response();
    HTTPClient http;
    http.begin("http://" + hubAddress + "/notify");
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    http.POST(jsonString);
    //http.writeToStream(&Serial);
    http.end();
  }
}
