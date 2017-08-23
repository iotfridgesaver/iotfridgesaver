// 
// 
// 

#include "WifiManagerSetup.h"

void MyWiFiManager::init() {
    //wifiManager.resetSettings();
	customNTPServer = new AsyncWiFiManagerParameter  ("server", "NTP server", "enabled", 40, " type=\"checkbox\"  ");
	customNTPLabel = new AsyncWiFiManagerParameter  ("<label for='server'>NTP</label>");
    setConnectTimeout(30);
    setCustomHeadElement("<style>input[type='checkbox'] {width: initial;}</style>");
    addParameter(customNTPServer);
    addParameter(customNTPLabel);
    if (!autoConnect()) {
        Serial.println("failed to connect and hit timeout");
        //reset and try again, or maybe put it to deep sleep
        ESP.reset();
        delay(1000);
    }
}

MyWiFiManager::MyWiFiManager() : AsyncWiFiManager (new AsyncWebServer(80),new DNSServer()) {}
