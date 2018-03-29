/**
 *  Irrigation Controller 8 Zones
 *  
 *	Creates connected irrigation controller
 *  Author: Brian Findley
 *  Date: 2017-6-12
 *
 */
 
 // for the UI
preferences {
    input("zip", "text", title: "Zip Code", description: "Zip Code for Weather", required: false, defaultValue: "")
    input("oneTimer", "text", title: "Zone One", description: "Zone One Time", required: false, defaultValue: "10")
    input("twoTimer", "text", title: "Zone Two", description: "Zone Two Time", required: false, defaultValue: "10")
    input("threeTimer", "text", title: "Zone Three", description: "Zone Three Time", required: false, defaultValue: "10")
    input("fourTimer", "text", title: "Zone Four", description: "Zone Four Time", required: false, defaultValue: "10")
    input("fiveTimer", "text", title: "Zone Five", description: "Zone Five Time", required: false, defaultValue: "10")
    input("sixTimer", "text", title: "Zone Six", description: "Zone Six Time", required: false, defaultValue: "10")
    input("sevenTimer", "text", title: "Zone Seven", description: "Zone Seven Time", required: false, defaultValue: "10")
    input("eightTimer", "text", title: "Zone Eight", description: "Zone Eight Time", required: false, defaultValue: "10")
    input name: "onename", type: "text", title: "One Name", description: "Enter Text"
    input name: "twoname", type: "text", title: "Two Name", description: "Enter Text"
    input name: "threename", type: "text", title: "Three Name", description: "Enter Text"
    input name: "fourname", type: "text", title: "Four Name", description: "Enter Text"
    input name: "fivename", type: "text", title: "Five Name", description: "Enter Text"
    input name: "sixname", type: "text", title: "Six Name", description: "Enter Text"
    input name: "sevenname", type: "text", title: "Seven Name", description: "Enter Text"
    input name: "eightname", type: "text", title: "Eight Name", description: "Enter Text"
}

import groovy.json.JsonSlurper

