/**
* @file ConfigData.h
* \~English
* @brief General configuration settings
*
* \~Spanish
* @brief Parámetros generales de configuración
*/
#pragma once
#ifndef _CONFIGDATA_h
#define _CONFIGDATA_h

//#define TEST          ///<\~English Enables test mode to use simulated data without using real sensors \~Spanish Habilita el modo test para usar datos simulados si no tengo sensores
//#define EMONLIB       ///<\~English If enabled it uses Emonlib library to measure power using a clamp sensor \~Spanish Debe estar activado si se usa una pinza CT030 con EmonLib

#include <Arduino.h>

#define DEBUG_ENABLED   ///<\~Spanish Activa la salida de debug por Telnet
#ifdef DEBUG_ENABLED
#define DEBUG_SERIAL    ///<\~Spanish Activa la salida de debug por puerto serie
#endif // DEBUG_ENABLED

#define WIFI_MANAGER    ///<\~Spanish Activa la configuración con WiFiManager
#define MQTT_FEED_SEND  ///<\~Spanish Activa el envío de datos por MQTT, además de EmonCMS

#ifndef EMONLIB
#define MQTT_POWER_INPUT            ///<\~Spanish Activa la recepción de la medida de potencia por MQTT
#endif // EMONLIB

#define WIFI_TIMEOUT        30      ///<\~Spanish Timeout en segundos para conectar a la red WiFi
#define CONFIG_PORTAL_TIMEOUT 120   ///<\~Spanish Timeout en segundos del portal de configuración. Si se supera el dispositivo se reinicia
#define FAN_ENABLE_BUTTON   D3      ///<\~Spanish Pin donde conectar el botón para activar el ventilador
#define FAN_PWM_PIN         D1      ///<\~Spanish Pin para el control de la velocidad del ventilador
#define MEASURE_PERIOD      60000   ///<\~Spanish Periodo de medida de temperatura y consumo en milisegundos, y envío de los datos al servicio en la nube

const int t_longPress =     10000;   ///<\~Spanish Tiempo en milisegundos para identificar pulsación larga

const int fanThreshold =    60;     ///<\~Spanish Valor en vatios por encima del cual se enciende el ventilador

typedef struct {
    String emonCMSserverAddress;       ///<\~Spanish Dirección del servidor Web que aloja a EmonCMS
    String emonCMSserverPath;          ///<\~Spanish Ruta del servicio EmonCMS en el servidor Web
    String emonCMSwriteApiKey;         ///<\~Spanish API key del usuario
    int mainsVoltage;                  ///<\~Spanish Tensión de alimentación
#if defined MQTT_POWER_INPUT || defined MQTT_FEED_SEND
    String mqttServerName;             ///<\~Spanish Nombre del servidor MQTT
    uint16_t mqttServerPort;         ///<\~Spanish Puerto del ervidor MQTT
#ifdef MQTT_POWER_INPUT
    String mqttFridgePowerTopic;       ///<\~Spanish Topic del consumo del frigorífico
    String mqttTotalPowerTopic;        ///<\~Spanish Topic del consumo total de la vivienda
#endif // MQTT_POWER_INPUT
    bool mqttStarted;               ///<\~Spanish Tensión de alimentación
#endif
} config_t;

#endif