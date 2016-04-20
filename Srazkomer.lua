--Init  
base = "/home/Srazkomer/esp05/"
deviceID = "ESP8266 Srazkomer "..node.chipid()

pulseTotal        = 0
heartBeat         = node.bootreason()
pulseDuration     = 0

wifi.setmode(wifi.STATION)
wifi.sta.config("Datlovo","Nu6kMABmseYwbCoJ7LyG")
wifi.sta.autoconnect(1)

Broker="88.146.202.186"  

pinLed = 3
gpio.mode(pinLed,gpio.OUTPUT)  
gpio.write(pinLed,gpio.LOW)  

versionSW         = 0.1
versionSWString   = "Srazkomer v" 
print(versionSWString .. versionSW)

pin = 4
gpio.mode(pin,gpio.INT)

function pinPulse(level)
  if gpio.read(pin) == gpio.HIGH then
  --if level == gpio.HIGH then --nabezna hrana
    --tmr.delay(10000)
    pulseDuration = tmr.now()
    gpio.write(pinLed,gpio.HIGH)  
    print("nabezna")
  else 
    if (tmr.now() - pulseDuration) > 70000 and (tmr.now() - pulseDuration) < 100000 then
      pulseTotal=pulseTotal+1
      gpio.write(pinLed,gpio.LOW) 
      print("dobezna")
      sendData()
    end
  end
end


function sendData()
  print("I am sending pulse to OpenHab")
  print(pulseTotal)
  m:publish(base.."Pulse",       pulseTotal,0,0)  
  m:publish(base.."VersionSW",   versionSW,0,0)  
  m:publish(base.."HeartBeat",   heartBeat,0,0)

  file.open("config.ini", "w+")
  file.write(string.format("%u", pulseTotal))
  file.write("\n\r")
  file.close()
  
  if heartBeat==0 then heartBeat=1
  else heartBeat=0
  end
end

function mqtt_sub()  
  m:subscribe(base,0, function(conn)   
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
end)  

tmr.alarm(0, 1000, 1, function() 
  print ("Connecting to Wifi... ")
  if wifi.sta.status() == 5 and wifi.sta.getip() ~= nil then 
    print ("Wifi connected")
    tmr.stop(0) 
    m:connect(Broker, 31883, 0, function(conn) 
      mqtt_sub() --run the subscription function 
      print(wifi.sta.getip())
      print("Mqtt Connected to:" .. Broker.." - "..base) 
      gpio.trig(pin, "both", pinPulse)
      print ("Read config file... ")
      file.open("config.ini", "r")
      s = file.readline()
      if (s==nil) then
        print("empty file")
        pulseTotal=0
      else
        pulseTotal = s
      end
      print(pulseTotal)
      file.close()  
    end) 
  end
end)