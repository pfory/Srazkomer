#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <FS.h>          //this needs to be first
#include <Wire.h>
#include <ArduinoJson.h>

//SW name & version
#define     VERSION                          "1.71"
#define     SW_NAME                          "Srazkomer"

#define timers
#define ota
#define verbose
//#define time
//#define serverHTTP



//All of the IO pins have interrupt/pwm/I2C/one-wire support except D0.
//#define                                                   10k Pull-up     GPIO00
//#define                                                                   GPIO16
//#define                                                   MISO            GPIO12
//#define                                                   SCK             GPIO14
//SDA                                       D2 //                           GPIO04
//SCL                                       D1 //                           GPIO05
//BUILTIN_LED                               D4 //10k Pull-up, BUILTIN_LED   GPIO02
#define INTERRUPTPIN                        D5


static const char* const      mqtt_server                    = "192.168.1.56";
static const uint16_t         mqtt_port                      = 1883;
static const char* const      mqtt_username                  = "datel";
static const char* const      mqtt_key                       = "hanka12";
static const char* const      mqtt_base                      = "/home/Srazkomer/esp05";
static const char* const      mqtt_topic_restart             = "restart";
static const char* const      mqtt_topic_netinfo             = "netinfo";
static const char* const      mqtt_config_portal             = "config";
static const char* const      mqtt_config_portal_stop        = "disconfig";

#define SENDSTAT_DELAY                       60000 //poslani statistiky kazdou minutu
#define CONNECT_DELAY                        5000 //ms

#include <fce.h>

#endif