metadata {
    definition (name: "Sprinkler Controller with Display", version: "1.0", author: "Brian Findley", namespace: "bfindley") {
        
        capability "Switch"
        capability "Momentary"
        capability "Refresh"
        command "OnWithZoneTimes"
        command "RelayOn1"
        command "RelayOn1For"
        command "RelayOff1"
        command "RelayOn2"
        command "RelayOn2For"
        command "RelayOff2"
        command "RelayOn3"
        command "RelayOn3For"
        command "RelayOff3"
        command "RelayOn4"
        command "RelayOn4For"
        command "RelayOff4"
        command "RelayOn5"
        command "RelayOn5For"
        command "RelayOff5"
        command "RelayOn6"
        command "RelayOn6For"
        command "RelayOff6"
        command "RelayOn7"
        command "RelayOn7For"
        command "RelayOff7"
        command "RelayOn8"
        command "RelayOn8For"
        command "RelayOff8"
        command "rainDelayed"
        command "update" 
        command "enablePump"
        command "disablePump"
        command "onPump"
        command "offPump"
        command "noEffect"
        command "skip"
        command "expedite"
        command "onHold"
        command "warning"
        command "refresh"
        attribute "effect", "string"
    }

    simulator {
          }
    
    tiles(scale: 2){
        standardTile("allZonesTile", "device.switch", width: 2, height: 2, canChangeIcon: true, canChangeBackground: true) {
            state "off", label: 'Start', action: "switch.on", icon: "st.samsung.da.RC_ic_power", backgroundColor: "#ffffff", nextState: "starting"
            state "on", label: 'Running', action: "switch.off", icon: "st.Health & Wellness.health7", backgroundColor: "#53a7c0", nextState: "stopping"
            state "starting", label: 'Starting...', action: "switch.off", icon: "st.Health & Wellness.health7", backgroundColor: "#53a7c0"
            state "stopping", label: 'Stopping...', action: "switch.off", icon: "st.Health & Wellness.health7", backgroundColor: "#53a7c0"
            state "rainDelayed", label: 'Rain Delay', action: "switch.off", icon: "st.Weather.weather10", backgroundColor: "#fff000", nextState: "off"
        	state "warning", label: 'Issue',  icon: "st.Health & Wellness.health7", backgroundColor: "#ff000f", nextState: "off"
        }
        standardTile("zoneOneTile", "device.1", width: 2, height: 2, canChangeIcon: true, canChangeBackground: true) {
            state "off", label: 'One', action: "RelayOn1", icon: "st.Outdoor.outdoor12", backgroundColor: "#ffffff",nextState: "sending"
            state "sending", label: 'sending', action: "RelayOff1", icon: "st.Health & Wellness.health7", backgroundColor: "#cccccc"
            state "q", label: 'One', action: "RelayOff1",icon: "st.Outdoor.outdoor12", backgroundColor: "#c0a353", nextState: "sending"
            state "on", label: 'One', action: "RelayOff1",icon: "st.Outdoor.outdoor12", backgroundColor: "#53a7c0", nextState: "sending"
        }
        standardTile("zoneTwoTile", "device.2", width: 2, height: 2, canChangeIcon: true, canChangeBackground: true) {
            state "off", label: 'Two', action: "RelayOn2", icon: "st.Outdoor.outdoor12", backgroundColor: "#ffffff", nextState: "sending"
            state "sending", label: 'sending', action: "RelayOff2", icon: "st.Health & Wellness.health7", backgroundColor: "#cccccc"
            state "q", label: 'Two', action: "RelayOff2",icon: "st.Outdoor.outdoor12", backgroundColor: "#c0a353", nextState: "sending"
            state "on", label: 'Two', action: "RelayOff2",icon: "st.Outdoor.outdoor12", backgroundColor: "#53a7c0", nextState: "sending"
        }
        standardTile("zoneThreeTile", "device.3", width: 2, height: 2, canChangeIcon: true, canChangeBackground: true) {
            state "off", label: 'Three', action: "RelayOn3", icon: "st.Outdoor.outdoor12", backgroundColor: "#ffffff", nextState: "sending"
            state "sending", label: 'sending', action: "RelayOff3", icon: "st.Health & Wellness.health7", backgroundColor: "#cccccc"
            state "q", label: 'Three', action: "RelayOff3",icon: "st.Outdoor.outdoor12", backgroundColor: "#c0a353", nextState: "sending"
            state "on", label: 'Three', action: "RelayOff3",icon: "st.Outdoor.outdoor12", backgroundColor: "#53a7c0", nextState: "sending"
        }
        standardTile("zoneFourTile", "device.4", width: 2, height: 2, canChangeIcon: true, canChangeBackground: true) {
            state "off", label: 'Four', action: "RelayOn4", icon: "st.Outdoor.outdoor12", backgroundColor: "#ffffff", nextState: "sending"
            state "sending", label: 'sending', action: "RelayOff4", icon: "st.Health & Wellness.health7", backgroundColor: "#cccccc"
            state "q", label: 'Four', action: "RelayOff4",icon: "st.Outdoor.outdoor12", backgroundColor: "#c0a353", nextState: "sending"
            state "on", label: 'Four', action: "RelayOff4",icon: "st.Outdoor.outdoor12", backgroundColor: "#53a7c0", nextState: "sending"
        }
        standardTile("zoneFiveTile", "device.5", width: 2, height: 2, canChangeIcon: true, canChangeBackground: true) {
            state "off", label: 'Five', action: "RelayOn5", icon: "st.Outdoor.outdoor12", backgroundColor: "#ffffff", nextState: "sending"
            state "sending", label: 'sending', action: "RelayOff5", icon: "st.Health & Wellness.health7", backgroundColor: "#cccccc"
            state "q", label: 'Five', action: "RelayOff5",icon: "st.Outdoor.outdoor12", backgroundColor: "#c0a353", nextState: "sending"
            state "on", label: 'Five', action: "RelayOff5",icon: "st.Outdoor.outdoor12", backgroundColor: "#53a7c0", nextState: "sending"
        }
        standardTile("zoneSixTile", "device.6", width: 2, height: 2, canChangeIcon: true, canChangeBackground: true) {
            state "off", label: 'Six', action: "RelayOn6", icon: "st.Outdoor.outdoor12", backgroundColor: "#ffffff", nextState: "sending"
            state "sending", label: 'sending', action: "RelayOff6", icon: "st.Health & Wellness.health7", backgroundColor: "#cccccc"
            state "q", label: 'Six', action: "RelayOff6",icon: "st.Outdoor.outdoor12", backgroundColor: "#c0a353", nextState: "sending"
            state "on", label: 'Six', action: "RelayOff6",icon: "st.Outdoor.outdoor12", backgroundColor: "#53a7c0", nextState: "sending"
        }
        standardTile("zoneSevenTile", "device.7", width: 2, height: 2, canChangeIcon: true, canChangeBackground: true) {
            state "off", label: 'Seven', action: "RelayOn7", icon: "st.Outdoor.outdoor12", backgroundColor: "#ffffff", nextState: "sending"
            state "sending", label: 'sending', action: "RelayOff7", icon: "st.Health & Wellness.health7", backgroundColor: "#cccccc"
            state "q", label: 'Seven', action: "RelayOff7",icon: "st.Outdoor.outdoor12", backgroundColor: "#c0a353", nextState: "sending"
            state "on", label: 'Seven', action: "RelayOff7",icon: "st.Outdoor.outdoor12", backgroundColor: "#53a7c0", nextState: "sending"
        }
        standardTile("zoneEightTile", "device.8", width: 2, height: 2, canChangeIcon: true, canChangeBackground: true) {
            state "off", label: 'Eight', action: "RelayOn8", icon: "st.Outdoor.outdoor12", backgroundColor: "#ffffff", nextState: "sending"
            state "sending", label: 'sending', action: "RelayOff8", icon: "st.Health & Wellness.health7", backgroundColor: "#cccccc"
            state "q", label: 'Eight', action: "RelayOff8",icon: "st.Outdoor.outdoor12", backgroundColor: "#c0a353", nextState: "sending"
            state "on", label: 'Eight', action: "RelayOff8",icon: "st.Outdoor.outdoor12", backgroundColor: "#53a7c0", nextState: "sending"
            state "havePump", label: 'Eight', action: "disablePump", icon: "st.custom.buttons.subtract-icon", backgroundColor: "#ffffff"

        }
        standardTile("pumpTile", "device.pump", width: 2, height: 2, canChangeIcon: true, canChangeBackground: true) {
            state "noPump", label: 'Pump', action: "enablePump", icon: "st.custom.buttons.subtract-icon", backgroundColor: "#ffffff",nextState: "enablingPump"
         	state "offPump", label: 'Pump', action: "onPump", icon: "st.valves.water.closed", backgroundColor: "#ffffff", nextState: "sendingPump"
           	state "enablingPump", label: 'sending', action: "disablePump", icon: "st.Health & Wellness.health7", backgroundColor: "#cccccc"
            state "disablingPump", label: 'sending', action: "disablePump", icon: "st.Health & Wellness.health7", backgroundColor: "#cccccc"
            state "onPump", label: 'Pump', action: "offPump",icon: "st.valves.water.open", backgroundColor: "#53a7c0", nextState: "sendingPump"
            state "sendingPump", label: 'sending', action: "offPump", icon: "st.Health & Wellness.health7", backgroundColor: "#cccccc"
        }
        	
        standardTile("refreshTile", "device.switch", width: 2, height: 2, decoration: "flat") {
            state "ok", label: "", action: "refresh.refresh", icon: "st.secondary.refresh", backgroundColor: "#ffffff"
        }
        standardTile("scheduleEffect", "device.effect", width: 2, height: 2) {
            state("noEffect", label: "Normal", action: "skip", icon: "st.Office.office7", backgroundColor: "#ffffff")
            state("skip", label: "Skip 1X", action: "expedite", icon: "st.Office.office7", backgroundColor: "#c0a353")
            state("expedite", label: "Expedite", action: "onHold", icon: "st.Office.office7", backgroundColor: "#53a7c0")
            state("onHold", label: "Pause", action: "noEffect", icon: "st.Office.office7", backgroundColor: "#bc2323")
        }
    	valueTile("onename", "onename", inactiveLabel: false, height: 1, width: 2, decoration: "flat") {
            state "default", label:'${currentValue}'
		}
    	valueTile("twoname", "twoname", inactiveLabel: false, height: 1, width: 2, decoration: "flat") {
            state "default", label:'${currentValue}'
		}
    	valueTile("threename", "threename", inactiveLabel: false, height: 1, width: 2, decoration: "flat") {
            state "default", label:'${currentValue}'
		}
    	valueTile("fourname", "fourname", inactiveLabel: false, height: 1, width: 2, decoration: "flat") {
            state "default", label:'${currentValue}'
		}
    	valueTile("fivename", "fivename", inactiveLabel: false, height: 1, width: 2, decoration: "flat") {
            state "default", label:'${currentValue}'
		}
    	valueTile("sixname", "sixname", inactiveLabel: false, height: 1, width: 2, decoration: "flat") {
            state "default", label:'${currentValue}'
		}
    	valueTile("sevenname", "sevenname", inactiveLabel: false, height: 1, width: 2, decoration: "flat") {
            state "default", label:'${currentValue}'
		}
    	valueTile("eightname", "eightname", inactiveLabel: false, height: 1, width: 2, decoration: "flat") {
            state "default", label:'${currentValue}'
		}

    
        main "allZonesTile"
        details([ "scheduleEffect", "allZonesTile","refreshTile","zoneOneTile","zoneTwoTile","zoneThreeTile","onename","twoname", "threename", "zoneFourTile","zoneFiveTile","zoneSixTile","fourname", "fivename","sixname",
        "zoneSevenTile","zoneEightTile", "pumpTile","sevenname","eightname" ])
    }
}


