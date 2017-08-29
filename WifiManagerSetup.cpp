// 
// 
// 

#include "WifiManagerSetup.h"

#define MAX_STRING_LENGTH 40

void MyWiFiManager::init() {
    resetSettings();
	//customNTPServer = new AsyncWiFiManagerParameter  ("server", "NTP server", "enabled", 40, " type=\"checkbox\"  ");
	//customNTPLabel = new AsyncWiFiManagerParameter  ("<label for='server'>NTP</label>");
    _emonCMSserverAddressCParam = new AsyncWiFiManagerParameter ("server", "EmonCMS server", "cloud.iotfridgesaver.com", MAX_STRING_LENGTH);
    _emonCMSwriteApiKeyCParam = new AsyncWiFiManagerParameter ("apikey", "EmonCMS API key", "", MAX_STRING_LENGTH);
    _mainsVoltageCParam = new AsyncWiFiManagerParameter ("voltage", "Mains voltage", "230", 5);
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
