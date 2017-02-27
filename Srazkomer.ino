#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <FS.h>

const char *ssid = "Datlovo";
const char *password = "Nu6kMABmseYwbCoJ7LyG";

//#define webserver +13 724 flash +3 812 memory
#ifdef webserver
ESP8266WebServer server(80);
#endif

#define AIO_SERVER      "192.168.1.56"
//#define AIO_SERVER      "178.77.238.20"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME    "datel"
#define AIO_KEY         "hanka12"

uint32_t pulseCount     = 0;
uint32_t pulseWidth     = 0;
bool pulseNow           = false;
uint32_t pulseMillisOld = 0;

WiFiClient client;

const byte ledPin       = 0;
const byte interruptPin = 2;

unsigned long sendDelay = 60000;
unsigned long lastSend  = sendDelay * -1;

Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

/****************************** Feeds ***************************************/
Adafruit_MQTT_Publish verSW               = Adafruit_MQTT_Publish(&mqtt, "/home/Srazkomer/esp05/VersionSW");
Adafruit_MQTT_Publish hb                  = Adafruit_MQTT_Publish(&mqtt, "/home/Srazkomer/esp05/HeartBeat");
Adafruit_MQTT_Publish pulse               = Adafruit_MQTT_Publish(&mqtt, "/home/Srazkomer/esp05/Pulse");
Adafruit_MQTT_Publish pulseLength         = Adafruit_MQTT_Publish(&mqtt, "/home/Srazkomer/esp05/pulseLength");

Adafruit_MQTT_Subscribe setupPulse    = Adafruit_MQTT_Subscribe(&mqtt, "/home/Srazkomer/esp05/setupPulse");
Adafruit_MQTT_Subscribe restart       = Adafruit_MQTT_Subscribe(&mqtt, "/home/Srazkomer/esp05/restart");

#define SERIALSPEED 115200

void MQTT_connect(void);

File f;

#ifdef webserver
void handleRoot()
{
  
	char temp[450];
	int sec = millis() / 1000;
	int min = sec / 60;
	int hr = min / 60;

	snprintf(temp, 450,
		"<html>\
  <head>\
    <meta http-equiv='refresh' content='5'/>\
    <title>ESP8266 Temperatures</title>\
    <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
    </style>\
  </head>\
  <body>\
    <h1>Teploty chata</h1>\
    <p>Uptime: %02d:%02d:%02d</p>\
	<p>Teplota venku: %02d.%01d&deg;C</p>\
	<p>Teplota doma: %02d.%01d&deg;C</p>\
  </body>\
</html>",

hr, min % 60, sec % 60, 0, 0
);
	server.send(200, "text/html", temp);
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

float versionSW                   = 0.62;
String versionSWString            = "Srazkomer v";
byte heartBeat                    = 10;

void setup() {
  Serial.begin(SERIALSPEED);
  Serial.print(versionSWString);
  Serial.println(versionSW);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);
  
  Serial.println(ESP.getResetReason());
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
  
  //Serial.println(ESP.getFlashChipRealSize);
  //Serial.println(ESP.getCpuFreqMHz);
  WiFi.begin(ssid, password);

	// Wait for connection
	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print(".");
	}

	Serial.println("");
	Serial.print("Connected to ");
	Serial.println(ssid);
	Serial.print("IP address: ");
	Serial.println(WiFi.localIP());
  
  SPIFFS.begin();

#ifdef webserver  
  server.on("/", handleRoot);
	server.on("/inline", []() {
		server.send(200, "text/plain", "this works as well");
	});
	server.onNotFound(handleNotFound);
	server.begin();
	Serial.println("HTTP server started");
#endif

  //v klidu 0, kladny pulz po dobu xx ms
  pinMode(interruptPin, INPUT);
  attachInterrupt(digitalPinToInterrupt(interruptPin), pulseCountEvent, CHANGE);
  digitalWrite(ledPin, LOW);
  
  // open config file for reading
  if (SPIFFS.exists("/config.ini")) {
    pulseCount = readPulseFromFile();
  } else {
    writePulseToFile(0);
  }

  mqtt.subscribe(&setupPulse);
  mqtt.subscribe(&restart);
}