// parse events into attributes
def parse(description) {
log.debug "Parsing"
 //   log.debug "Parsing '${description}'"
    def msg = parseLanMessage(description)
    def headerString = msg.header
if (headerString?.contains("/notify")){
log.debug "/notify received"
}
if (headerString?.contains("SID: uuid:")) {
        def sid = (headerString =~ /SID: uuid:.*/) ? ( headerString =~ /SID: uuid:.*/)[0] : "0"
        sid -= "SID: uuid:".trim()

        updateDataValue("subscriptionId", sid)
 	}

    def result = []
    def bodyString = msg.body
    //log.debug msg
	log.debug bodyString
    
    if (bodyString) {
    	unschedule("setOffline")
        def slurper = new JsonSlurper()
        def body = slurper.parseText(bodyString)
        //log.debug body
        //def value = body.text() == "off" ? "off" : "on"
        //def value = body.text()
        //log.trace "Notify: BinaryState = ${value}, ${body.property.BinaryState}"
     	//def eventlist = []
     	log.debug "Run Time: " + body["debugging"]["runseconds"]
		def debugmodes = "Received: "
		['1', '2', '3', '4', '5', '6', '7', '8'].each { deviceName ->
			log.debug "CurrentValue: " + device.currentValue(deviceName);
			debugmodes += "[${deviceName},${body[deviceName]["mode"]}],"
 			def newMode = body[deviceName]["mode"]
        	def isDisplayed = true
        	def isPhysical = true
			if (newMode == "q") {
				isDisplayed = false
         		isPhysical = false
        	}
            if (device.currentValue(deviceName) != newMode){
            	log.debug "sendEvent"
				sendEvent(name: deviceName, value: newMode, descriptionText: "${deviceName} turned ${newMode}", displayed: isDisplayed, isPhysical: isPhysical)
            }
		}
		log.debug debugmodes
	 	if(anyZoneOn()) {
        //manages the state of the overall system.  Overall state is "on" if any zone is on
        //set displayed to false; does not need to be logged in mobile app
        if(device?.currentValue("switch") != "on") {
        	 sendEvent(name: "switch", value: "on", descriptionText: "Irrigation System Is On", displayed: false)  //displayed default is false to minimize logging
        }
     	} else if (device?.currentValue("switch") != "rainDelayed") {
         	if(device?.currentValue("switch") != "off") {
        		sendEvent(name: "switch", value: "off", descriptionText: "Irrigation System Is Off", displayed: false)  //displayed default is false to minimize logging
       	 	}	
     	}
 	}
    result
}


