/**
 *  
 *  ESP8266 Compool Bridge
 *  
 *
 * Derived from: 
 * ESP8266 Switch
 *
 * Author: Brian Findley 
 * Date: 04/19/2020
 * Version 19
 */

import groovy.json.JsonSlurper


metadata {
 	definition (name: "ESP8266_Compool_Bridge", namespace: "bfindley", author: "Brian Findley") {
        //capability "Switch"
        capability "Thermostat"
        capability "Temperature Measurement"
        capability "Polling"
        capability "Refresh"
        capability "Sensor"

        // Custom attributes
        attribute "pool", "string" //on/off
        attribute "spa", "string" //on/off
        attribute "aux1", "string" //on/off
       	attribute "aux2", "string" //on/off
     	attribute "aux3", "string" //on/off
       	attribute "aux4", "string" //on/off
       	attribute "aux5", "string" //on/off
		attribute "aux6", "string" //on/off
       	attribute "poolcurrenttemp", "string" 
       	attribute "spacurrenttemp", "string" 
        attribute "servicemode", "string"
        
        attribute "poolsettemp", "string" //temp degrees F
        attribute "spasettemp", "string" //temp degrees F
        attribute "poolheatmode", "string" //solar only, heater, soloar priority, off
        attribute "spaheatmode", "string" //solar only, heater, soloar priority, off
       	attribute "currentIP", "string"
        attribute "aux1name", "string"
        attribute "aux2name", "string"
        attribute "aux3name", "string"
        attribute "aux4name", "string"
        attribute "aux5name", "string"
		attribute "aux6name", "string"
		attribute "temperature" , "string"
 
       // Custom commands
        command "subscribe"
        command "resubscribe"
        command "unsubscribe"
        command "setOffline"
        command "aux1on"
        command "aux1off"
        command "aux2on"
        command "aux2off"
        command "aux3on"
        command "aux3off"
        command "aux4on"
        command "aux4off"
        command "aux5on"
        command "aux5off"
        command "aux6on"
        command "aux6off"
        command "poolon"
        command "pooloff"
        command "spaon"
        command "spaoff"
        command "poll"

		command "spaheatmodeoff"
        command "spaheatmodesolarpriority"
        command "spaheatmodesolaronly"
        command "spaheatmodeheater"

		command "poolheatmodeoff"
        command "poolheatmodesolarpriority"
        command "poolheatmodesolaronly"
        command "poolheatmodeheater"

		command "setpoolsettemp"
        command "setspasettemp"
 }

 // simulator metadata
 simulator {}

preferences {
    input name: "aux1name", type: "text", title: "Aux1 Name", description: "Enter Text"
}
preferences {
    input name: "aux2name", type: "text", title: "Aux2 Name", description: "Enter Text"
}
preferences {
    input name: "aux3name", type: "text", title: "Aux3 Name", description: "Enter Text"
}
preferences {
    input name: "aux4name", type: "text", title: "Aux4 Name", description: "Enter Text"
}
preferences {
    input name: "aux5name", type: "text", title: "Aux5 Name", description: "Enter Text"
}
preferences {
    input name: "aux6name", type: "text", title: "Aux6 Name", description: "Enter Text"
}




 // UI tile definitions
    tiles(scale: 2) {
         standardTile("poolswitch", "device.pool", width: 2, height: 2, canChangeIcon: true) {
            state "on", label:'Pool', action:"pooloff", icon:"st.Health & Wellness.health2", backgroundColor:"#79b821", nextState:"turningOff"
            state "off", label:'Pool', action:"poolon", icon:"st.Health & Wellness.health2", backgroundColor:"#ffffff", nextState:"turningOn"
            state "turningOn", label:'sending', action:"pooloff", icon:"st.Health & Wellness.health2", backgroundColor:"#79b821", nextState:"turningOff"
            state "turningOff", label:'sending', action:"poolon", icon:"st.Health & Wellness.health2", backgroundColor:"#ffffff", nextState:"turningOn"
            state "offline", label:'offline', icon:"st.switches.switch.off", backgroundColor:"#ff0000"
        }
        standardTile("spaswitch", "device.spa", width: 2, height: 2, canChangeIcon: true) {
            state "on", label:'Spa', action:"spaoff", icon:"st.Health & Wellness.health2", backgroundColor:"#79b821", nextState:"aturningOff"
            state "off", label:'Spa', action:"spaon", icon:"st.Health & Wellness.health2", backgroundColor:"#ffffff", nextState:"turningOn"
            state "turningOn", label:'sending', action:"spaoff", icon:"st.Health & Wellness.health2", backgroundColor:"#79b821", nextState:"turningOff"
            state "turningOff", label:'sending', action:"spaon", icon:"st.Health & Wellness.health2", backgroundColor:"#ffffff", nextState:"turningOn"
            state "offline", label:'${name}', icon:"st.Health & Wellness.health2", backgroundColor:"#ff0000"
        }
        standardTile("aux1", "device.aux1", canChangeIcon: false) {
            state "on", label:'aux1', action:"aux1off", icon:"st.samsung.da.RC_ic_power", backgroundColor:"#79b821", nextState:"turningOff"
            state "off", label:'aux1', action:"aux1on", icon:"st.samsung.da.RC_ic_power", backgroundColor:"#ffffff", nextState:"turningOn"
//             state "on", label:'aux1', action:"aux1off", icon:"st.samsung.da.RC_ic_power", backgroundColor:"#79b821"
//            state "off", label:'aux1', action:"aux1on", icon:"st.samsung.da.RC_ic_power", backgroundColor:"#ffffff"
            state "turningOn", label:'sending', action:"aux1off", icon:"st.samsung.da.RC_ic_power", backgroundColor:"#79b821", nextState:"turningOff"
            state "turningOff", label:'sending', action:"aux1on", icon:"st.samsung.da.RC_ic_power", backgroundColor:"#ffffff", nextState:"turningOn"
            state "offline", label:'${name}', icon:"st.samsung.da.RC_ic_power", backgroundColor:"#ff0000"
        }
        standardTile("aux2", "device.aux2", canChangeIcon: false) {
            state "on", label:'aux2', action:"aux2off", icon:"st.samsung.da.RC_ic_power", backgroundColor:"#79b821", nextState:"turningOff"
            state "off", label:'aux2', action:"aux2on", icon:"st.samsung.da.RC_ic_power", backgroundColor:"#ffffff", nextState:"turningOn"
            state "turningOn", label:'sending', action:"aux2off", icon:"st.samsung.da.RC_ic_power", backgroundColor:"#79b821", nextState:"turningOff"
            state "turningOff", label:'sending', action:"aux2on", icon:"st.samsung.da.RC_ic_power", backgroundColor:"#ffffff", nextState:"turningOn"
            state "offline", label:'${name}', icon:"st.samsung.da.RC_ic_power", backgroundColor:"#ff0000"
        }
        standardTile("aux3", "device.aux3", canChangeIcon: false) {
            state "on", label:'aux3', action:"aux3off", icon:"st.samsung.da.RC_ic_power", backgroundColor:"#79b821", nextState:"turningOff"
            state "off", label:'aux3', action:"aux3on", icon:"st.samsung.da.RC_ic_power", backgroundColor:"#ffffff", nextState:"turningOn"
            state "turningOn", label:'sending', action:"aux3off", icon:"st.samsung.da.RC_ic_power", backgroundColor:"#79b821", nextState:"turningOff"
            state "turningOff", label:'sending', action:"aux3on", icon:"st.samsung.da.RC_ic_power", backgroundColor:"#ffffff", nextState:"turningOn"
            state "offline", label:'${name}', icon:"st.samsung.da.RC_ic_power", backgroundColor:"#ff0000"
        }
        standardTile("aux4", "device.aux4", canChangeIcon: false) {
            state "on", label:'aux4', action:"aux4off", icon:"st.samsung.da.RC_ic_power", backgroundColor:"#79b821", nextState:"turningOff"
            state "off", label:'aux4', action:"aux4on", icon:"st.samsung.da.RC_ic_power", backgroundColor:"#ffffff", nextState:"turningOn"
            state "turningOn", label:'sending', action:"aux4off", icon:"st.samsung.da.RC_ic_power", backgroundColor:"#79b821", nextState:"turningOff"
            state "turningOff", label:'sending', action:"aux4on", icon:"st.samsung.da.RC_ic_power", backgroundColor:"#ffffff", nextState:"turningOn"
            state "offline", label:'aux4${name}', icon:"st.samsung.da.RC_ic_power", backgroundColor:"#ff0000"
        }
        standardTile("aux5", "device.aux5", canChangeIcon: false) {
            state "on", label:'aux5', action:"aux5off", icon:"st.samsung.da.RC_ic_power", backgroundColor:"#79b821", nextState:"turningOff"
            state "off", label:'aux5', action:"aux5on", icon:"st.samsung.da.RC_ic_power", backgroundColor:"#ffffff", nextState:"turningOn"
            state "turningOn", label:'sending', action:"aux5off", icon:"st.samsung.da.RC_ic_power", backgroundColor:"#79b821", nextState:"turningOff"
            state "turningOff", label:'sending', action:"aux5on", icon:"st.samsung.da.RC_ic_power", backgroundColor:"#ffffff", nextState:"turningOn"
            state "offline", label:'${name}', icon:"st.samsung.da.RC_ic_power", backgroundColor:"#ff0000"
        }
        standardTile("aux6", "device.aux6", canChangeIcon: false) {
            state "on", label:'aux6', action:"aux6off", icon:"st.samsung.da.RC_ic_power", backgroundColor:"#79b821", nextState:"turningOff"
            state "off", label:'aux6', action:"aux6on", icon:"st.samsung.da.RC_ic_power", backgroundColor:"#ffffff", nextState:"turningOn"
            state "turningOn", label:'sending', action:"aux6off", icon:"st.samsung.da.RC_ic_power", backgroundColor:"#79b821", nextState:"turningOff"
            state "turningOff", label:'sending', action:"aux6on", icon:"st.samsung.da.RC_ic_power", backgroundColor:"#ffffff", nextState:"turningOn"
            state "offline", label:'${name}', icon:"st.samsung.da.RC_ic_power", backgroundColor:"#ff0000"
        }
        standardTile("refresh", "device.switch", inactiveLabel: false, height: 1, width: 1, decoration: "flat") {
            state "default", label:"Refresh", action:"refresh.refresh", icon:"st.secondary.refresh-icon"
        }
        standardTile("subscribe", "device.switch", inactiveLabel: false, height: 1, width: 1, decoration: "flat") {
            state "default", label:"Refresh", action:"subscribe", icon:"st.secondary.refresh-icon"
        }
//       valueTile("poolcurrenttemp", "device.poolcurrenttemp", inactiveLabel: false, height: 1, width: 2, decoration: "flat") {
       valueTile("poolcurrenttemp", "device.temperature", inactiveLabel: false, height: 1, width: 2, decoration: "flat") {
            state "default", label:'${currentValue}°', unit:"F"
		}
//       valueTile("poolcurrenttemp2", "device.poolcurrenttemp", inactiveLabel: false, height: 1, width: 2, decoration: "flat") {
       valueTile("poolcurrenttemp2", "device.temperature", inactiveLabel: false, height: 1, width: 2, decoration: "flat") {
            state "default", label:'${currentValue}°', unit:"F", icon:"st.Health & Wellness.health2", backgroundColor:"#79b821"
		}
      valueTile("lights", "lights", inactiveLabel: false, height: 1, width: 2, decoration: "flat") {
            state "default", label:'${currentValue}'
		}
       valueTile("poolsettemp", "poolsettemp", inactiveLabel: false, height: 1, width: 3, decoration: "flat") {
            state "default", label:'${currentValue}°', unit:"F"
		}
       valueTile("blank", "blank", inactiveLabel: false, height: 1, width: 1, decoration: "flat") {
            state "default", label:'', unit:"F"
		}
       valueTile("spasettemp", "spasettemp", inactiveLabel: false, height: 1, width: 3, decoration: "flat") {
            state "default", label:'${currentValue}°', unit:"F"
		}
       standardTile("poolheatmode", "poolheatmode", inactiveLabel: false, height: 1, width: 3, decoration: "flat") {
            state "off", label:'Off', action:"poolheatmodeheater", nextState:"heater"
            state "heater", label:'Heater', action:"poolheatmodesolarpriority", nextState:"solarpriority"
            state "solarpriority", label:'Solar Priority', action:"poolheatmodesolaronly", nextState:"solaronly"
            state "solaronly", label:'Solar Only', action:"poolheatmodeoff", nextState:"off"
		}
       standardTile("spaheatmode", "spaheatmode", inactiveLabel: false, height: 1, width: 3, decoration: "flat") {
            state "off", label:'Off', action:"spaheatmodeheater", nextState:"heater"
            state "heater", label:'Heater', action:"spaheatmodesolarpriority", nextState:"solarpriority"
            state "solarpriority", label:'Solar Priority', action:"spaheatmodesolaronly", nextState:"solaronly"
            state "solaronly", label:'Solar Only', action:"spaheatmodeoff", nextState:"off"
		}
	controlTile("poolSliderControl", "poolsettemp", "slider", height: 1,
             width: 3, inactiveLabel: false, range:"(75..90)") {
    		state "level", action:"setpoolsettemp"
	}
	controlTile("spaSliderControl", "spasettemp", "slider", height: 1,
             width: 3, inactiveLabel: false, range:"(85..110)") {
    		state "level", action:"setspasettemp"
	}
    valueTile("aux1name", "aux1name", inactiveLabel: false, height: 1, width: 1, decoration: "flat") {
            state "default", label:'${currentValue}'
	}
    valueTile("aux2name", "aux2name", inactiveLabel: false, height: 1, width: 1, decoration: "flat") {
            state "default", label:'${currentValue}'
	}
    valueTile("aux3name", "aux3name", inactiveLabel: false, height: 1, width: 1, decoration: "flat") {
            state "default", label:'${currentValue}'
	}
    valueTile("aux4name", "aux4name", inactiveLabel: false, height: 1, width: 1, decoration: "flat") {
            state "default", label:'${currentValue}'
	}
    valueTile("aux5name", "aux5name", inactiveLabel: false, height: 1, width: 1, decoration: "flat") {
            state "default", label:'${currentValue}'
	}
    valueTile("aux6name", "aux6name", inactiveLabel: false, height: 1, width: 1, decoration: "flat") {
            state "default", label:'${currentValue}'
	}


main(["poolcurrenttemp2"])
//        details(["thermostat", "poolswitch", "spaswitch", "poolcurrenttemp", 
//        "poolsettemp","pooltempup", "spasettemp", "spatempup", "blank", "blank", "pooltempdown", "spatempdown", "blank", "blank", 
//        "poolheatmode", "spaheatmode", "blank", "blank",
//        "aux1", "aux2", "aux3", "aux4", "aux5", "refresh"])
        details(["poolswitch",  "poolcurrenttemp", "spaswitch", "lights",
        "poolsettemp","spasettemp", "poolSliderControl", "spaSliderControl",
        "poolheatmode", "spaheatmode", 
        "aux1", "aux2", "aux3", "aux4", "aux5", "aux6", "aux1name", "aux2name", "aux3name", "aux4name", "aux5name", "aux6name", "refresh"])
    }
}

