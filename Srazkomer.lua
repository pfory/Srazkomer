--Init  
base = "/home/Srazkomer/esp05/"
deviceID = "ESP8266 Srazkomer "..node.chipid()

pulseTotal        = 0
pulseDuration     = 0
heartBeat         = node.bootreason() + 10

wifi.setmode(wifi.STATION)
wifi.sta.config("Datlovo","Nu6kMABmseYwbCoJ7LyG")
cfg={
  ip = "192.168.1.150",
  netmask = "255.255.255.0",
  gateway = "192.168.1.1"
}
wifi.sta.setip(cfg)
wifi.sta.autoconnect(1)

Broker="88.146.202.186"  

pinLed = 4
gpio.mode(pinLed,gpio.OUTPUT)  
gpio.write(pinLed,gpio.HIGH)  

versionSW         = 0.4
versionSWString   = "Srazkomer v" 
print(versionSWString .. versionSW)

pin = 3
gpio.mode(pin,gpio.INT)

function pinPulse(level)
  if gpio.read(pin) == gpio.LOW then
  --if level == gpio.HIGH then --nabezna hrana
    --tmr.delay(10000)
    pulseDuration = tmr.now()
    gpio.write(pinLed,gpio.HIGH)  
--    print("nabezna")
  else 
    if (tmr.now() - pulseDuration) > 20000 and (tmr.now() - pulseDuration) < 100000 then
      pulseTotal=pulseTotal+1
      gpio.write(pinLed,gpio.LOW) 
--      print("dobezna")
      sendData()
    end
  end
end


function sendData()
  uart.write(0,"I am sending pulse to OpenHab:"..pulseTotal.." pulse(s).\n\r")
  m:publish(base.."Pulse",       pulseTotal,0,0)  

  file.open("config.ini", "w")
  file.write(string.format("%u", pulseTotal))
  file.write("\n\r")
  file.close()
  sendHB()
end

function sendHB()
  print("I am sending HB to OpenHab")
  m:publish(base.."HeartBeat",   heartBeat,0,0)
 
  if heartBeat==0 then heartBeat=1
  else heartBeat=0
  end
end


function mqtt_sub()  
  m:subscribe(base.."com",0, function(conn)   
    print("Mqtt Subscribed to OpenHAB feed for device "..deviceID)  
  end)  
end

function reconnect()
  print ("Waiting for Wifi")
  if wifi.sta.status() == 5 and wifi.sta.getip() ~= nil then 
    print ("Wifi Up!")
    tmr.stop(1) 
    m:connect(Broker, 31883, 0, function(conn) 
      print(wifi.sta.getip())
      print("Mqtt Connected to:" .. Broker) 
      mqtt_sub() --run the subscription function 
    end)
  end
  heartBeat=20
  sendHB()
end


m = mqtt.Client(deviceID, 180, "datel", "hanka12")  
m:lwt("/lwt", deviceID, 0, 0)  
m:on("offline", function(con)   
  print ("Mqtt Reconnecting...")   
  tmr.alarm(1, 10000, 1, function()  
    reconnect()
  end)
end)  


 -- on publish message receive event  
m:on("message", function(conn, topic, data)   
  print("Received:" .. topic .. ":" .. data) 
  if topic == base.."com" then
    if data == "ON" then
      print("Restarting ESP, bye.")
      node.restart()
    end
  end
end)  


uart.write(0,"Connecting to Wifi")
tmr.alarm(0, 1000, 1, function() 
  uart.write(0,".")
  if wifi.sta.status() == 5 and wifi.sta.getip() ~= nil then 
    print ("Wifi connected")
    print(wifi.sta.getmac())
    tmr.stop(0) 
    m:connect(Broker, 31883, 0, 1, function(conn) 
      mqtt_sub() --run the subscription function 
      print(wifi.sta.getip())
      print("Mqtt Connected to:" .. Broker.." - "..base) 
      gpio.trig(pin, "both", pinPulse)
      uart.write(0,"Read config file... ")
      file.open("config.ini", "r")
      s = file.readline()
      if (s==nil) then
        uart.write(0,"empty file.")
        pulseTotal=0
      else
        pulseTotal = tonumber(s)
      end
      uart.write(0,pulseTotal.." pulse(s).\n\r")
      file.close()  
      m:publish(base.."VersionSW",   versionSW,0,0)  
      sendHB() 
      tmr.alarm(0, 60000, tmr.ALARM_AUTO, function()
        sendHB() 
      end)
    end) 
  end
end)