def anyZoneOn() {
    if(device?.currentValue("1") in ["on","q"]) return true;
    if(device?.currentValue("2") in ["on","q"]) return true;
    if(device?.currentValue("3") in ["on","q"]) return true;
    if(device?.currentValue("4") in ["on","q"]) return true;
    if(device?.currentValue("5") in ["on","q"]) return true;
    if(device?.currentValue("6") in ["on","q"]) return true;
    if(device?.currentValue("7") in ["on","q"]) return true;
    if(device?.currentValue("8") in ["on","q"]) return true;

    false;
}
def handleoff(String compdevice){
log.debug "Executing 'off' for device: " + compdevice
    def result = new physicalgraph.device.HubAction(
        method: "GET",
        path: "/off",
        headers: [
            HOST: getHostAddress(),
            'Content-Type': 'application/json'
        ],
        query: [device: compdevice]
    )
    sendHubCommand(result)
//    log.debug result
//    return result
}


def handleon(String zone, int timer){
	log.debug "Executing 'on' for device: ${zone}, timer=${timer}"
	if (timer && timer !=0){
    	def result = new physicalgraph.device.HubAction(
        	method: "GET",
        	path: "/on",
        	headers: [
            	HOST: getHostAddress(),
            	'Content-Type': 'application/json'
        	],
        	query: [device1: zone,
        	time1: timer]
    	)
    	sendHubCommand(result)
		//log.debug result
    }
}

