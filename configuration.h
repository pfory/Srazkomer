#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <ESP8266WiFi.h>
#include <WiFiManager.h> 
#include <FS.h>          //this needs to be first
#include <Ticker.h>
#include <ArduinoJson.h> //https://github.com/bblanchon/ArduinoJson
#include "DoubleResetDetector.h" // https://github.com/datacute/DoubleResetDetector
#include "Sender.h"
#include <Wire.h>

//SW name & version
#define     VERSION                          "1.50"
#define     SW_NAME                          "Srazkomer"

//HW
//Wemos D1 R2 & mini 

#define timers
#define ota
#define verbose
//#define time
//#define serverHTTP

#define AUTOCONNECTNAME   HOSTNAMEOTA
#define AUTOCONNECTPWD    "password"

#ifdef ota
#include <ArduinoOTA.h>
#define HOSTNAMEOTA   SW_NAME VERSION
#endif

#ifdef verbose
  #define DEBUG_PRINT(x)                     Serial.print (x)
  #define DEBUG_PRINTDEC(x)                  Serial.print (x, DEC)
  #define DEBUG_PRINTLN(x)                   Serial.println (x)
  #define DEBUG_PRINTF(x, y)                 Serial.printf (x, y)
  #define PORTSPEED 115200             
  #define DEBUG_WRITE(x)                     Serial.write (x)
  #define DEBUG_PRINTHEX(x)                  Serial.print (x, HEX)
  #define SERIAL_BEGIN                       Serial.begin(PORTSPEED)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTDEC(x)
  #define DEBUG_PRINTLN(x)
  #define DEBUG_PRINTF(x, y)
  #define DEBUG_WRITE(x)
#endif 


//All of the IO pins have interrupt/pwm/I2C/one-wire support except D0.
#define STATUS_LED                          BUILTIN_LED //status LED
#define LED2PIN                             D3 //stav rele, 10k Pull-up     GPIO00
#define RELAY1PIN                           D8 //pin rele 1                 GPIO16
#define RELAY2PIN                           D6 //pin rele 2, MISO           GPIO12
#define PIRPIN                              D5 //pin pir sensoru, SCK       GPIO14
//SDA                                       D2 //                           GPIO04
//SCL                                       D1 //                           GPIO05
//BUILTIN_LED                               D4 //10k Pull-up, BUILTIN_LED   GPIO02
#define INTERRUPTPIN                        2


static const char* const      mqtt_server                    = "192.168.1.56";
static const uint16_t         mqtt_port                      = 1883;
static const char* const      mqtt_username                  = "datel";
static const char* const      mqtt_key                       = "hanka12";
static const char* const      mqtt_base                      = "/home/Srazkomer/esp05";
// static const char* const      static_ip                      = "192.168.1.107";
// static const char* const      static_gw                      = "192.168.1.1";
// static const char* const      static_sn                      = "255.255.255.0";
static const char* const      mqtt_topic_restart             = "restart";
static const char* const      mqtt_topic_netinfo             = "netinfo";


uint32_t              connectDelay                = 30000; //30s
uint32_t              lastConnectAttempt          = 0; 

// Number of seconds after reset during which a
// subseqent reset will be considered a double reset.
#define DRD_TIMEOUT 2
// RTC Memory Address for the DoubleResetDetector to use
#define DRD_ADDRESS 0

#define CONFIG_PORTAL_TIMEOUT 60 //jak dlouho zustane v rezimu AP nez se cip resetuje
#define CONNECT_TIMEOUT 120 //jak dlouho se ceka na spojeni nez se aktivuje config portal
  


#endif
