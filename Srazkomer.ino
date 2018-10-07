//WEMOS D1 mini

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <WiFiManager.h> 
// #include <ESP8266WebServer.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <ArduinoOTA.h>

//for LED status
#include <Ticker.h>
Ticker ticker;

void tick()
{
  //toggle state
  int state = digitalRead(BUILTIN_LED);  // get the current state of GPIO1 pin
  digitalWrite(BUILTIN_LED, !state);     // set pin to the opposite state
}

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

unsigned int volatile pulseCount          = 0;
unsigned long lastPulseMillis             = 0;
bool pulseNow                             = false;


WiFiClient client;

const byte interruptPin = D5;

unsigned long sendDelay = 60000;
unsigned long lastSend  = sendDelay * -1;

Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

/****************************** Feeds ***************************************/
Adafruit_MQTT_Publish verSW               = Adafruit_MQTT_Publish(&mqtt, "/home/Srazkomer/esp05/VersionSW");
Adafruit_MQTT_Publish hb                  = Adafruit_MQTT_Publish(&mqtt, "/home/Srazkomer/esp05/HeartBeat");
Adafruit_MQTT_Publish pulse               = Adafruit_MQTT_Publish(&mqtt, "/home/Srazkomer/esp05/Pulse");
Adafruit_MQTT_Publish napeti              = Adafruit_MQTT_Publish(&mqtt, "/home/Srazkomer/esp05/Napeti");
// Adafruit_MQTT_Publish pulseLength         = Adafruit_MQTT_Publish(&mqtt, "/home/Srazkomer/esp05/pulseLength");

// Adafruit_MQTT_Subscribe setupPulse    = Adafruit_MQTT_Subscribe(&mqtt, "/home/Srazkomer/esp05/setupPulse");
Adafruit_MQTT_Subscribe restart       = Adafruit_MQTT_Subscribe(&mqtt, "/home/Srazkomer/esp05/restart");


IPAddress _ip           = IPAddress(192, 168, 1, 107);
IPAddress _gw           = IPAddress(192, 168, 1, 1);
IPAddress _sn           = IPAddress(255, 255, 255, 0);


void MQTT_connect(void);

WiFiManager wifiManager;

ADC_MODE(ADC_VCC);

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

//gets called when WiFiManager enters configuration mode
void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
  //entered config mode, make led toggle faster
  ticker.attach(0.2, tick);
}

float versionSW                   = 0.92;
String versionSWString            = "Srazkomer v";
uint32_t heartBeat                = 0;

void setup() {
#ifdef verbose
  Serial.begin(PORTSPEED);
#endif
  DEBUG_PRINT(versionSWString);
  DEBUG_PRINTLN(versionSW);
  //set led pin as output
  pinMode(BUILTIN_LED, OUTPUT);
  // start ticker with 0.5 because we start in AP mode and try to connect
  ticker.attach(0.6, tick);

  DEBUG_PRINTLN(ESP.getResetReason());
  if (ESP.getResetReason()=="Software/System restart") {
    heartBeat=1;
  } else if (ESP.getResetReason()=="Power on") {
    heartBeat=2;
  } else if (ESP.getResetReason()=="External System") {
    heartBeat=3;
  } else if (ESP.getResetReason()=="Hardware Watchdog") {
    heartBeat=4;
  } else if (ESP.getResetReason()=="Exception") {
    heartBeat=5;
  } else if (ESP.getResetReason()=="Software Watchdog") {
    heartBeat=6;
  } else if (ESP.getResetReason()=="Deep-Sleep Wake") {
    heartBeat=7;
  }

  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  //reset settings - for testing
  //wifiManager.resetSettings();
  wifiManager.setConnectTimeout(600); //5min

  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wifiManager.setAPCallback(configModeCallback);
  
  wifiManager.setSTAStaticIPConfig(_ip, _gw, _sn);

  if (!wifiManager.autoConnect("Srazkomer", "password")) {
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
  /*It have the output state light, if output high level, the lights are off. if output low level ,it on.
  If it covered,it will output high level;otherwise it output low level.*/
  pinMode(interruptPin, INPUT);
  attachInterrupt(digitalPinToInterrupt(interruptPin), pulseCountEvent, RISING);
  
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
  ticker.detach();
  digitalWrite(BUILTIN_LED, HIGH);
}

void loop() {
  // Ensure the connection to the MQTT server is alive (this will make the first
  // connection and automatically reconnect when disconnected).  See the MQTT_connect
  // function definition further below.
  MQTT_connect();

  Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(5000))) {
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
    if (! hb.publish(heartBeat)) {
      DEBUG_PRINTLN("Send HB failed");
    } else {
      DEBUG_PRINTLN("Send HB OK!");
    }
    heartBeat++;
    if (! napeti.publish(ESP.getVcc())) {
      DEBUG_PRINTLN("Send napeti failed");
    } else {
      DEBUG_PRINTLN("Send napeti OK!");
    }

    if (pulseCount>0) {
      digitalWrite(BUILTIN_LED, LOW);
      if (! pulse.publish(pulseCount)) {
        DEBUG_PRINTLN("Send pulse failed");
      } else {
        pulseCount = 0;
        DEBUG_PRINTLN("Send pulse OK!");
      }
    } else {
        pulseCount = 0;
    }
    digitalWrite(BUILTIN_LED, HIGH);
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
  digitalWrite(BUILTIN_LED, LOW);
  if (millis() - lastPulseMillis > 5000) {
    lastPulseMillis = millis();
    pulseCount++;
  }
  digitalWrite(BUILTIN_LED, HIGH);
}
