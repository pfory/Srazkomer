#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <WiFiManager.h> 
// #include <ESP8266WebServer.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <ArduinoOTA.h>
// #include <FS.h>

#define verbose
#ifdef verbose
 #define DEBUG_PRINT(x)         Serial.print (x)
 #define DEBUG_PRINTDEC(x)      Serial.print (x, DEC)
 #define DEBUG_PRINTLN(x)       Serial.println (x)
 #define DEBUG_PRINTF(x, y)     Serial.printf (x, y)
 #define PORTSPEED 115200
#else
 #define DEBUG_PRINT(x)
 #define DEBUG_PRINTDEC(x)
 #define DEBUG_PRINTLN(x)
 #define DEBUG_PRINTF(x, y)
#endif 

//#define webserver +13 724 flash +3 812 memory
#ifdef webserver
ESP8266WebServer server(80);
#endif

#define AIO_SERVER      "192.168.1.56"
//#define AIO_SERVER      "178.77.238.20"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME    "datel"
#define AIO_KEY         "hanka12"

unsigned int volatile pulseCount     = 0;
bool pulseNow                        = false;

WiFiClient client;

const byte interruptPin = D5;

unsigned long sendDelay = 60000;
unsigned long lastSend  = sendDelay * -1;

Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

/****************************** Feeds ***************************************/
Adafruit_MQTT_Publish verSW               = Adafruit_MQTT_Publish(&mqtt, "/home/Srazkomer/esp05/VersionSW");
Adafruit_MQTT_Publish hb                  = Adafruit_MQTT_Publish(&mqtt, "/home/Srazkomer/esp05/HeartBeat");
Adafruit_MQTT_Publish pulse               = Adafruit_MQTT_Publish(&mqtt, "/home/Srazkomer/esp05/Pulse");
// Adafruit_MQTT_Publish pulseLength         = Adafruit_MQTT_Publish(&mqtt, "/home/Srazkomer/esp05/pulseLength");

// Adafruit_MQTT_Subscribe setupPulse    = Adafruit_MQTT_Subscribe(&mqtt, "/home/Srazkomer/esp05/setupPulse");
Adafruit_MQTT_Subscribe restart       = Adafruit_MQTT_Subscribe(&mqtt, "/home/Srazkomer/esp05/restart");

#define SERIALSPEED 115200

void MQTT_connect(void);

WiFiManager wifiManager;


// File f;

#ifdef webserver
void handleRoot() {
  
	// char temp[450];
	// int sec = millis() / 1000;
	// int min = sec / 60;
	// int hr = min / 60;

	// snprintf(temp, 450,
		// "<html>\
  // <head>\
    // <meta http-equiv='refresh' content='5'/>\
    // <title>ESP8266 Temperatures</title>\
    // <style>\
      // body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
    // </style>\
  // </head>\
  // <body>\
    // <h1>Teploty chata</h1>\
    // <p>Uptime: %02d:%02d:%02d</p>\
	// <p>Teplota venku: %02d.%01d&deg;C</p>\
	// <p>Teplota doma: %02d.%01d&deg;C</p>\
  // </body>\
// </html>",

// hr, min % 60, sec % 60, 0, 0
// );
	// server.send(200, "text/html", temp);
}

void handleNotFound() {
	String message = "File Not Found\n\n";
	message += "URI: ";
	message += server.uri();
	message += "\nMethod: ";
	message += (server.method() == HTTP_GET) ? "GET" : "POST";
	message += "\nArguments: ";
	message += server.args();
	message += "\n";

	for (uint8_t i = 0; i < server.args(); i++) {
		message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
	}

	server.send(404, "text/plain", message);
}
#endif

extern "C" {
  #include "user_interface.h"
}

float versionSW                   = 0.81;
String versionSWString            = "Srazkomer v";
byte heartBeat                    = 10;

// unsigned long readPulseFromFile(void);
// void writePulseToFile(uint32_t);

void setup() {
  Serial.begin(SERIALSPEED);
  DEBUG_PRINT(versionSWString);
  DEBUG_PRINTLN(versionSW);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  
  DEBUG_PRINTLN(ESP.getResetReason());
  if (ESP.getResetReason()=="Software/System restart") {
    heartBeat=11;
  } else if (ESP.getResetReason()=="Power on") {
    heartBeat=12;
  } else if (ESP.getResetReason()=="External System") {
    heartBeat=13;
  } else if (ESP.getResetReason()=="Hardware Watchdog") {
    heartBeat=14;
  } else if (ESP.getResetReason()=="Exception") {
    heartBeat=15;
  } else if (ESP.getResetReason()=="Software Watchdog") {
    heartBeat=16;
  } else if (ESP.getResetReason()=="Deep-Sleep Wake") {
    heartBeat=17;
  }
  
  
  if (!wifiManager.autoConnect("AutoConnectAP", "password")) {
    DEBUG_PRINTLN("failed to connect, we should reset as see if it connects");
    delay(3000);
    ESP.reset();
    delay(5000);
  }
	// DEBUG_PRINTLN("");
	// DEBUG_PRINT("Connected to ");
	// DEBUG_PRINTLN(ssid);
	DEBUG_PRINT("IP address: ");
	DEBUG_PRINTLN(WiFi.localIP());
  
  // SPIFFS.begin();

#ifdef webserver  
  server.on("/", handleRoot);
	server.on("/inline", []() {
		server.send(200, "text/plain", "this works as well");
	});
	server.onNotFound(handleNotFound);
	server.begin();
	DEBUG_PRINTLN("HTTP server started");
#endif

  //v klidu LOW, pulz HIGH
  pinMode(interruptPin, INPUT);
  attachInterrupt(digitalPinToInterrupt(interruptPin), pulseCountEvent, FALLING);
  digitalWrite(LED_BUILTIN, HIGH);
  
  // open config file for reading
  // if (SPIFFS.exists("/config.ini")) {
    // pulseCount = readPulseFromFile();
  // } else {
    // writePulseToFile(0);
  // }

  // mqtt.subscribe(&setupPulse);
  mqtt.subscribe(&restart);
  
    //OTA
  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname("srazkomer");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    // String type;
    // if (ArduinoOTA.getCommand() == U_FLASH)
      // type = "sketch";
    // else // U_SPIFFS
      // type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    //DEBUG_PRINTLN("Start updating " + type);
    DEBUG_PRINTLN("Start updating ");
  });
  ArduinoOTA.onEnd([]() {
   DEBUG_PRINTLN("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    DEBUG_PRINTF("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    DEBUG_PRINTF("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) DEBUG_PRINTLN("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) DEBUG_PRINTLN("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) DEBUG_PRINTLN("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) DEBUG_PRINTLN("Receive Failed");
    else if (error == OTA_END_ERROR) DEBUG_PRINTLN("End Failed");
  });
  ArduinoOTA.begin();

  DEBUG_PRINTLN(" Ready");
  //DEBUG_PRINT("IP address: ");
  //DEBUG_PRINTLN(WiFi.localIP());
  digitalWrite(LED_BUILTIN, HIGH);

  
}

