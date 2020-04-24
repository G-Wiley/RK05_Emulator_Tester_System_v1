/**
 *  Generic UPnP Service Manager
 *
 *  Copyright 2016 SmartThings
 *
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except
 *  in compliance with the License. You may obtain a copy of the License at:
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software distributed under the License is distributed
 *  on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License
 *  for the specific language governing permissions and limitations under the License.
 *
 * Author: Brian Findley 
 * Date: 04/20/2020
 * Version 19a
 */
definition(
		name: "New Compool",
		namespace: "smartthings",
		author: "SmartThings",
		description: "This is a basic template for a UPnP Service Manager SmartApp, intended for instructional use only",
		category: "SmartThings Labs",
		iconUrl: "https://s3.amazonaws.com/smartapp-icons/Convenience/Cat-Convenience.png",
		iconX2Url: "https://s3.amazonaws.com/smartapp-icons/Convenience/Cat-Convenience@2x.png",
		iconX3Url: "https://s3.amazonaws.com/smartapp-icons/Convenience/Cat-Convenience@2x.png")


preferences {
	page(name: "searchTargetSelection", title: "firstpage", content: "searchTargetSelection")
	page(name: "deviceDiscovery", title: "UPnP Device Setup", content: "deviceDiscovery")
    page(name: "addswitch")
    page(name: "removeswitch")
    page(name: "createPage")
    page(name: "removePage")
}

def searchTargetSelection(){
	app.updateSetting("resetDevices", "No")
	return 	dynamicPage(name: "searchTargetSelection", title: "UPnP Search Target", nextPage: "deviceDiscovery") {
		section("Search Target") {
			input "searchTarget", "string", title: "Search Target", defaultValue: "urn:schemas-upnp-org:device:Compool:1", required: true
            input "resetDevices", "enum", required: true, title: "Clear devices list?", multiple: false, options: ["Yes","No"], description: "NO"

		}
			section("Set Up Virtual Devices..."){
            href name: "addswitch", title: "Add Virtual Switch", page: "addswitch"
        }

	}

}

