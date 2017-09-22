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
#ifdef MQTT
    WiFiManagerParameter * _MQTTserverCParam;
    WiFiManagerParameter * _MQTTfridgeTopicCParam;
    WiFiManagerParameter * _MQTTtotalTopicCParam;
    WiFiManagerParameter * _MQTTportCParam;
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
#ifdef MQTT
    String getMQTTserver ();
    int16_t getMQTTport ();
    String getFridgeMQTTtopic ();
    String getTotalMQTTtopic ();
#endif
    int getMainsVoltage ();             ///< Obtiene la tensión configurada por el usuario
};

#endif

