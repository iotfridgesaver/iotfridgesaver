// 
// 
// 

#include "WifiManagerSetup.h"
extern const char *configFileName;

//extern String emonCMSserverAddress;  ///< Dirección del servidor EmonCMS
//extern String emonCMSwriteApiKey; 
//extern int mainsVoltage;

#define MAX_STRING_LENGTH 40
extern bool configLoaded;

void MyWiFiManager::init() {
    //resetSettings();

/*#ifdef DEBUG_ENABLED
    Serial.printf ("emonCMSserverAddress1: %s\n", emonCMSserverAddress.c_str ());
    Serial.printf ("emonCMSwriteApiKey1: %s\n", emonCMSwriteApiKey.c_str ());
    Serial.printf ("mainsVoltage1: %d\n", mainsVoltage);
#endif // DEBUG_ENABLED

    String serverName;
    String apiKey;
    String volt;*/
        
    /*if (configLoaded) {
        serverName = emonCMSserverAddress;
        apiKey = emonCMSwriteApiKey;
        char * tempStr;
        tempStr = itoa (mainsVoltage, tempStr, 10);
        volt = tempStr;
    } else {
        serverName = "cloud.iotfridgesaver.com";
        apiKey = "";
        volt = "230";
    }*/
    
    _emonCMSserverAddressCParam = new WiFiManagerParameter ("server", "EmonCMS server", "emoncms.iotfridgesaver.com", MAX_STRING_LENGTH);
    _emonCMSserverPathCParam = new WiFiManagerParameter ("path", "EmonCMS server path", "/", MAX_STRING_LENGTH);
    _emonCMSwriteApiKeyCParam = new WiFiManagerParameter ("apikey", "EmonCMS API key", "", MAX_STRING_LENGTH);
    _mainsVoltageCParam = new WiFiManagerParameter ("voltage", "Mains voltage", "230", 5);

#ifdef MQTT
    _MQTTserverCParam = new WiFiManagerParameter ("mqtt_server", "MQTT server", "", MAX_STRING_LENGTH);
    _MQTTportCParam = new WiFiManagerParameter ("mqtt_port", "MQTT server port", "1883", MAX_STRING_LENGTH);
    _MQTTtopicCParam = new WiFiManagerParameter ("mqtt_topic", "MQTT topic", "", MAX_STRING_LENGTH);
#endif

    setConnectTimeout(30);
    //setCustomHeadElement("<style>input[type='checkbox'] {width: initial;}</style>");
    addParameter(_emonCMSserverAddressCParam);
    addParameter (_emonCMSserverPathCParam);
    addParameter(_emonCMSwriteApiKeyCParam);
    addParameter (_mainsVoltageCParam);

#ifdef MQTT
    addParameter (_MQTTserverCParam);
    addParameter (_MQTTportCParam);
    addParameter (_MQTTtopicCParam);
#endif

    if (!autoConnect()) {
        Serial.println("failed to connect and hit timeout");
        //reset and try again, or maybe put it to deep sleep
        ESP.reset();
        delay(1000);
    }
}

#ifdef MQTT
String MyWiFiManager::getMQTTserver () {
    const char * charStr = _MQTTserverCParam->getValue ();
    return String (charStr);
}

int16_t MyWiFiManager::getMQTTport () {
    const char * charStr = _MQTTportCParam->getValue ();
    return atoi (charStr);
}

String MyWiFiManager::getMQTTtopic () {
    const char * charStr = _MQTTtopicCParam->getValue ();
    return String (charStr);
}
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

MyWiFiManager::MyWiFiManager() : WiFiManager () {}