def deviceDiscovery() {
	if (resetDevices == "Yes"){
		state.remove("devices")
    	app.updateSetting("resetDevices", "No")
    }
	def options = [:]
	def devices = getVerifiedDevices()
	devices.each {
//    	log.debug "it: $it"
//		def value = it.value.name ?: "UPnP Device ${it.value.ssdpUSN.split(':')[1][-3..-1]}"
		def value = it.value.name ?: "${it.value.model}-${it.value.mac}"
		def key = it.value.mac
		options["${key}"] = value
	}

	ssdpSubscribe()

	ssdpDiscover()
	verifyDevices()

	return dynamicPage(name: "deviceDiscovery", title: "Discovery Started!", nextPage: "", refreshInterval: 5, install: true, uninstall: true) {
		section("Please wait while we discover your UPnP Device. Discovery can take five minutes or more, so sit back and relax! Select your device below once discovered.") {
			input "selectedDevices", "enum", required: false, title: "Select Devices (${options.size() ?: 0} found)", multiple: true, options: options
		}
	}
}
def addswitch(){
	def children = getChildDevices(true)
	def childdeviceinfo = []
	def devicechoices = []
    log.debug children
	children.each{ 
		childdeviceinfo << [devicename: it.name, networkid: it.deviceNetworkId, deviceid: it.id]
		log.debug "childdeviceeinfo: " + childdeviceinfo
	}
	['Pool', 'Spa', 'Aux1', 'Aux2', 'Aux3', 'Aux4', 'Aux5', 'Aux6'].each { devicename ->
    	def size = childdeviceinfo*.get('devicename').findIndexValues { it.contains("${devicename}")}.size()
    	log.debug size
    	if (size > 0){
    		log.debug "already have ${devicename}" 
    		def addto = "-" + size
        	def ndevicename = devicename + addto
        	log.debug ndevicename
    		devicechoices.push("${ndevicename}")
    	}else{
			devicechoices.push("${devicename}")
		}
    }
    log.debug devicechoices
	dynamicPage(name: "addswitch", title: "Pick the switch to add the virtual device", nextPage: "createPage") {
        section{
                    input name: "addvirtualdevice", type: "enum", title: "Device to Add", options: devicechoices, required: false
                    input name: "switchname", type: "text", title: "Enter Name of Switch", defaultValue: "enter name"
        }
	}
}
def createPage(){
	log.debug "createPage"
    log.debug "addvirtualdevice: $addvirtualdevice"
	def children = getChildDevices(true)
	def childdeviceinfo = []
	children.each{ 
		childdeviceinfo << [devicename: it.name, networkid: it.deviceNetworkId, deviceid: it.id]
	}
	if (addvirtualdevice){
    	log.debug childdeviceinfo*.get('devicename').findIndexValues { it.contains("${addvirtualdevice}") }
		def child = addChildDevice('bfindley', 'Simulated Switch', addvirtualdevice, null,[name: addvirtualdevice, label: switchname, completeSetup: true])
    }
	dynamicPage(name: "createPage", title: "${addvirtualdevice} has been added", nextPage: "deviceDiscovery") {
        section{
                    paragraph "Hit Next and then Done to complete the connection"
		}
	}
}
def subscribeToDevices() {
	log.debug "subscribeToDevices() called"
//	def devices = getAllChildDevices()
//	devices.each { d ->
//		d.subscribe()
//	}
//	unsubscribe()
	def children = getChildDevices(true)
	def childdeviceinfo = []
    def bridgedevice
    log.debug selectedSwitch
    children.each{
    	if (it.getName() == "ESP8266_Compool_Bridge"){
    		bridgedevice = it
    	}
    }
	children.each{ 
    	log.debug it.getName()
        log.debug it.deviceNetworkId
        if (it.getName() != "ESP8266_Compool_Bridge"){
    		subscribe(it, "switch", switchevent)
          	subscribe(bridgedevice, "${it.getName().toLowerCase()}", masterevent)
//log.debug "subscribed to ${bridgedevice.getName()} with ${it.getName().toLowerCase()}"
        }
	}

}
def switchevent(evt){
log.debug "switchevent(${evt.getDevice().getDisplayName()} set to ${evt.value} at ${evt.date}. ID= ${evt.id}, source= ${evt.source}, isStateChange = ${evt.isStateChange()}, type = ${evt.type})"

//	log.debug "getLabel " + evt.getDevice().getLabel()
//	log.debug "getStatus " +  evt.getDevice().getStatus()
//	log.debug "getId " +  evt.getDevice().getId()
//	log.debug "getName " +  evt.getDevice().getName()
//	log.debug "evt.value " +  evt.value
	def name = evt.getDevice().getName().toLowerCase()
	def mastercommand = name + evt.value
//	log.debug mastercommand
	def ch = getChildDevices(true).find { it.name == "ESP8266_Compool_Bridge" }
    log.debug "children: $ch"
    if (evt.type != "physical"){
	log.debug "selectedSwitches: $selectedSwitches"
//	def switch0 = getChildDevice(selectedSwitches[0])
	def switch0 = ch
    switch0."${mastercommand}"()
    }
}
def masterevent(evt){
log.debug "masterevent(${evt.getDevice().getDisplayName()} set to ${evt.value} at ${evt.date}. ID= ${evt.id}, source= ${evt.source}, isStateChange = ${evt.isStateChange()}, type = ${evt.type})"
//	log.debug "getLabel " + evt.getDevice().getLabel()
//	log.debug "getStatus " +  evt.getDevice().getStatus()
//	log.debug "getId " +  evt.getDevice().getId()
//	log.debug "getName " +  evt.getDevice().getName()
//	log.debug "evt.value " +  evt.value
//	log.debug evt.data
//	log.debug evt.descriptionText
//	log.debug evt.displayName
//	log.debug evt.linkText
//  log.debug "evt.name: " + evt.name.capitalize()
//	def children = getChildDevices(true)
    def namecaps = evt.name.capitalize()
//    log.debug namecaps
    def child = getChildDevice("${namecaps}")
//    log.debug child.getName()
    child."${evt.value}Physical"()
}

def installed() {
	log.debug "Installed with settings: ${settings}"

	initialize()
}

def updated() {
	log.debug "Updated with settings: ${settings}"

	unsubscribe()
	initialize()
}

def initialize() {
	unsubscribe()
	unschedule()
	ssdpSubscribe()

	if (selectedDevices) {
		addDevices()
	}
   	if (selectedSwitches)
		addSwitches()




	runEvery5Minutes("ssdpDiscover")
   	runIn(5, "subscribeToDevices") //initial subscriptions delayed by 5 seconds

}

void ssdpDiscover() {
	sendHubCommand(new physicalgraph.device.HubAction("lan discovery ${searchTarget}", physicalgraph.device.Protocol.LAN))
}

void ssdpSubscribe() {
	subscribe(location, "ssdpTerm.${searchTarget}", ssdpHandler)
}

Map verifiedDevices() {
	def devices = getVerifiedDevices()
	def map = [:]
	devices.each {
		def value = it.value.name ?: "UPnP Device ${it.value.ssdpUSN.split(':')[1][-3..-1]}"
		def key = it.value.mac
		map["${key}"] = value
	}
	map
}

void verifyDevices() {
	def devices = getDevices().findAll { it?.value?.verified != true }
	devices.each {
		int port = convertHexToInt(it.value.deviceAddress)
		String ip = convertHexToIP(it.value.networkAddress)
		String host = "${ip}:${port}"
		sendHubCommand(new physicalgraph.device.HubAction("""GET ${it.value.ssdpPath} HTTP/1.1\r\nHOST: $host\r\n\r\n""", physicalgraph.device.Protocol.LAN, host, [callback: deviceDescriptionHandler]))
	}
}