// handle commands
def RelayOn1() {
    log.info "Executing 'on,1'"
//    zigbee.smartShield(text: "on,1,${oneTimer}").format()
	handleon("1", oneTimer.toInteger())
}

def RelayOn1For(value) {
    value = checkTime(value)
    log.info "Executing 'on,1,$value'"
//    zigbee.smartShield(text: "on,1,${value}").format()
	handleon("1", value.toInteger())
}

def RelayOff1() {
    log.info "Executing 'off,1'"
//    zigbee.smartShield(text: "off,1").format()
	handleoff("1")
}

def RelayOn2() {
    log.info "Executing 'on,2'"
	handleon("2", twoTimer.toInteger())
}

def RelayOn2For(value) {
    value = checkTime(value)
    log.info "Executing 'on,2,$value'"
	handleon("2", value.toInteger())
}

def RelayOff2() {
    log.info "Executing 'off,2'"
	handleoff("2")
}

def RelayOn3() {
    log.info "Executing 'on,3'"
	handleon("3", threeTimer.toInteger())
}

def RelayOn3For(value) {
    value = checkTime(value)
    log.info "Executing 'on,3,$value'"
	handleon("3", value.toInteger())
}

def RelayOff3() {
    log.info "Executing 'off,3'"
	handleoff("3")
}

def RelayOn4() {
    log.info "Executing 'on,4'"
	handleon("4", fourTimer.toInteger())
}

def RelayOn4For(value) {
    value = checkTime(value)
    log.info "Executing 'on,4,$value'"
	handleon("4", value.toInteger())
}

def RelayOff4() {
    log.info "Executing 'off,4'"
	handleoff("4")
}

def RelayOn5() {
    log.info "Executing 'on,5'"
	handleon("5", fiveTimer.toInteger())
}

def RelayOn5For(value) {
    value = checkTime(value)
    log.info "Executing 'on,5,$value'"
	handleon("5", value.toInteger())
}

def RelayOff5() {
    log.info "Executing 'off,5'"
	handleoff("5")
}

def RelayOn6() {
    log.info "Executing 'on,6'"
	handleon("6", sixTimer.toInteger())
}

def RelayOn6For(value) {
    value = checkTime(value)
    log.info "Executing 'on,6,$value'"
	handleon("6", value.toInteger())
}

def RelayOff6() {
    log.info "Executing 'off,6'"
	handleoff("6")
}

def RelayOn7() {
    log.info "Executing 'on,7'"
	handleon("7", sevenTimer.toInteger())
}

def RelayOn7For(value) {
    value = checkTime(value)
    log.info "Executing 'on,7,$value'"
	handleon("7", value.toInteger())
}

def RelayOff7() {
    log.info "Executing 'off,7'"
	handleoff("7")
}

def RelayOn8() {
    log.info "Executing 'on,8'"
	handleon("8", eightTimer.toInteger())
}

def RelayOn8For(value) {
    value = checkTime(value)
    log.info "Executing 'on,8,$value'"
	handleon("8", value.toInteger())
}

def RelayOff8() {
    log.info "Executing 'off,8'"
	handleoff("8")
}