void loop() {
  // Ensure the connection to the MQTT server is alive (this will make the first
  // connection and automatically reconnect when disconnected).  See the MQTT_connect
  // function definition further below.
  MQTT_connect();

  Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(5000))) {
    if (subscription == &setupPulse) {
      Serial.print(F("Set new pulse to: "));
      char *pNew = (char *)setupPulse.lastread;
      uint32_t pCount=atol(pNew); 
      Serial.println(pCount);
      writePulseToFile(pCount);
      pulseCount=pCount;
      if (! pulse.publish(pulseCount)) {
        Serial.println("failed");
      } else {
        Serial.println("OK!");
      }
    }
    if (subscription == &restart) {
      char *pNew = (char *)restart.lastread;
      uint32_t pPassw=atol(pNew); 
      if (pPassw==650419) {
        Serial.print(F("Restart ESP now!"));
        ESP.restart();
      } else {
        Serial.print(F("Wrong password."));
      }
    }
  }

  if (millis() - lastSend>=sendDelay) {
    lastSend = millis();
    if (! verSW.publish(versionSW)) {
      Serial.println("failed");
    } else {
      Serial.println("OK!");
    }
    if (! hb.publish(heartBeat++)) {
      Serial.println("failed");
    } else {
      Serial.println("OK!");
    }
    if (heartBeat>1) {
      heartBeat = 0;
    }
    if (! pulse.publish(pulseCount)) {
      Serial.println("failed");
    } else {
      Serial.println("OK!");
    }

    if (! pulse.publish(pulseWidth)) {
      Serial.println("failed");
    } else {
      Serial.println("OK!");
    }
    
  }
  
  if (pulseNow) {
    pulseNow=false;
    digitalWrite(ledPin, HIGH);
    writePulseToFile(pulseCount);
    //Serial.println(millis());
    if (! pulse.publish(pulseCount)) {
      Serial.println("failed");
    } else {
      Serial.println("OK!");
    }
    
    if (! pulse.publish(pulseWidth)) {
      Serial.println("failed");
    } else {
      Serial.println("OK!");
    }
  
    digitalWrite(ledPin, LOW);
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
}

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
       Serial.println(mqtt.connectErrorString(ret));
       Serial.println("Retrying MQTT connection in 5 seconds...");
       mqtt.disconnect();
       delay(5000);  // wait 5 seconds
       retries--;
       if (retries == 0) {
         // basically die and wait for WDT to reset me
         while (1);
       }
  }
  Serial.println("MQTT Connected!");
}

void pulseCountEvent() {
  if (digitalRead(interruptPin)==LOW) {
    pulseWidth = millis() - pulseMillisOld
    if (pulseWidth>50) {
      pulseCount++;
      pulseMillisOld = millis();
      Serial.println(pulseCount);
      pulseNow=true;
    }
  else {
    pulseMillisOld = millis();
  }
}

void writePulseToFile(uint32_t pocet) {
  f = SPIFFS.open("/config.ini", "w");
  if (!f) {
    Serial.println("file open failed");
  } else {
    Serial.print("Zapisuji pocet pulzu ");
    Serial.print(pocet);
    Serial.print(" do souboru config.ini.");
    f.print(pocet);
    f.println();
    f.close();
  }
}

unsigned long readPulseFromFile() {
  f = SPIFFS.open("/config.ini", "r");
  if (!f) {
    Serial.println("file open failed");
    return 0;
  } else {
    Serial.println("====== Reading config from SPIFFS file =======");
    String s = f.readStringUntil('\n');
    Serial.print("Pocet pulzu z config.ini:");
    Serial.println(s);
    f.close();
    return s.toInt();
  }
}