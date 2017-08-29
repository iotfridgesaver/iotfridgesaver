// 
// 
// 

#include "WifiManagerSetup.h"

extern String emonCMSserverAddress;  ///< Dirección del servidor EmonCMS
extern String emonCMSwriteApiKey; 
extern int mainsVoltage;

#define MAX_STRING_LENGTH 40
extern bool configLoaded;

void MyWiFiManager::init() {
    resetSettings();

    const char * serverName;
    const char * apiKey;
    char *  volt;
        
    if (configLoaded) {
        serverName = emonCMSserverAddress.c_str ();
        apiKey = emonCMSwriteApiKey.c_str ();
        itoa(mainsVoltage, volt,10);
    } else {
        serverName = "cloud.iotfridgesaver.com";
        apiKey = "";
        volt = "230";
    }
    
    _emonCMSserverAddressCParam = new AsyncWiFiManagerParameter ("server", "EmonCMS server", serverName, MAX_STRING_LENGTH);
    _emonCMSwriteApiKeyCParam = new AsyncWiFiManagerParameter ("apikey", "EmonCMS API key", apiKey, MAX_STRING_LENGTH);
    _mainsVoltageCParam = new AsyncWiFiManagerParameter ("voltage", "Mains voltage", volt, 5);
    setConnectTimeout(15);
    //setCustomHeadElement("<style>input[type='checkbox'] {width: initial;}</style>");
    addParameter(_emonCMSserverAddressCParam);
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

String MyWiFiManager::getEmonCMSwriteApiKey () {
    const char * charStr = _emonCMSwriteApiKeyCParam->getValue ();
    return String (charStr);
}

int MyWiFiManager::getMainsVoltage () {
    const char * charStr = _mainsVoltageCParam->getValue ();
    return atoi (charStr);
}

MyWiFiManager::MyWiFiManager() : AsyncWiFiManager (new AsyncWebServer(80),new DNSServer()) {}