private subscribeAction(path, callbackPath="") {
    log.trace "subscribe($path, $callbackPath)"
    def address = getCallBackAddress() // gets the hub address : port
    def ip = getHostAddress() // gets the device address

    def result = new physicalgraph.device.HubAction(
        method: "SUBSCRIBE",
        path: path,
        headers: [
            HOST: ip,
            CALLBACK: "<http://${address}/notify$callbackPath>",
            NT: "upnp:event",
            TIMEOUT: "Second-28800"
        ],
             query: [address: address]
  
    )
//    log.debug result
//    log.trace "SUBSCRIBE $path"

    return result
}

private unsubscribeAction(path, callbackPath="") {
    log.trace "unsubscribe($path, $callbackPath)"
    def address = getCallBackAddress()
    def ip = getHostAddress()

    def result = new physicalgraph.device.HubAction(
        method: "UNSUBSCRIBE",
        path: path,
        headers: [
            HOST: ip,
            CALLBACK: "<http://${address}/notify$callbackPath>",
            NT: "upnp:event",
            TIMEOUT: "Second-28800"
        ]
    )
//log.debug result
//    log.trace "SUBSCRIBE $path"

    return result
}

def subscribe() {
	subscribeAction("/subscribe")
}


def poll() {
log.debug "Executing 'poll'"
    def forecastWeather = getWeatherFeature("forecast", zip)
def todaysHigh = forecastWeather.forecast.simpleforecast.forecastday.high.fahrenheit.toArray() 
	def icon =  forecastWeather.forecast.simpleforecast.forecastday.icon.toArray() 
    def date =  forecastWeather.forecast.simpleforecast.forecastday.date.weekday_short.toArray()[0] + " " +
    forecastWeather.forecast.simpleforecast.forecastday.date.monthname_short.toArray()[0] + " " +
    forecastWeather.forecast.simpleforecast.forecastday.date.day.toArray()[0]
    def conditions = forecastWeather.forecast.simpleforecast.forecastday.conditions.toArray()[0] 
//if (device.currentValue("currentIP") != "Offline")
//    runIn(30, setOffline)
//log.debug getWeatherFeature("forecast")
log.debug todaysHigh[0]
log.debug icon[0]
log.debug date
log.debug conditions
    def result = new physicalgraph.device.HubAction(
        method: "GET",
        path: "/poll",
        headers: [
            HOST: getHostAddress()
        ],
       	query: [temp: todaysHigh[0],
       	icon: getIconLetter(icon[0]),
            date: date]

    )
//    log.debug result
    sendHubCommand(result)
    subscribe()
  //return result  
 }
private getCallBackAddress() {
 	device.hub.getDataValue("localIP") + ":" + device.hub.getDataValue("localSrvPortTCP")
}

private Integer convertHexToInt(hex) {
 	Integer.parseInt(hex,16)
}

private String convertHexToIP(hex) {
 	[convertHexToInt(hex[0..1]),convertHexToInt(hex[2..3]),convertHexToInt(hex[4..5]),convertHexToInt(hex[6..7])].join(".")
} 
private getHostAddress() {
 	def ip = getDataValue("ip")
 	def port = getDataValue("port")
 	if (!ip || !port) {
 		def parts = device.deviceNetworkId.split(":")
 		if (parts.length == 2) {
 			ip = parts[0]
 			port = parts[1]
 		} else {
 			log.warn "Can't figure out ip and port for device: ${device.id}"
		 }
 	}
 	log.debug "Using ip: ${ip} and port: ${port} for device: ${device.id}"
    if (ip?.contains("."))
    {
 	return ip + ":" + port
    }
    else
    {
 	return convertHexToIP(ip) + ":" + convertHexToInt(port)
    }
}


def on() {
    log.info "Executing 'allOn'"
    def querystring = "?"
    int counter = 1
	[[1, oneTimer], [2, twoTimer], [3, threeTimer], [4, fourTimer], [5, fiveTimer], [6, sixTimer], [7, sevenTimer], [8, eightTimer]].each{
		log.info "Timer: ${it[0]}/${it[1]}"
		if (it[1] && it[1] !="0"){
        querystring += "device${counter}=${it[0]}&time${counter}=${it[1]}&"
        counter++
        }

	}
log.debug querystring
    def result = new physicalgraph.device.HubAction(
        method: "GET",
        path: "/on" + querystring,
        headers: [
            HOST: getHostAddress(),
            'Content-Type': 'application/json'
        ],
//        query: [device1: zone,
//        time1: timer]
    )
    sendHubCommand(result)
    log.debug result



//    zigbee.smartShield(text: "allOn,${oneTimer ?: 0},${twoTimer ?: 0},${threeTimer ?: 0},${fourTimer ?: 0},${fiveTimer ?: 0},${sixTimer ?: 0},${sevenTimer ?: 0},${eightTimer ?: 0}").format()
}