def installed() {
    log.debug "installed()"
//    printTitle()
//        sendEvent([name:'temperature', value:'70', displayed:false])
	updated();

}
def updated() {
    log.debug "updated()"
 	unschedule()
 	runEvery15Minutes(refresh)
 	runIn(2, refresh)

}


// parse events into attributes
def parse(description) {
log.debug "Parsing"
log.debug device.currentValue("servicemode")
    def msg = parseLanMessage(description)
    def headerString = msg.header
if (headerString?.contains("/notify")){
log.debug "/notify received"
}
 //   log.debug "Parsing '${description}'"
if (headerString?.contains("SID: uuid:")) {
        def sid = (headerString =~ /SID: uuid:.*/) ? ( headerString =~ /SID: uuid:.*/)[0] : "0"
        sid -= "SID: uuid:".trim()

        updateDataValue("subscriptionId", sid)
 	}

    def result = []
    def bodyString = msg.body
    //log.debug msg
//    log.debug bodyString
    
    if (bodyString) {
//    	unschedule("setOffline")
        def slurper = new JsonSlurper()
        def body = slurper.parseText(bodyString)
        //log.debug body
        //def value = body.text() == "off" ? "off" : "on"
        //def value = body.text()
        //log.trace "Notify: BinaryState = ${value}, ${body.property.BinaryState}"
 
     //def eventlist = []

 ['pool', 'spa', 'aux1', 'aux2', 'aux3', 'aux4', 'aux5', 'aux6'].each { deviceName ->
 	def newMode = body[deviceName]["mode"]
//    log.debug "old mode for ${deviceName} is ${device.currentValue(deviceName)}"
    //log.trace body[deviceName]["mode"]
//    log.debug "new mode for ${deviceName} is ${newMode}"
//    if (newMode != device.currentValue(deviceName)){
    def evt1 = createEvent(name: deviceName, value: newMode, descriptionText: "${deviceName} turned ${newMode}", displayed: true)
    result << evt1
    
//    }
  
}
['pool', 'spa'].each { deviceName ->
 	['settemp', 'currenttemp', 'heatmode'].each { deviceSetting ->
    	def newSetting = body[deviceName][deviceSetting]
        def displayyesno = true
//    	log.debug "old ${deviceSetting} for ${deviceName} is ${device.currentValue(deviceName+deviceSetting)}"
    	//log.trace body[deviceName]["mode"]
//    	log.debug "new ${deviceSetting} for ${deviceName} is ${newSetting}"
if (deviceSetting == "currenttemp"){displayyesno = false
sendEvent(name: "temperature",
            value:  newSetting,
            unit:   getTemperatureScale(),)


//log.trace deviceName+deviceSetting
//log.trace newSetting
}
//log.debug "Is this Displayed? ${displayyesno}"

    	def evt1 = createEvent(name: deviceName+deviceSetting, value: newSetting, descriptionText: "${deviceName+deviceSetting} set to ${newSetting}", displayed: displayyesno)
   
     	result << evt1
  }
}
def lights = '| He | So | Se |'
//log.debug body["heaterstate"]["mode"]
if (body["heaterstate"]["mode"] == 'off'){
	lights = lights.replaceAll('He','   ')
}
if (body["solarstate"]["mode"] == 'off'){
	lights = lights.replaceAll('So','   ')
}
if (body["servicemode"]["mode"] == 'off'){
	lights = lights.replaceAll('Se','   ')
result << createEvent(name: "servicemode", value: "off")
}else {
result << createEvent(name: "servicemode", value: "on"
)
}
log.debug lights
result << createEvent(name: 'lights', value: lights)


// result << createEvent(name: "pooltemp", value: 77, descriptionText: "pool temp turned 77", displayed: true)

 	}
//    sendEvent(name:"temperature", value: device.poolcurrenttemp) 

//    result
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
// 	log.debug "Using ip: ${ip} and port: ${port} for device: ${device.id}"
    if (ip?.contains("."))
    {
 	return ip + ":" + port
    }
    else
    {
 	return convertHexToIP(ip) + ":" + convertHexToInt(port)
    }
}
def poolon() {
//    runIn(3, poll)
//    sendEvent(name: "pool", value: "turningOn",  displayed: false)

	on('pool')
}
def pooloff() {
//    runIn(3, poll)
//    sendEvent(name: "pool", value: "turningOff",  displayed: false)
	off('pool')
}

