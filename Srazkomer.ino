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

uint32_t heartBeat                          = 0;

DoubleResetDetector drd(DRD_TIMEOUT, DRD_ADDRESS);

WiFiClient espClient;
PubSubClient client(espClient);

void ICACHE_RAM_ATTR pulseCountEvent() {
  digitalWrite(BUILTIN_LED, LOW);
  if (millis() - lastPulseMillis > 1000) {
    lastPulseMillis = millis();
    pulseCount++;
  }
  digitalWrite(BUILTIN_LED, HIGH);
}

ADC_MODE(ADC_VCC);

bool isDebugEnabled() {
#ifdef verbose
  return true;
#endif // verbose
  return false;
}

//for LED status
Ticker ticker;

auto timer = timer_create_default(); // create a timer with default settings
Timer<> default_timer; // save as above

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

void tick() {
  //toggle state
  int state = digitalRead(BUILTIN_LED);  // get the current state of GPIO1 pin
  digitalWrite(BUILTIN_LED, !state);     // set pin to the opposite state
}
  
//gets called when WiFiManager enters configuration mode
void configModeCallback (WiFiManager *myWiFiManager) {
  DEBUG_PRINTLN("Entered config mode");
  DEBUG_PRINTLN(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  DEBUG_PRINTLN(myWiFiManager->getConfigPortalSSID());
}

WiFiManager wifiManager;

void setup() {
  SERIAL_BEGIN;
  DEBUG_PRINT(F(SW_NAME));
  DEBUG_PRINT(F(" "));
  DEBUG_PRINTLN(F(VERSION));
  
  pinMode(STATUS_LED, OUTPUT);
  ticker.attach(1, tick);
    
  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP

  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wifiManager.setAPCallback(configModeCallback);
  wifiManager.setConfigPortalTimeout(CONFIG_PORTAL_TIMEOUT);
  wifiManager.setConnectTimeout(CONNECT_TIMEOUT);
  wifiManager.setBreakAfterConfig(true);
  wifiManager.setWiFiAutoReconnect(true);
  
  if (drd.detectDoubleReset()) {
    drd.stop();
    DEBUG_PRINTLN("Double reset detected, starting config portal...");
    if (!wifiManager.startConfigPortal(HOSTNAMEOTA)) {
      DEBUG_PRINTLN("Failed to connect. Use ESP without WiFi.");
    }
  }
 
  rst_info *_reset_info = ESP.getResetInfoPtr();
  uint8_t _reset_reason = _reset_info->reason;
  DEBUG_PRINT("Boot-Mode: ");
  DEBUG_PRINTLN(_reset_reason);
  heartBeat = _reset_reason;
  /*
  REASON_DEFAULT_RST             = 0      normal startup by power on 
  REASON_WDT_RST                 = 1      hardware watch dog reset 
  REASON_EXCEPTION_RST           = 2      exception reset, GPIO status won't change 
  REASON_SOFT_WDT_RST            = 3      software watch dog reset, GPIO status won't change 
  REASON_SOFT_RESTART            = 4      software restart ,system_restart , GPIO status won't change 
  REASON_DEEP_SLEEP_AWAKE        = 5      wake up from deep-sleep 
  REASON_EXT_SYS_RST             = 6      external system reset 
  */
  
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  if (!wifiManager.autoConnect(AUTOCONNECTNAME, AUTOCONNECTPWD)) { 
    DEBUG_PRINTLN("Autoconnect failed connect to WiFi. Use ESP without WiFi.");
  }   

  WiFi.printDiag(Serial);
  
  sendNetInfoMQTT();

#ifdef serverHTTP
  server.on ( "/", handleRoot );
  server.begin();
  DEBUG_PRINTLN ( "HTTP server started!!" );
#endif

#ifdef time
  DEBUG_PRINTLN("Setup TIME");
  EthernetUdp.begin(localPort);
  DEBUG_PRINT("Local port: ");
  DEBUG_PRINTLN(EthernetUdp.localPort());
  DEBUG_PRINTLN("waiting for sync");
  setSyncProvider(getNtpTime);
  setSyncInterval(300);
  
  printSystemTime();
#endif

#ifdef ota
  ArduinoOTA.setHostname(HOSTNAMEOTA);

  ArduinoOTA.onStart([]() {
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
#endif
 
  //v klidu LOW, pulz HIGH
  /*It have the output state light, if output high level, the lights are off. if output low level ,it on.
  If it covered,it will output high level;otherwise it output low level.*/
  pinMode(INTERRUPTPIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(INTERRUPTPIN), pulseCountEvent, RISING);
  
  //setup timers
  timer.every(SENDSTAT_DELAY, sendStatisticMQTT);
  timer.every(CONNECT_DELAY, reconnect);

  void * a;
  sendStatisticMQTT(a);
  reconnect(a);
 
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

void startConfigPortal(void) {
  DEBUG_PRINTLN("START config portal");
  wifiManager.setConfigPortalBlocking(false);
  wifiManager.startConfigPortal(HOSTNAMEOTA);
}

void stopConfigPortal(void) {
  DEBUG_PRINTLN("STOP config portal");
  wifiManager.stopConfigPortal();
}

bool sendDataMQTT() {
  digitalWrite(BUILTIN_LED, LOW);
  DEBUG_PRINTLN(F("Data"));
  
  SenderClass sender;
  
  sender.add("Pulse", pulseCount );

  DEBUG_PRINTLN(F("Calling MQTT"));

  if (sender.sendMQTT(mqtt_server, mqtt_port, mqtt_username, mqtt_key, mqtt_base)) {
      pulseCount = 0;
  }
  
  digitalWrite(BUILTIN_LED, HIGH);
  return true;
}

bool sendStatisticMQTT(void *) {
  digitalWrite(BUILTIN_LED, LOW);
  DEBUG_PRINTLN(F("Statistic"));

  SenderClass sender;
  sender.add("VersionSW", VERSION);
  sender.add("Napeti",  ESP.getVcc());
  sender.add("HeartBeat", heartBeat++);
  sender.add("RSSI", WiFi.RSSI());
  DEBUG_PRINTLN(F("Calling MQTT"));
  
  sender.sendMQTT(mqtt_server, mqtt_port, mqtt_username, mqtt_key, mqtt_base);
  digitalWrite(BUILTIN_LED, HIGH);
  return true;
}

void sendNetInfoMQTT() {
  digitalWrite(BUILTIN_LED, LOW);
  DEBUG_PRINTLN(F("Net info"));

  SenderClass sender;
  sender.add("IP",              WiFi.localIP().toString().c_str());
  sender.add("MAC",             WiFi.macAddress());
  
  DEBUG_PRINTLN(F("Calling MQTT"));
  
  sender.sendMQTT(mqtt_server, mqtt_port, mqtt_username, mqtt_key, mqtt_base);
  digitalWrite(BUILTIN_LED, HIGH);
  return;
}


void printDigits(int digits){
  // utility function for digital clock display: prints preceding
  // colon and leading 0
  DEBUG_PRINT(":");
  if(digits < 10)
    DEBUG_PRINT('0');
  DEBUG_PRINT(digits);
}

// void printSystemTime(){
// #ifdef time
  // DEBUG_PRINT(day());
  // DEBUG_PRINT(".");
  // DEBUG_PRINT(month());
  // DEBUG_PRINT(".");
  // DEBUG_PRINT(year());
  // DEBUG_PRINT(" ");
  // DEBUG_PRINT(hour());
  // printDigits(minute());
  // printDigits(second());
// #endif
// }

bool reconnect(void *) {
  if (!client.connected()) {
    DEBUG_PRINT("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(mqtt_base, mqtt_username, mqtt_key)) {
      client.subscribe((String(mqtt_base) + "/#").c_str());
      DEBUG_PRINTLN("connected");
    } else {
      DEBUG_PRINT("failed, rc=");
      DEBUG_PRINTLN(client.state());
    }
  }
  return true;
}