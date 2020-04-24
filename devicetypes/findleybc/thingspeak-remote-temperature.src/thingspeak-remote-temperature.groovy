/**
 * Particle (Spark) Core / Photon / Electron Remote Temperature and Humidity Logger
 *
 * Author: Nic Jansma
 *
 * Licensed under the MIT license
 *
 * Available at: https://github.com/nicjansma/smart-things/
 *
 * Device type for a Particle (Spark) Core/Photon/Electron temperature/humidity/heat index sensor:
 *   https://github.com/nicjansma/dht-logger/
 */

preferences {
    input name: "apikey", type: "text", title: "API Key", required: false, defaultValue: "I6LBF5MFL2YQVGO6"
    input name: "channelid", type: "text", title: "Channel ID", required: false, defaultValue: "116559"
    input name: "temperaturefield", type: "text", title: "Temperature Field", required: false, defaultValue: "field1"
    //input name: "sparkHumidityVar", type: "text", title: "Spark Humidity Variable", required: true, defaultValue: "humidity"
    //input name: "sparkHeatIndexVar", type: "text", title: "Spark Heat Index Variable", required: true, defaultValue: "heatIndex"
}

metadata {
    definition (name: "Thingspeak Remote Temperature", namespace: "findleybc", author: "Brian Findley") {
        capability "Polling"
        capability "Sensor"
        capability "Refresh"
        capability "Relative Humidity Measurement"
        capability "Temperature Measurement"

        attribute "temperature", "number"
        attribute "lastupdate", "date"
    }

    tiles(scale: 2) {
        valueTile("temperature", "device.temperature", width: 2, height: 2) {
            state("temperature", label:'${currentValue}°', unit:"F",
                backgroundColors:[    
                    [value: 31, color: "#153591"],
                    [value: 44, color: "#1e9cbb"],
                    [value: 59, color: "#90d2a7"],
                    [value: 74, color: "#44b621"],
                    [value: 84, color: "#f1d801"],
                    [value: 95, color: "#d04e00"],
                    [value: 96, color: "#bc2323"]
                ]
            )
        }

 //       valueTile("heatIndex", "device.heatIndex", width: 2, height: 2) {
 //           state("heatIndex", label:'${currentValue}°', unit:"F",
 //               backgroundColors:[    
 //                   [value: 31, color: "#153591"],
 //                   [value: 44, color: "#1e9cbb"],
 //                   [value: 59, color: "#90d2a7"],
 //                   [value: 74, color: "#44b621"],
 //                   [value: 84, color: "#f1d801"],
 //                   [value: 95, color: "#d04e00"],
 //                   [value: 96, color: "#bc2323"]
 //               ]
 //           )
 //       }

 //       valueTile("humidity", "device.humidity", width: 2, height: 2) {
 //           state "default", label:'${currentValue}%'
 //       }

       standardTile("lastupdate", "device.lastupdate", width: 2, height: 2) {
            state "val", label:'${currentValue}', defaultState: true
        }
        
        standardTile("refresh", "device.refresh", inactiveLabel: false, decoration: "flat", width: 2, height: 2) {
            state "default", action:"refresh.refresh", icon:"st.secondary.refresh"
        }

 
        main "temperature"
        details(["temperature", "lastupdate", "refresh"])
    }
}

// handle commands
def poll() {
    log.debug "Executing 'poll'"

    getAll()
}

def refresh() {
    log.debug "Executing 'refresh'"

    getAll()
}

def getAll() {
    getTemperature()
//    getHumidity()
//    getHeatIndex()
}

def parse(String description) {
    def pair = description.split(":")

    createEvent(name: pair[0].trim(), value: pair[1].trim())
}

private getTemperature() {
    def closure = { response ->
    log.debug "this it the data: $response.data.field1 "
    log.debug "lastupdate: $response.data.created_at"
        def temp = Float.parseFloat("$response.data.field1")
        def strtemp = Float.toString(temp);
        log.debug "Temperature request was successful, $strtemp "

        //sendEvent(name: "temperature", value: response.data.result)
        sendEvent(name: "temperature", value: strtemp)
        sendEvent(name: "lastupdate", value: response.data.created_at)
    }

   	//httpGet("https://api.spark.io/v1/devices/${deviceId}/${sparkTemperatureVar}?access_token=${token}", closure)
    httpGet("https://api.thingspeak.com/channels/${channelid}/feeds/last.json?api_key=${apikey}", closure)
    //httpGet("https://api.thingspeak.com/channels/${channelid}/fields/${field/last", closure)
    
}

private getHumidity() {
    def closure = { response ->
        log.debug "Humidity request was successful, $response.data"

        sendEvent(name: "humidity", value: response.data.result)
    }

    httpGet("https://api.spark.io/v1/devices/${deviceId}/${sparkHumidityVar}?access_token=${token}", closure)
}

private getHeatIndex() {
    def closure = { response ->
        log.debug "Heat Index request was successful, $response.data"

        sendEvent(name: "heatIndex", value: response.data.result)
    }

    httpGet("https://api.spark.io/v1/devices/${deviceId}/${sparkHeatIndexVar}?access_token=${token}", closure)
}