def spaon() {
 //   runIn(3, poll)
//    sendEvent(name: "spa", value: "turningOn",  displayed: false)
	on('spa')
}
def spaoff() {
//    runIn(3, poll)
//    sendEvent(name: "spa", value: "turningOff",  displayed: false)
	off('spa')
}

def aux1on() {
//    runIn(3, poll)
//    sendEvent(name: "aux1", value: "turningOn",  displayed: false)
//	log.debug device.currenState("aux1")
//if (device.currentValue("servicemode") == "off"){
	on('aux1')
//}
}

def aux1off() {
//    runIn(3, poll)
//    sendEvent(name: "aux1", value: "turningOff",  displayed: false)
//	log.debug device.currentState("aux1")
//if (device.currentValue("servicemode") == "off"){
	off('aux1')
//    }
}

def aux2on() {
//    runIn(3, poll)
//    sendEvent(name: "aux2", value: "turningOn",  displayed: false)
	on('aux2')
}
def aux2off() {
//    runIn(3, poll)
//    sendEvent(name: "aux2", value: "turningOff",  displayed: false)
	off('aux2')
}

def aux3on() {
//    runIn(3, poll)
//    sendEvent(name: "aux3", value: "turningOn",  displayed: false)
	on('aux3')
}
def aux3off() {
//    runIn(3, poll)
//    sendEvent(name: "aux3", value: "turningOff",  displayed: false)
	off('aux3')
}

