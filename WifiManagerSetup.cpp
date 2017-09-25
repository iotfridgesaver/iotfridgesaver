
/**
* @file WifiManagerSetup.cpp
* \~English
* @brief WiFiManager class method definition.
*
* \~Spanish
* @brief Definición de métodos para la clase WiFiManager.
*/
#include "ConfigData.h"
#include "WifiManagerSetup.h"
#ifdef WIFI_MANAGER

extern const char *configFileName;
extern String emonCMSserverAddress;
extern String emonCMSserverPath;          ///<\~Spanish Ruta del servicio EmonCMS en el servidor Web
extern String emonCMSwriteApiKey;         ///<\~Spanish API key del usuario
extern int mainsVoltage;                 ///<\~Spanish Tensión de alimentación

//extern String emonCMSserverAddress;  ///< Dirección del servidor EmonCMS
//extern String emonCMSwriteApiKey; 
//extern int mainsVoltage;

#define MAX_STRING_LENGTH 40  ///< Longitud máxima de los parámetros de usuario
extern bool configLoaded;

void MyWiFiManager::init() { ///< Inicia el servidor WiFiManager con los parámetros específicos del proyecto
    //resetSettings(); // No quitar el comentario, solo para desarrollo

    if (emonCMSserverAddress != "")
        _emonCMSserverAddressCParam = new WiFiManagerParameter ("server", "EmonCMS server", emonCMSserverAddress.c_str(), MAX_STRING_LENGTH);
    else
        _emonCMSserverAddressCParam = new WiFiManagerParameter ("server", "EmonCMS server", "cloud.iotfridgesaver.com", MAX_STRING_LENGTH);

    if (emonCMSserverPath != "")
        _emonCMSserverPathCParam = new WiFiManagerParameter ("path", "EmonCMS server path", emonCMSserverPath.c_str(), MAX_STRING_LENGTH);
    else
        _emonCMSserverPathCParam = new WiFiManagerParameter ("path", "EmonCMS server path", "/", MAX_STRING_LENGTH);

    if (emonCMSwriteApiKey != "")
        _emonCMSwriteApiKeyCParam = new WiFiManagerParameter ("apikey", "EmonCMS API key", emonCMSwriteApiKey.c_str(), MAX_STRING_LENGTH);
    else
        _emonCMSwriteApiKeyCParam = new WiFiManagerParameter ("apikey", "EmonCMS API key", "", MAX_STRING_LENGTH);

    if (mainsVoltage > 0)
        _mainsVoltageCParam = new WiFiManagerParameter ("voltage", "Mains voltage", itoa(mainsVoltage,NULL,10) , 5);
    else
        _mainsVoltageCParam = new WiFiManagerParameter ("voltage", "Mains voltage", "230", 5);

    setConnectTimeout(WIFI_TIMEOUT);
    setConfigPortalTimeout (CONFIG_PORTAL_TIMEOUT);
    //setCustomHeadElement("<style>input[type='checkbox'] {width: initial;}</style>");
    addParameter(_emonCMSserverAddressCParam);
    addParameter (_emonCMSserverPathCParam);
    addParameter(_emonCMSwriteApiKeyCParam);
    addParameter (_mainsVoltageCParam);
    if (!autoConnect()) {
        Serial.println("failed to connect and hit timeout");
        //reset and try again, or maybe put it to deep sleep
        ESP.reset();
        delay(1000);
    }
}

String MyWiFiManager::getEmonCMSserverAddress () {
    const char * charStr = _emonCMSserverAddressCParam->getValue ();
    return String (charStr);
}

String MyWiFiManager::getEmonCMSserverPath () {
    const char * charStr = _emonCMSserverPathCParam->getValue ();
    return String (charStr);
}

String MyWiFiManager::getEmonCMSwriteApiKey () {
    const char * charStr = _emonCMSwriteApiKeyCParam->getValue ();
    return String (charStr);
}

int MyWiFiManager::getMainsVoltage () {
    const char * charStr = _mainsVoltageCParam->getValue ();
    return atoi (charStr);
}

MyWiFiManager::MyWiFiManager() : WiFiManager () {}

#endif // WIFI_MANAGER