def OnWithZoneTimes(value) {
    log.info "Executing 'allOn' with zone times [$value]"
    def evt = createEvent(name: "switch", value: "starting", displayed: true)
    sendEvent(evt)
    
	def zoneTimes = [:]
    for(z in value.split(",")) {
    	def parts = z.split(":")
        zoneTimes[parts[0].toInteger()] = parts[1]
        log.info("Zone ${parts[0].toInteger()} on for ${parts[1]} minutes")
    }
def querystring = "?"   
int offset = 0
for (int i = 1; i <=8; i++) {
	if (checkTime(zoneTimes[i]) != 0){
	if (i != 1){querystring += "&"}
		querystring += "device${i-offset}=${i}&time${i-offset}=${checkTime(zoneTimes[i]) ?: 0}"
	} else
    {
    offset++
    }
}
log.debug querystring
    def result = new physicalgraph.device.HubAction(
        method: "GET",
        path: "/on" + querystring,
        headers: [
            HOST: getHostAddress(),
            'Content-Type': 'application/json'
        ],
//        query: [device1: zone,
//        time1: timer]
    )
    sendHubCommand(result)
    log.debug result


//    zigbee.smartShield(text: "allOn,${checkTime(zoneTimes[1]) ?: 0},${checkTime(zoneTimes[2]) ?: 0},${checkTime(zoneTimes[3]) ?: 0},${checkTime(zoneTimes[4]) ?: 0},${checkTime(zoneTimes[5]) ?: 0},${checkTime(zoneTimes[6]) ?: 0},${checkTime(zoneTimes[7]) ?: 0},${checkTime(zoneTimes[8]) ?: 0}").format()
}

def off() {
    log.info "Executing 'allOff'"
    def result = new physicalgraph.device.HubAction(
        method: "GET",
        path: "/alloff",
        headers: [
            HOST: getHostAddress(),
            'Content-Type': 'application/json'
        ],
    )
    sendHubCommand(result)
//    log.debug result

//    zigbee.smartShield(text: "allOff").format()
}

def checkTime(t) {
	def time = (t ?: 0).toInteger()
    time > 3600 ? 120 : time
}

def update() {
    log.info "Executing refresh"
    sendEvent([name: 'onename', value: onename ?: '---'])
    sendEvent([name: 'twoname', value: twoname ?: '---'])
    sendEvent([name: 'threename', value: threename ?: '---'])
    sendEvent([name: 'fourname', value: fourname ?: '---'])
    sendEvent([name: 'fivename', value: fivename ?: '---'])
    sendEvent([name: 'sixname', value: sixname ?: '---'])
    sendEvent([name: 'sevenname', value: sevenname ?: '---'])
    sendEvent([name: 'eightname', value: eightname ?: '---'])

//    zigbee.smartShield(text: "update").format()
[
	subscribe(),
	poll()
]
}

def refresh() {
    log.info "Executing refresh"
    sendEvent([name: 'onename', value: onename ?: '---'])
    sendEvent([name: 'twoname', value: twoname ?: '---'])
    sendEvent([name: 'threename', value: threename ?: '---'])
    sendEvent([name: 'fourname', value: fourname ?: '---'])
    sendEvent([name: 'fivename', value: fivename ?: '---'])
    sendEvent([name: 'sixname', value: sixname ?: '---'])
    sendEvent([name: 'sevenname', value: sevenname ?: '---'])
    sendEvent([name: 'eightname', value: eightname ?: '---'])

//    zigbee.smartShield(text: "update").format()
[
	subscribe(),
	poll()
]
}

def rainDelayed() {
    log.info "rain delayed"
    if(device.currentValue("switch") != "on") {
        sendEvent (name:"switch", value:"rainDelayed", displayed: true)
    }
}

def warning() {
    log.info "Warning: Programmed Irrigation Did Not Start"
    if(device.currentValue("switch") != "on") {
        sendEvent (name:"switch", value:"warning", displayed: true)
    }
}