def aux4on() {
//    runIn(3, poll)
//    sendEvent(name: "aux4", value: "turningOn",  displayed: false)
	on('aux4')
}
def aux4off() {
//    runIn(3, poll)
//    sendEvent(name: "aux4", value: "turningOff",  displayed: false)
	off('aux4')
}

def aux5on() {
//    runIn(3, poll)
//    sendEvent(name: "aux5", value: "turningOn",  displayed: false)
	on('aux5')
}
def aux5off() {
//    runIn(3, poll)
//    sendEvent(name: "aux5", value: "turningOff",  displayed: false)
	off('aux5')
}
def aux6on() {
//    runIn(3, poll)
//    sendEvent(name: "aux6", value: "turningOn",  displayed: false)
	on('aux6')
}
def aux6off() {
//    runIn(3, poll)
//    sendEvent(name: "aux6", value: "turningOff",  displayed: false)
	off('aux6')
}

def poolheatmodeoff() {
//    runIn(3, poll)
	heatmode('pool', 'off')
}

def poolheatmodesolarpriority() {
//    runIn(3, poll)
	heatmode('pool', 'solarpriority')
}

def poolheatmodesolaronly(){
//    runIn(3, poll)
	heatmode('pool', 'solaronly')
}

def poolheatmodeheater(){
//    runIn(3, poll)
	heatmode('pool', 'heater')
}

