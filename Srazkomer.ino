//SRAZKOMER
/*
*/
//Wemos D1 R2 & mini

#include "configuration.h"

#ifdef serverHTTP
#include <ESP8266WebServer.h>
ESP8266WebServer server(80);
#endif

unsigned int volatile pulseCount            = 0;
unsigned long lastPulseMillis               = 0;
bool pulseNow                               = false;

unsigned long milisLastRunMinOld            = 0;

const unsigned long   sendStatDelay         = 60000;

uint32_t heartBeat                          = 0;

DoubleResetDetector drd(DRD_TIMEOUT, DRD_ADDRESS);

void ICACHE_RAM_ATTR pulseCountEvent() {
  digitalWrite(BUILTIN_LED, LOW);
  if (millis() - lastPulseMillis > 5000) {
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
#include <Ticker.h>
Ticker ticker;


#include <timer.h>
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
  }
}


#ifdef serverHTTP
void handleRoot() {
	char temp[600];
  // DEBUG_PRINT(year());
  // DEBUG_PRINT(month());
  // DEBUG_PRINT(day());
  // DEBUG_PRINT(hour());
  // DEBUG_PRINT(minute());
  // DEBUG_PRINT(second());
  printSystemTime();
  DEBUG_PRINTLN(" Client request");
  digitalWrite(BUILTIN_LED, LOW);
  
	// snprintf ( temp, 400,
      // "<html>\
        // <head>\
          // <meta charset='UTF-8'>\
        // </head>\
        // <body>\
          // T2899BDCF02000076,%4d-%02d-%02dT%02d:%02d:%02d.000000Z,%s%d.%02d<br />\
          // Humidity,%4d-%02d-%02dT%02d:%02d:%02d.000000Z,%d.00<br />\
          // Press,%4d-%02d-%02dT%02d:%02d:%02d.000000Z,%d.00<br />\
          // DewPoint,%4d-%02d-%02dT%02d:%02d:%02d.000000Z,%s%d.%02d<br />\
        // </body>\
      // </html>",
      // year(), month(), day(), hour(), minute(), second(),
      // temperature<0 && temperature>-1 ? "-":"",
      // (int)temperature, 
      // abs((temperature - (int)temperature) * 100),
      // year(), month(), day(), hour(), minute(), second(),
      // (int)humidity,
      // year(), month(), day(), hour(), minute(), second(),
      // (int)pressure,
      // year(), month(), day(), hour(), minute(), second(),
      // dewPoint<0 && dewPoint>-1 ? "-":"",
      // (int)dewPoint, 
      // abs((dewPoint - (int)dewPoint) * 100)
	// );
	server.send ( 200, "text/html", temp );
  digitalWrite(BUILTIN_LED, HIGH);
}
#endif

void tick()
{
  //toggle state
  int state = digitalRead(BUILTIN_LED);  // get the current state of GPIO1 pin
  digitalWrite(BUILTIN_LED, !state);     // set pin to the opposite state
}
  
//flag for saving data
bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback () {
  DEBUG_PRINTLN("Should save config");
  shouldSaveConfig = true;
}

//gets called when WiFiManager enters configuration mode
void configModeCallback (WiFiManager *myWiFiManager) {
  DEBUG_PRINTLN("Entered config mode");
  DEBUG_PRINTLN(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  DEBUG_PRINTLN(myWiFiManager->getConfigPortalSSID());
  //entered config mode, make led toggle faster
  ticker.attach(0.2, tick);
}

WiFiClient espClient;
PubSubClient client(espClient);

WiFiManager wifiManager;

void ICACHE_RAM_ATTR handleInterrupt();

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
  
    if (drd.detectDoubleReset()) {
    DEBUG_PRINTLN("Double reset detected, starting config portal...");
    ticker.attach(0.2, tick);
    if (!wifiManager.startConfigPortal(HOSTNAMEOTA)) {
      DEBUG_PRINTLN("failed to connect and hit timeout");
      delay(3000);
      //reset and try again, or maybe put it to deep sleep
      ESP.reset();
      delay(5000);
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

  //reset settings - for testing
  //wifiManager.resetSettings();
  
  WiFi.printDiag(Serial);
    
  if (!wifiManager.autoConnect(AUTOCONNECTNAME, AUTOCONNECTPWD)) { 
    DEBUG_PRINTLN("failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(5000);
  } 

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
  //OTA
  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname(HOSTNAMEOTA);

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
#endif
 
  //v klidu LOW, pulz HIGH
  /*It have the output state light, if output high level, the lights are off. if output low level ,it on.
  If it covered,it will output high level;otherwise it output low level.*/
  pinMode(INTERRUPTPIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(INTERRUPTPIN), pulseCountEvent, RISING);
  
  //setup timers
  timer.every(sendStatDelay, sendStatisticHA);

  void * a;
  sendStatisticHA(a);
 
  ticker.detach();
  //keep LED on
  digitalWrite(BUILTIN_LED, HIGH);
  DEBUG_PRINTLN(" Ready");
}

void loop() {
#ifdef timers
  timer.tick(); // tick the timer
#endif

#ifdef serverHTTP
  server.handleClient();
#endif

  if (pulseCount>0) {
    sendDataHA();
  }
  
  reconnect();
  client.loop();

#ifdef ota
  ArduinoOTA.handle();
#endif
}

bool sendDataHA() {
  noInterrupts();
  digitalWrite(BUILTIN_LED, LOW);
  printSystemTime();
  DEBUG_PRINTLN(F("Data"));
  
  SenderClass sender;
  
  sender.add("Pulse", pulseCount );

  DEBUG_PRINTLN(F("Calling MQTT"));

  if (sender.sendMQTT(mqtt_server, mqtt_port, mqtt_username, mqtt_key, mqtt_base)) {
      pulseCount = 0;
  }
  
  digitalWrite(BUILTIN_LED, HIGH);
  interrupts();
  return true;
}

bool sendStatisticHA(void *) {
  digitalWrite(BUILTIN_LED, LOW);
  printSystemTime();
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
  printSystemTime();
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

void printSystemTime(){
#ifdef time
  DEBUG_PRINT(day());
  DEBUG_PRINT(".");
  DEBUG_PRINT(month());
  DEBUG_PRINT(".");
  DEBUG_PRINT(year());
  DEBUG_PRINT(" ");
  DEBUG_PRINT(hour());
  printDigits(minute());
  printDigits(second());
#endif
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
     if (lastConnectAttempt == 0 || lastConnectAttempt + connectDelay < millis()) {
    DEBUG_PRINT("Attempting MQTT connection...");
      // Attempt to connect
      if (client.connect(mqtt_base, mqtt_username, mqtt_key)) {
        DEBUG_PRINTLN("connected");
        client.subscribe((String(mqtt_base) + "/#").c_str());
      } else {
        lastConnectAttempt = millis();
        DEBUG_PRINT("failed, rc=");
        DEBUG_PRINTLN(client.state());
      }
    }
  }
}