def enablePump() {
    log.info "Enabling Pump"
    zigbee.smartShield(text: "pump,3").format()  //pump is queued and ready to turn on when zone is activated
}
def disablePump() {
    log.info "Disabling Pump"
    zigbee.smartShield(text: "pump,0").format()  //remove pump from system, reactivate Zone8
}
def onPump() {
    log.info "Turning On Pump"
    zigbee.smartShield(text: "pump,2").format()
    }

def offPump() {
	log.info "Turning Off Pump"
    zigbee.smartShield(text: "pump,1").format()  //pump returned to queue state to turn on when zone turns on
        }
def push() {
    log.info "advance to next zone"
    zigbee.smartShield(text: "advance").format()  //turn off currently running zone and advance to next
    }

// commands that over-ride the SmartApp

// skip one scheduled watering
def	skip() {
    def evt = createEvent(name: "effect", value: "skip", displayed: true)
    log.info("Sending: $evt")
    sendEvent(evt)
}
// over-ride rain delay and water even if it rains
def	expedite() {
    def evt = createEvent(name: "effect", value: "expedite", displayed: true)
    log.info("Sending: $evt")
    sendEvent(evt)
}

// schedule operates normally
def	noEffect() {
    def evt = createEvent(name: "effect", value: "noEffect", displayed: true)
    log.info("Sending: $evt")
    def result = new physicalgraph.device.HubAction(
        method: "GET",
        path: "/blink",
        headers: [
            HOST: getHostAddress(),
            'Content-Type': 'application/json'
        ],
        query: [number: 1]

    )
    sendHubCommand(result)
    sendEvent(evt)
}

// turn schedule off indefinitely
def	onHold() {
    def evt = createEvent(name: "effect", value: "onHold", displayed: true)
    log.info("Sending: $evt")
    def result = new physicalgraph.device.HubAction(
        method: "GET",
        path: "/blink",
        headers: [
            HOST: getHostAddress(),
            'Content-Type': 'application/json'
        ],
        query: [number: 2]

    )
    sendHubCommand(result)
    sendEvent(evt)
}

def getIconLetter(String icontext){
log.debug icontext
log.debug icontext.equals("clear")
log.debug icontext == "clear"
def result = ""
switch (icontext) {
    case "chanceflurries":
        result = "F"
		break
	case "chancerain":
        result = "Q";
		break
  case "chancesleet":
        result = "W";
		break
  case "chancesnow":
        result = "V";
		break
  case "chancetstorms":
        result = "S";
		break
  case "clear":
        result = "B";
		break
  case "cloudy":
        result = "Y";
		break
  case "flurries":
        result = "F";
		break
  case "fog":
        result = "M";
		break
  case "hazy":
        result = "E";
		break
  case "mostlycloudy":
        result = "Y";
		break
  case "mostlysunny":
        result = "H";
		break
  case "partlycloudy":
        result = "H";
		break
  case "partlysunny":
        result = "J";
		break
  case "sleet":
        result = "W";
		break
  case "rain":
        result = "R";
		break
  case "snow":
        result = "W";
		break
  case "sunny":
        result = "B";
		break
  case "tstorms":
        result = "0";
		break
 case "nt_chanceflurries":
        result = "F";
		break
  case "nt_chancerain":
        result = "7";
		break
  case "nt_chancesleet":
        result = "#";
		break
  case "nt_chancesnow":
        result = "#";
 case "nt_chancetstorms":
        result = "&";
		break
  case "nt_clear":
        result = "2";
		break
  case "nt_cloudy":
        result = "Y";
		break
  case "nt_flurries":
        result = "9";
		break
  case "nt_fog":
        result = "M";
		break
  case "nt_hazy":
        result = "E";
		break
  case "nt_mostlycloudy":
        result = "5";
		break
  case "nt_mostlysunny":
        result = "3";
		break
  case "nt_partlycloudy":
        result = "4";
		break
  case "nt_partlysunny":
        result = "4";
		break
  case "nt_sleet":
        result = "9";
		break
 case "nt_rain":
        result = "7";
		break
  case "nt_snow":
        result = "#";
		break
  case "nt_sunny":
        result = "4";
		break
  case "nt_tstorms":
        result = "&";
		break
 default:
result = ")";
}
}
