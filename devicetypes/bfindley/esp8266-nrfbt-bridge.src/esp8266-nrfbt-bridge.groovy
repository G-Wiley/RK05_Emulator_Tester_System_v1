/**
 *  
 *  ESP8266 NRF/BT Window Shades Bridge
 *  
 *
 * Derived from: 
 * ESP8266 Switch
 *
 * Author: Jacob Schreiver 
 * Date: 2016-02-16
 * Derived from Wemo Switch 
 * 	- Author: Juan Risso (SmartThings)
 *  - Date: 2015-10-11 
 * 
 */

import groovy.json.JsonSlurper


metadata {
    definition (name: "ESP8266_NRFBT_Bridge", namespace: "bfindley", author: "Brian Findley") {
    capability "Switch"
    capability "Thermostat"
    capability "Temperature Measurement"
    capability "Polling"
    capability "Refresh"
    capability "Sensor"

    // Custom attributes
    attribute "win01", "string" //open state
    attribute "win02", "string" //open state
    attribute "win03", "string" //open state
    attribute "win04", "string" //open state
    attribute "win01openstate", "string" //set to % open
    attribute "win02openstate", "string" //set to % open
    attribute "win03openstate", "string" //set to % open
    attribute "win04openstate", "string" //set to % open
    attribute "currentIP", "string"
    attribute "win01name", "string"
    attribute "win02name", "string"
    attribute "win03name", "string"
    attribute "win04name", "string"
 
    // Custom commands
    command "subscribe"
    command "resubscribe"
    command "unsubscribe"
    command "setOffline"
    command "win01on"
    command "win01off"
    command "win02on"
    command "win02off"
    command "win03on"
    command "win03off"
    command "win04on"
    command "win04off"
    command "setwin01openstate"
    command "setwin02openstate"
    command "setwin03openstate"
    command "setwin04openstate"
    command "poll"
}

// simulator metadata
simulator {}

preferences {
    input name: "win01name", type: "text", title: "Win01 Name", description: "Enter Text"
}
preferences {
    input name: "win02name", type: "text", title: "Win02 Name", description: "Enter Text"
}
preferences {
    input name: "win03name", type: "text", title: "Win03 Name", description: "Enter Text"
}
preferences {
    input name: "win04name", type: "text", title: "Win04 Name", description: "Enter Text"
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
    standardTile("win01on", "device.win01on", canChangeIcon: false) {
        state "on", label:'aux1', action:"aux1off", icon:"st.samsung.da.RC_ic_power", backgroundColor:"#79b821", nextState:"turningOff"
        state "off", label:'aux1', action:"aux1on", icon:"st.samsung.da.RC_ic_power", backgroundColor:"#ffffff", nextState:"turningOn"
        //state "on", label:'aux1', action:"aux1off", icon:"st.samsung.da.RC_ic_power", backgroundColor:"#79b821"
        //state "off", label:'aux1', action:"aux1on", icon:"st.samsung.da.RC_ic_power", backgroundColor:"#ffffff"
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
    standardTile("refresh", "device.switch", inactiveLabel: false, height: 1, width: 1, decoration: "flat") {
        state "default", label:"", action:"refresh.refresh", icon:"st.secondary.refresh"
    }
    standardTile("subscribe", "device.switch", inactiveLabel: false, height: 1, width: 1, decoration: "flat") {
        state "default", label:"", action:"subscribe", icon:"st.secondary.refresh"
    }
    valueTile("blank", "blank", inactiveLabel: false, height: 1, width: 1, decoration: "flat") {
        state "default", label:'', unit:"F"
    }
    valueTile("blank3v", "blank3v", inactiveLabel: false, height: 3, width: 1, decoration: "flat") {
        state "default", label:'', unit:"F"
    }
    controlTile("win01SliderControl", "win01openstate", "slider", height: 3, width: 1, inactiveLabel: false, range:"(0..100)") {
            state "openstate", action:"setwin01openstate"
    }
    controlTile("win02SliderControl", "win02openstate", "slider", height: 3, width: 1, inactiveLabel: false, range:"(0..100)") {
        state "level", action:"setwin02openstate"
    }
    controlTile("win03SliderControl", "win03openstate", "slider", height: 3, width: 1, inactiveLabel: false, range:"(0..100)") {
        state "level", action:"setwin03openstate"
    }
    controlTile("win04SliderControl", "win04openstate", "slider", height: 3, width: 1, inactiveLabel: false, range:"(0..100)") {
        state "level", action:"setwin04openstate"
    }
    valueTile("win01name", "win01name", inactiveLabel: false, height: 1, width: 1, decoration: "flat") {
        state "default", label:'win01'
    }
    valueTile("win02name", "win02name", inactiveLabel: false, height: 1, width: 1, decoration: "flat") {
        state "default", label:'win02'
    }
    valueTile("win03name", "win03name", inactiveLabel: false, height: 1, width: 1, decoration: "flat") {
        state "default", label:'win03'
    }
    valueTile("win04name", "win04name", inactiveLabel: false, height: 1, width: 1, decoration: "flat") {
        state "default", label:'win04'
    }
    standardTile("w1on", "device.win01on", canChangeIcon: false) {
        state "on", label:'W1on', action:"win01on", icon:"st.samsung.da.RC_ic_power", backgroundColor:"#79b821", nextState:"off"
        state "off", label:'W1on', action:"win01on", icon:"st.samsung.da.RC_ic_power", backgroundColor:"#ffffff", nextState:"on"
        state "offline", label:'offline', icon:"st.samsung.da.RC_ic_power", backgroundColor:"#ff0000"
    }
    standardTile("w1off", "device.win01off", canChangeIcon: false) {
        state "on", label:'W1off', action:"win01off", icon:"st.samsung.da.RC_ic_power", backgroundColor:"#79b821", nextState:"off"
        state "off", label:'W1off', action:"win01off", icon:"st.samsung.da.RC_ic_power", backgroundColor:"#ffffff", nextState:"on"
        state "offline", label:'offline', icon:"st.samsung.da.RC_ic_power", backgroundColor:"#ff0000"
    }
    standardTile("w2on", "device.win02on", canChangeIcon: false) {
        state "on", label:'W2on', action:"win02on", icon:"st.samsung.da.RC_ic_power", backgroundColor:"#79b821", nextState:"off"
        state "off", label:'W2on', action:"win02on", icon:"st.samsung.da.RC_ic_power", backgroundColor:"#ffffff", nextState:"on"
        state "offline", label:'offline', icon:"st.samsung.da.RC_ic_power", backgroundColor:"#ff0000"
    }
    standardTile("w2off", "device.win02off", canChangeIcon: false) {
        state "on", label:'W2off', action:"win02off", icon:"st.samsung.da.RC_ic_power", backgroundColor:"#79b821", nextState:"off"
        state "off", label:'W2off', action:"win02off", icon:"st.samsung.da.RC_ic_power", backgroundColor:"#ffffff", nextState:"on"
        state "offline", label:'offline', icon:"st.samsung.da.RC_ic_power", backgroundColor:"#ff0000"
    }
    standardTile("w3on", "device.win03on", canChangeIcon: false) {
        state "on", label:'W3on', action:"win03on", icon:"st.samsung.da.RC_ic_power", backgroundColor:"#79b821", nextState:"off"
        state "off", label:'W3on', action:"win03on", icon:"st.samsung.da.RC_ic_power", backgroundColor:"#ffffff", nextState:"on"
        state "offline", label:'offline', icon:"st.samsung.da.RC_ic_power", backgroundColor:"#ff0000"
    }
    standardTile("w3off", "device.win03off", canChangeIcon: false) {
        state "on", label:'W3off', action:"win03off", icon:"st.samsung.da.RC_ic_power", backgroundColor:"#79b821", nextState:"off"
        state "off", label:'W3off', action:"win03off", icon:"st.samsung.da.RC_ic_power", backgroundColor:"#ffffff", nextState:"on"
        state "offline", label:'offline', icon:"st.samsung.da.RC_ic_power", backgroundColor:"#ff0000"
    }
    standardTile("w4on", "device.win04on", canChangeIcon: false) {
        state "on", label:'W4on', action:"win04on", icon:"st.samsung.da.RC_ic_power", backgroundColor:"#79b821", nextState:"off"
        state "off", label:'W4on', action:"win04on", icon:"st.samsung.da.RC_ic_power", backgroundColor:"#ffffff", nextState:"on"
        state "offline", label:'offline', icon:"st.samsung.da.RC_ic_power", backgroundColor:"#ff0000"
    }
    standardTile("w4off", "device.win04off", canChangeIcon: false) {
        state "on", label:'W4off', action:"win04off", icon:"st.samsung.da.RC_ic_power", backgroundColor:"#79b821", nextState:"off"
        state "off", label:'W4off', action:"win04off", icon:"st.samsung.da.RC_ic_power", backgroundColor:"#ffffff", nextState:"on"
        state "offline", label:'offline', icon:"st.samsung.da.RC_ic_power", backgroundColor:"#ff0000"
    }


main(["windowstemp2"])
//        details(["thermostat", "poolswitch", "spaswitch", "poolcurrenttemp", 
//        "poolsettemp","pooltempup", "spasettemp", "spatempup", "blank", "blank", "pooltempdown", "spatempdown", "blank", "blank", 
//        "poolheatmode", "spaheatmode", "blank", "blank",
//        "aux1", "aux2", "aux3", "aux4", "aux5", "refresh"])
    details(["win01SliderControl", "win02SliderControl", "win03SliderControl", "win04SliderControl", "blank3v", "blank3v",
        "w1on", "w2on", "w3on", "w4on", "blank", "blank", "w1off", "w2off", "w3off", "w4off", "blank", "blank", 
        "win01name", "win02name", "win03name", "win04name", "blank", "blank", 
        "refresh"])
    }
}

def installed() {
    log.debug "installed()"
    printTitle()
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
    //log.debug "Parsing '${description}'"
    if (headerString?.contains("SID: uuid:")) {
        def sid = (headerString =~ /SID: uuid:.*/) ? ( headerString =~ /SID: uuid:.*/)[0] : "0"
        sid -= "SID: uuid:".trim()

        updateDataValue("subscriptionId", sid)
    }

    def result = []
    def bodyString = msg.body
    //log.debug msg
    //log.debug bodyString
    
    if (bodyString) {
        //unschedule("setOffline")
        def slurper = new JsonSlurper()
        def body = slurper.parseText(bodyString)
        //log.debug body
        //def value = body.text() == "off" ? "off" : "on"
        //def value = body.text()
        //log.trace "Notify: BinaryState = ${value}, ${body.property.BinaryState}"
 
        //def eventlist = []

        ['win01', 'win02', 'win03', 'win04'].each { deviceName ->
            def newMode = body[deviceName]["openstate"]
            //log.debug "old mode for ${deviceName} is ${device.currentValue(deviceName)}"
            //log.trace body[deviceName]["mode"]
            //log.debug "new mode for ${deviceName} is ${newMode}"
            //if (newMode != device.currentValue(deviceName)){
            def evt1 = createEvent(name: deviceName, value: newMode, descriptionText: "${deviceName} turned ${newMode}", displayed: true)
            result << evt1
            //}
        }

        //['win01', 'win02', 'win03', 'win04'].each { deviceName ->
        //['settemp', 'currenttemp', 'heatmode'].each { deviceSetting ->
        //def newSetting = body[deviceName][deviceSetting]
        //def displayyesno = true
        //log.debug "old ${deviceSetting} for ${deviceName} is ${device.currentValue(deviceName+deviceSetting)}"
        //log.trace body[deviceName]["mode"]
        //log.debug "new ${deviceSetting} for ${deviceName} is ${newSetting}"
        //if (deviceSetting == "currenttemp"){displayyesno = false
            //log.trace deviceName+deviceSetting
            //log.trace newSetting
        //}
        //log.debug "Is this Displayed? ${displayyesno}"

    	def evt1 = createEvent(name: deviceName+deviceSetting, value: newSetting, descriptionText: "${deviceName+deviceSetting} set to ${newSetting}", displayed: displayyesno)
   
     	result << evt1
    }
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
    //log.debug "Using ip: ${ip} and port: ${port} for device: ${device.id}"
    if (ip?.contains("."))
    {
        return ip + ":" + port
    }
    else
    {
        return convertHexToIP(ip) + ":" + convertHexToInt(port)
    }
}

def win01on() {
    //runIn(3, poll)
    //sendEvent(name: "pool", value: "turningOn",  displayed: false)

	setwindow('win01', '100')
}
def win01off() {
    //runIn(3, poll)
    //sendEvent(name: "pool", value: "turningOff",  displayed: false)
    
	//off('win01')
    setwindow('win01', '0')
}

def win02on() {
    //runIn(3, poll)
    //sendEvent(name: "pool", value: "turningOn",  displayed: false)

	//on('win02')
    setwindow('win02', '100')
}
def win02off() {
    //runIn(3, poll)
    //sendEvent(name: "pool", value: "turningOff",  displayed: false)
    
	//off('win02')
    setwindow('win02', '0')
}

def win03on() {
    //runIn(3, poll)
    //sendEvent(name: "pool", value: "turningOn",  displayed: false)

	//on('win03')
    setwindow('win03', '100')
}
def win03off() {
    //runIn(3, poll)
    //sendEvent(name: "pool", value: "turningOff",  displayed: false)
    
	//off('win03')
    setwindow('win03', '0')
}

def win04on() {
    //runIn(3, poll)
    //sendEvent(name: "pool", value: "turningOn",  displayed: false)

	//on('win04')
    setwindow('win04', '100')
}
def win04off() {
    //runIn(3, poll)
    //sendEvent(name: "pool", value: "turningOff",  displayed: false)
    
	//off('win04')
    setwindow('win04', '0')
}


def setwin01openstate(t){
    // runIn(3, poll)
    setwindow('win01', t)
    //def result = []
    //log.debug t
    //sendEvent(name: "win01openstate", value: t)
}

def setwin02openstate(t){
    // runIn(3, poll)
    setwindow('win02', t)
    //def result = []
    //log.debug t
    //sendEvent(name: "win02openstate", value: t)
}

def setwin03openstate(t){
    // runIn(3, poll)
    setwindow('win03', t)
    //def result = []
    //log.debug t
    //sendEvent(name: "win03openstate", value: t)
}

def setwin04openstate(t){
    // runIn(3, poll)
    setwindow('win04', t)
    //def result = []
    //log.debug t
    //sendEvent(name: "win04openstate", value: t)
}

def setwindow(device, percentopen){
    log.debug "Executing 'setwindow' for device: ${device} to opening: ${percentopen}"
    def result = new physicalgraph.device.HubAction(
        method: "GET",
        path: "/goto",
        headers: [
            HOST: getHostAddress()
        ],
        query: [device: device, setting: percentopen]
    )
    //log.debug result
    
    return result

}

def stepscalibrate(device, calvalue){
    log.debug "Executing 'stepscalibrate' for device: ${device} to steps-to-open: ${calvalue}"
    def result = new physicalgraph.device.HubAction(
        method: "GET",
        path: "/stepsinrange",
        headers: [
            HOST: getHostAddress()
        ],
        query: [device: device, setting: calvalue]
    )
    //log.debug result
    
    return result

}

//From ComPool, leave it here and may use it later
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

//From ComPool, leave it here and may use it later
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
    //sendHubCommand(result)
    return result  
}