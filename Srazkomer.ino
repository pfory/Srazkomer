//SRAZKOMER
/*
*/
//Wemos D1 R2 & mini

#include "configuration.h"

#ifdef serverHTTP
ESP8266WebServer server(80);
#endif

unsigned int volatile pulseCount            = 0;
unsigned long lastPulseMillis               = 0;


void ICACHE_RAM_ATTR pulseCountEvent() {
  digitalWrite(BUILTIN_LED, LOW);
  if (millis() - lastPulseMillis > 1000) {
    lastPulseMillis = millis();
    pulseCount++;
  }
  digitalWrite(BUILTIN_LED, HIGH);
}

ADC_MODE(ADC_VCC);

//MQTT callback
void callback(char* topic, byte* payload, unsigned int length) {
  char * pEnd;
  String val =  String();
  DEBUG_PRINT("\nMessage arrived [");
  DEBUG_PRINT(topic);
  DEBUG_PRINT("] ");
  for (int i=0;i<length;i++) {
    DEBUG_PRINT((char)payload[i]);
    val += (char)payload[i];
  }
  DEBUG_PRINTLN();
 
  if (strcmp(topic, "/home/Srazkomer/esp05/restart")==0) {
    DEBUG_PRINT("RESTART");
    ESP.restart();
  } else if (strcmp(topic, (String(mqtt_base) + "/" + String(mqtt_topic_netinfo)).c_str())==0) {
    DEBUG_PRINT("NET INFO");
    sendNetInfoMQTT();
  } else if (strcmp(topic, (String(mqtt_base) + "/" + String(mqtt_config_portal)).c_str())==0) {
    startConfigPortal();
  } else if (strcmp(topic, (String(mqtt_base) + "/" + String(mqtt_config_portal_stop)).c_str())==0) {
    stopConfigPortal();
  }
}


#ifdef serverHTTP
void handleRoot() {
  digitalWrite(BUILTIN_LED, LOW);
	char temp[600];
  digitalWrite(BUILTIN_LED, HIGH);
}
#endif

void setup() {
  preSetup();
  
  //v klidu LOW, pulz HIGH
  /*It have the output state light, if output high level, the lights are off. if output low level ,it on.
  If it covered,it will output high level;otherwise it output low level.*/
  pinMode(INTERRUPTPIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(INTERRUPTPIN), pulseCountEvent, RISING);

#ifdef timers  
  //setup timers
  timer.every(SENDSTAT_DELAY, sendStatisticMQTT);
  timer.every(CONNECT_DELAY, reconnect);
#endif

  void * a;
  reconnect(a);
  sendStatisticMQTT(a);
  sendNetInfoMQTT();
  
  ticker.detach();
  //keep LED on
  digitalWrite(BUILTIN_LED, HIGH);

  drd.stop();

  DEBUG_PRINTLN(F("SETUP END......................."));
}

void loop() {
#ifdef timers
  timer.tick(); // tick the timer
#endif

#ifdef serverHTTP
  server.handleClient();
#endif

  if (pulseCount>0) {
    sendDataMQTT();
  }
  
  client.loop();
  wifiManager.process();

#ifdef ota
  ArduinoOTA.handle();
#endif
}

bool sendDataMQTT() {
  digitalWrite(BUILTIN_LED, LOW);
  DEBUG_PRINTLN(F("Data"));
  
  client.publish((String(mqtt_base) + "/Pulse").c_str(), String(pulseCount).c_str());
 
  DEBUG_PRINTLN(F("Calling MQTT"));

  pulseCount = 0;
 
  digitalWrite(BUILTIN_LED, HIGH);
  return true;
}

bool reconnect(void *) {
  if (!client.connected()) {
    DEBUG_PRINT("Attempting MQTT connection...");
    // Attempt to connect
     if (client.connect(mqtt_base, mqtt_username, mqtt_key, (String(mqtt_base) + "/LWT").c_str(), 2, true, "Dead", false)) {
      client.subscribe((String(mqtt_base) + "/#").c_str());
      client.publish((String(mqtt_base) + "/connected").c_str(), "");
      DEBUG_PRINTLN("connected");
    } else {
      DEBUG_PRINT("failed, rc=");
      DEBUG_PRINTLN(client.state());
    }
  }
  return true;
}