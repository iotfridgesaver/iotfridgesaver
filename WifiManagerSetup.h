/**
* @file WifiManagerSetup.h
* \~English
* @brief Wrapper for WiFiManager class to setup server with only one line 
*
* \~Spanish
* @brief Envoltorio para la clase WiFiManager para iniciar el servidor con una sola línea
*/
#ifndef _WIFIMANAGERSETUP_h
#define _WIFIMANAGERSETUP_h
#ifdef WIFI_MANAGER

#if defined(ARDUINO) && ARDUINO >= 100
	#include "Arduino.h"
#else
	#include "WProgram.h"
#endif

#include <WiFiManager.h>    // https://github.com/tzapu/WiFiManager
#include "ConfigData.h"

/**
* @class MyWiFiManager
* \~English
* @brief Wrapper for WiFiManager class to setup server with only one line
*
* \~Spanish
* @brief Envoltorio para la clase WiFiManager para iniciar el servidor con una sola línea
*/
class MyWiFiManager : public WiFiManager {
protected:
    //AsyncWiFiManagerParameter * customNTPServer;
    //AsyncWiFiManagerParameter * customNTPLabel;
    WiFiManagerParameter * _emonCMSserverAddressCParam;
    WiFiManagerParameter * _emonCMSserverPathCParam;
    WiFiManagerParameter * _emonCMSwriteApiKeyCParam;
    WiFiManagerParameter * _mainsVoltageCParam;
#if defined MQTT_POWER_INPUT || defined MQTT_FEED_SEND
    WiFiManagerParameter * _MQTTserverCParam;
    WiFiManagerParameter * _MQTTportCParam;
#ifdef MQTT_POWER_INPUT
    WiFiManagerParameter * _MQTTfridgeTopicCParam;
    WiFiManagerParameter * _MQTTtotalTopicCParam;
#endif // MQTT_POWER_INPUT
#endif

    /*String emonCMSserverAddress;
    String emonCMSwriteApiKey;
    int mainsVoltage;*/
public:
	MyWiFiManager(); ///< Constructor
	void init();     ///< Inicia el servidor WiFiManager con los parámetros específicos del proyecto
    String getEmonCMSserverAddress ();  ///< Obtiene la dirección del servidor de EmonCMS
    String getEmonCMSserverPath ();     ///< Obtiene la ruta del servidor de EmonCMS
    String getEmonCMSwriteApiKey ();    ///< Obtiene la clave API del usuario
#if defined MQTT_POWER_INPUT || defined MQTT_FEED_SEND
    String getMQTTserver ();            ///< Obtiene la dirección del servidor MQTT
    int16_t getMQTTport ();             ///< Obtiene el puerto del servidor MQTT
#ifdef MQTT_POWER_INPUT
    String getFridgeMQTTtopic ();       ///< Obtiene el topic del consumo del frigorífico
    String getTotalMQTTtopic ();        ///< Obtiene el topic del consumo total de la vivienda
#endif // MQTT_POWER_INPUT
#endif
    int getMainsVoltage ();             ///< Obtiene la tensión configurada por el usuario
};

#endif WIFI_MANAGER
#endif