void loop() {
  // Ensure the connection to the MQTT server is alive (this will make the first
  // connection and automatically reconnect when disconnected).  See the MQTT_connect
  // function definition further below.
  MQTT_connect();

  Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(5000))) {
    // if (subscription == &setupPulse) {
      // DEBUG_PRINT(F("Set new pulse to: "));
      // char *pNew = (char *)setupPulse.lastread;
      // uint32_t pCount=atol(pNew); 
      // DEBUG_PRINTLN(pCount);
      // writePulseToFile(pCount);
      // pulseCount=pCount;
      // if (! pulse.publish(pulseCount)) {
        // DEBUG_PRINTLN("failed");
      // } else {
        // DEBUG_PRINTLN("OK!");
      // }
    // }
    if (subscription == &restart) {
      char *pNew = (char *)restart.lastread;
      uint32_t pPassw=atol(pNew); 
      if (pPassw==650419) {
        DEBUG_PRINT(F("Restart ESP now!"));
        ESP.restart();
      } else {
        DEBUG_PRINT(F("Wrong password."));
      }
    }
  }

  if (millis() - lastSend>=sendDelay) {
    lastSend = millis();
    if (! verSW.publish(versionSW)) {
      DEBUG_PRINTLN("Send version SW failed");
    } else {
      DEBUG_PRINTLN("Send version SW OK!");
    }
    if (! hb.publish(heartBeat++)) {
      DEBUG_PRINTLN("Send HB failed");
    } else {
      DEBUG_PRINTLN("Send HB OK!");
    }
    if (heartBeat>1) {
      heartBeat = 0;
    }
  
    if (pulseCount>0 && pulseCount < 60) {
      // pulseNow=false;
      digitalWrite(LED_BUILTIN, LOW);
      // writePulseToFile(pulseCount);
      //DEBUG_PRINTLN(millis());
      if (! pulse.publish(pulseCount)) {
        DEBUG_PRINTLN("Send pulse failed");
      } else {
        pulseCount = 0;
        DEBUG_PRINTLN("Send pulse OK!");
      }
    }
    
    // if (! pulseLength.publish(pulseWidth)) {
      // DEBUG_PRINTLN("failed");
    // } else {
      // DEBUG_PRINTLN("OK!");
    // }
  
    digitalWrite(LED_BUILTIN, HIGH);
  }
  // ping the server to keep the mqtt connection alive
  // NOT required if you are publishing once every KEEPALIVE seconds
  /*
  if(! mqtt.ping()) {
    mqtt.disconnect();
  }
  */
  
#ifdef webserver
  server.handleClient();
#endif
  ArduinoOTA.handle();
}

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  DEBUG_PRINT("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       DEBUG_PRINTLN(mqtt.connectErrorString(ret));
       DEBUG_PRINTLN("Retrying MQTT connection in 5 seconds...");
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds
       retries--;
       if (retries == 0) {
         // basically die and wait for WDT to reset me
         while (1);
       }
  }
  DEBUG_PRINTLN("MQTT Connected!");
}

void pulseCountEvent() {
  digitalWrite(LED_BUILTIN, LOW);
  pulseCount++;
  // Serial.print(pulseCount);
  digitalWrite(LED_BUILTIN, HIGH);
}

// void writePulseToFile(uint32_t pocet) {
  // f = SPIFFS.open("/config.ini", "w");
  // if (!f) {
    // DEBUG_PRINTLN("file open failed");
  // } else {
    // DEBUG_PRINT("Zapisuji pocet pulzu ");
    // DEBUG_PRINT(pocet);
    // DEBUG_PRINT(" do souboru config.ini.");
    // f.print(pocet);
    // f.println();
    // f.close();
  // }
// }

// unsigned long readPulseFromFile() {
  // f = SPIFFS.open("/config.ini", "r");
  // if (!f) {
    // DEBUG_PRINTLN("file open failed");
    // return 0;
  // } else {
    // DEBUG_PRINTLN("====== Reading config from SPIFFS file =======");
    // String s = f.readStringUntil('\n');
    // DEBUG_PRINT("Pocet pulzu z config.ini:");
    // DEBUG_PRINTLN(s);
    // f.close();
    // return s.toInt();
  // }
// }