def spaheatmodeoff(){
//    runIn(3, poll)
	heatmode('spa', 'off')
}

def spaheatmodesolarpriority(){
//    runIn(3, poll)
	heatmode('spa', 'solarpriority')
}

def spaheatmodesolaronly(){
//    runIn(3, poll)
	heatmode('spa', 'solaronly')
}

def spaheatmodeheater(){
//    runIn(3, poll)
	heatmode('spa', 'heater')
}

def setpoolsettemp(t){
//    runIn(3, poll)
	settemp('pool', t)
    //def result = []
	//log.debug t
	//sendEvent(name: "poolsettemp", value: t)
}

def setspasettemp(t){
//    runIn(3, poll)
	settemp('spa', t)
    //def result = []
	//log.debug t
	//sendEvent(name: "spasettemp", value: t)
}

def heatmode(device, mode) {
log.debug "hit the button HeatMode ${mode} for ${device}"
    def result = new physicalgraph.device.HubAction(
        method: "GET",
        path: "/setheatmode",
        headers: [
            HOST: getHostAddress()
        ],
        query: [device: device, mode: mode]
    )
    log.debug result
    return result
}

def settemp(device, temp){
log.debug "Executing 'settemp' for device: ${device} to temp: ${temp}"
    def result = new physicalgraph.device.HubAction(
        method: "GET",
        path: "/settemp",
        headers: [
            HOST: getHostAddress()
        ],
        query: [device: device, temp: temp]
    )
    //log.debug result
    
    return result

}

