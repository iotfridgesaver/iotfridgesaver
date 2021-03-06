
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

#define MAX_STRING_LENGTH 40  ///< Longitud máxima de los parámetros de usuario

void MyWiFiManager::init() { ///< Inicia el servidor WiFiManager con los parámetros específicos del proyecto
    //resetSettings(); // No quitar el comentario, solo para desarrollo
    if (_config.emonCMSserverAddress != "")
        _emonCMSserverAddressCParam = new WiFiManagerParameter ("server", "EmonCMS server", _config.emonCMSserverAddress.c_str(), MAX_STRING_LENGTH);
    else
        _emonCMSserverAddressCParam = new WiFiManagerParameter ("server", "EmonCMS server", "cloud.iotfridgesaver.com", MAX_STRING_LENGTH);

    if (_config.emonCMSserverPath != "")
        _emonCMSserverPathCParam = new WiFiManagerParameter ("path", "EmonCMS server path", _config.emonCMSserverPath.c_str(), MAX_STRING_LENGTH);
    else
        _emonCMSserverPathCParam = new WiFiManagerParameter ("path", "EmonCMS server path", "/", MAX_STRING_LENGTH);

    if (_config.emonCMSwriteApiKey != "")
        _emonCMSwriteApiKeyCParam = new WiFiManagerParameter ("apikey", "EmonCMS API key", _config.emonCMSwriteApiKey.c_str(), MAX_STRING_LENGTH);
    else
        _emonCMSwriteApiKeyCParam = new WiFiManagerParameter ("apikey", "EmonCMS API key", "", MAX_STRING_LENGTH);

    if (_config.mainsVoltage > 0)
        _mainsVoltageCParam = new WiFiManagerParameter ("voltage", "Mains voltage", String(_config.mainsVoltage).c_str() , 5);
    else
        _mainsVoltageCParam = new WiFiManagerParameter ("voltage", "Mains voltage", "230", 5);

    setConnectTimeout(WIFI_TIMEOUT);
    if (_configLoaded) // Añadir timeout al portal de configuración solo si la carga de la configuración fue correcta
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

void MyWiFiManager::setConfig (config_t config, bool configLoaded) {
    _config.emonCMSserverAddress = config.emonCMSserverAddress;
    _config.emonCMSserverPath = config.emonCMSserverPath;
    _config.emonCMSwriteApiKey = config.emonCMSwriteApiKey;
    _config.mainsVoltage = config.mainsVoltage;
    _configLoaded = configLoaded;
}

MyWiFiManager::MyWiFiManager() : WiFiManager () {}

#endif // WIFI_MANAGER