def getVerifiedDevices() {
	getDevices().findAll{ it.value.verified == true }
}

def getDevices() {
	if (!state.devices) {
		state.devices = [:]
	}
	state.devices
}

def addDevices() {
	def devices = getDevices()

	selectedDevices.each { dni ->
		def selectedDevice = devices.find { it.value.mac == dni }
		def d
		if (selectedDevice) {
			d = getChildDevices()?.find {
				it.deviceNetworkId == selectedDevice.value.mac
			}
		}

		if (!d) {
			log.debug "Creating Generic UPnP Device with dni: ${selectedDevice.value.mac}"
			addChildDevice("bfindley", "ESP8266_Compool_Bridge", selectedDevice.value.mac, selectedDevice?.value.hub, [
				"label": selectedDevice?.value?.name ?: "Compool",
				"data": [
					"mac": selectedDevice.value.mac,
					"ip": selectedDevice.value.networkAddress,
					"port": selectedDevice.value.deviceAddress
				]
			])
//                  		def ipvalue = convertHexToIP(selectedSwitch.value.ip)
//			d.sendEvent(name: "currentIP", value: ipvalue, descriptionText: "IP is ${ipvalue}")
//			log.debug "Created ${d.displayName} with id: ${d.id}, dni: ${d.deviceNetworkId}"

		}
	}
}

def ssdpHandler(evt) {
log.debug "ssdpHandler"
	def description = evt.description
	def hub = evt?.hubId
	def parsedEvent = parseLanMessage(description)
	parsedEvent << ["hub":hub]
	def devices = getDevices()
	String ssdpUSN = parsedEvent.ssdpUSN.toString()
	if (devices."${ssdpUSN}") {
		def d = devices."${ssdpUSN}"
		if (d.networkAddress != parsedEvent.networkAddress || d.deviceAddress != parsedEvent.deviceAddress) {
			d.networkAddress = parsedEvent.networkAddress
			d.deviceAddress = parsedEvent.deviceAddress
			def child = getChildDevice(parsedEvent.mac)
			if (child) {
				child.sync(parsedEvent.networkAddress, parsedEvent.deviceAddress)
			}
		}
	} else {
		devices << ["${ssdpUSN}": parsedEvent]
	}
}

void deviceDescriptionHandler(physicalgraph.device.HubResponse hubResponse) {
	def body = hubResponse.xml
	def devices = getDevices()
	def device = devices.find { it?.key?.contains(body?.device?.UDN?.text()) }
	if (device) {
		device.value << [name: body?.device?.roomName?.text(), model:body?.device?.modelName?.text(), serialNumber:body?.device?.serialNum?.text(), verified: true]
	}
}

private Integer convertHexToInt(hex) {
	Integer.parseInt(hex,16)
}
def getespSwitches()
{
	if (!state.switches) { state.switches = [:] }
	state.switches
}

private String convertHexToIP(hex) {
	[convertHexToInt(hex[0..1]),convertHexToInt(hex[2..3]),convertHexToInt(hex[4..5]),convertHexToInt(hex[6..7])].join(".")
}
def addSwitches() {
	def switches = getespSwitches()

	selectedSwitches.each { dni ->
		def selectedSwitch = switches.find { it.value.mac == dni } ?: switches.find { "${it.value.ip}:${it.value.port}" == dni }
		def d
		if (selectedSwitch) {
			d = getChildDevices()?.find {
				it.dni == selectedSwitch.value.mac || it.device.getDataValue("mac") == selectedSwitch.value.mac
			}
		}

		if (!d) {
        
			log.debug "Creating ESP..."
			log.debug "Creating esp Switch with name: ${selectedSwitch.value.name}"
			log.debug "Creating esp Switch with dni: ${selectedSwitch.value.mac}"
            
			d = addChildDevice("bfindley", "ESP8266_Compool_Bridge", selectedSwitch.value.mac, selectedSwitch?.value.hub, [
					"label": selectedSwitch?.value?.name ?: "ESP8266_Compool_Bridge",
					"data": [
							"mac": selectedSwitch.value.mac,
							"ip": selectedSwitch.value.ip,
							"port": selectedSwitch.value.port
					]
			])
      def ipvalue = convertHexToIP(selectedSwitch.value.ip)
			d.sendEvent(name: "currentIP", value: ipvalue, descriptionText: "IP is ${ipvalue}")
			log.debug "Created ${d.displayName} with id: ${d.id}, dni: ${d.deviceNetworkId}"
		} else {
			log.debug "found ${d.displayName} with id $dni already exists"
		}
	}
}