def on(compdevice) {
log.debug "Executing 'on' for device: " + compdevice
    def result = new physicalgraph.device.HubAction(
        method: "GET",
        path: "/on",
        headers: [
            HOST: getHostAddress(),
            'Content-Type': 'application/json'
        ],
        query: [device: compdevice]
    )
//    sendHubCommand(result)
//    log.debug result
    return result
}

def off(compdevice) {
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
//    sendHubCommand(result)
//    log.debug result
    return result
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
log.debug result
    log.trace "SUBSCRIBE $path"

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
log.debug result
    log.trace "SUBSCRIBE $path"

    return result
}

def subscribe() {
	subscribeAction("/subscribe")
}

def refresh() {
sendEvent([name: 'aux1name', value: aux1name ?: 'aux1'])
sendEvent([name: 'aux2name', value: aux2name ?: 'aux2'])
sendEvent([name: 'aux3name', value: aux3name ?: 'aux3'])
sendEvent([name: 'aux4name', value: aux4name ?: 'aux4'])
sendEvent([name: 'aux5name', value: aux5name ?: 'aux5'])
sendEvent([name: 'aux6name', value: aux6name ?: 'aux6'])

 	log.debug "Executing ESP8266 Switch 'subscribe', then 'poll'"
 	[
    subscribe(),
    poll()
    ]
}

