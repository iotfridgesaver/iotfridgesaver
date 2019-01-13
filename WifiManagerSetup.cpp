
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

    _emonCMSserverAddressCParam = new WiFiManagerParameter ("server", "EmonCMS server", "cloud.iotfridgesaver.com", MAX_STRING_LENGTH);
    _emonCMSserverPathCParam = new WiFiManagerParameter ("path", "EmonCMS server path", "/", MAX_STRING_LENGTH);
    _emonCMSwriteApiKeyCParam = new WiFiManagerParameter ("apikey", "EmonCMS API key", "", MAX_STRING_LENGTH);
    _mainsVoltageCParam = new WiFiManagerParameter ("voltage", "Mains voltage", "230", 5);

#if defined MQTT_POWER_INPUT || defined MQTT_FEED_SEND
    _MQTTserverCParam = new WiFiManagerParameter ("mqtt_server", "MQTT server", "", MAX_STRING_LENGTH);
    _MQTTportCParam = new WiFiManagerParameter ("mqtt_port", "MQTT server port", "1883", MAX_STRING_LENGTH);
    _MQTTloginCParam = new WiFiManagerParameter ("mqtt_login", "MQTT User", "", MAX_STRING_LENGTH);
    _MQTTpasswdCParam = new WiFiManagerParameter ("mqtt_password", "MQTT Password", "", MAX_STRING_LENGTH);
#ifdef MQTT_POWER_INPUT
    _MQTTfridgeTopicCParam = new WiFiManagerParameter ("mqtt_fridge_topic", "Fridge MQTT topic", "", MAX_STRING_LENGTH);
    _MQTTtotalTopicCParam = new WiFiManagerParameter ("mqtt_total_topic", "House MQTT topic", "", MAX_STRING_LENGTH);
#endif // MQTT_POWER_INPUT
#endif

    setConnectTimeout(30);
    //setCustomHeadElement("<style>input[type='checkbox'] {width: initial;}</style>");
    addParameter(_emonCMSserverAddressCParam);
    addParameter (_emonCMSserverPathCParam);
    addParameter(_emonCMSwriteApiKeyCParam);
    addParameter (_mainsVoltageCParam);

#if defined MQTT_POWER_INPUT || defined MQTT_FEED_SEND
    addParameter (_MQTTserverCParam);
    addParameter (_MQTTportCParam);
    addParameter (_MQTTloginCParam);
    addParameter (_MQTTpasswdCParam);
#ifdef MQTT_POWER_INPUT
    addParameter (_MQTTfridgeTopicCParam);
    addParameter (_MQTTtotalTopicCParam);
#endif // MQTT_POWER_INPUT
#endif

    if (!autoConnect()) {
        Serial.println("failed to connect and hit timeout");
        //reset and try again, or maybe put it to deep sleep
        ESP.reset();
        delay(1000);
    }
}

#if defined MQTT_POWER_INPUT || defined MQTT_FEED_SEND
String MyWiFiManager::getMQTTserver () {
    const char * charStr = _MQTTserverCParam->getValue ();
    return String (charStr);
}

int16_t MyWiFiManager::getMQTTport () {
    const char * charStr = _MQTTportCParam->getValue ();
    return atoi (charStr);
}

String MyWiFiManager::getMQTTlogin () {
    const char * charStr = _MQTTloginCParam->getValue ();
    return String (charStr);
}

String MyWiFiManager::getMQTTpasswd () {
    const char * charStr = _MQTTpasswdCParam->getValue ();
    return String (charStr);
}
#ifdef MQTT_POWER_INPUT
String MyWiFiManager::getFridgeMQTTtopic () {
    const char * charStr = _MQTTfridgeTopicCParam->getValue ();
    return String (charStr);
}

String MyWiFiManager::getTotalMQTTtopic () {
    const char * charStr = _MQTTtotalTopicCParam->getValue ();
    return String (charStr);
}
#endif // MQTT_POWER_INPUT
#endif

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