def subscribe(ip, port) {
    def existingIp = getDataValue("ip")//device ip
    def existingPort = getDataValue("port")//device port
    if (ip && ip != existingIp) {
         log.debug "Updating ip from $existingIp to $ip"    
    	 updateDataValue("ip", ip)
    	 def ipvalue = convertHexToIP(getDataValue("ip"))
         sendEvent(name: "currentIP", value: ipvalue, descriptionText: "IP changed to ${ipvalue}")
    }
 	if (port && port != existingPort) {
 		log.debug "Updating port from $existingPort to $port"
 		updateDataValue("port", port)
	}
    
 	log.debug "Subscribe ${ip}:${port}"
	subscribe("${ip}:${port}")
}

def resubscribe() {
    log.debug "Executing 'resubscribe()'"
    subscribe()
}


def unsubscribe() {
	unsubscribeAction("/on")
	unsubscribeAction("/off")
	unsubscribeAction("/poll")
}


//TODO: Use UTC Timezone


def setOffline() {
	//sendEvent(name: "currentIP", value: "Offline", displayed: false)
    sendEvent(name: "switch", value: "offline", descriptionText: "The device is offline")
}

def poll() {
log.debug "Executing 'poll'"
//if (device.currentValue("currentIP") != "Offline")
//    runIn(30, setOffline)

    def result = new physicalgraph.device.HubAction(
        method: "GET",
        path: "/poll",
        headers: [
            HOST: getHostAddress()
        ]
    )
    log.debug result
//   sendHubCommand(result)
return result  
}
