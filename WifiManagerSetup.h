// WifiManagerSetup.h

#ifndef _WIFIMANAGERSETUP_h
#define _WIFIMANAGERSETUP_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "Arduino.h"
#else
	#include "WProgram.h"
#endif

#include <WiFiManager.h>    // https://github.com/tzapu/WiFiManager
#include "ConfigData.h"

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
	MyWiFiManager();
	void init();
    String getEmonCMSserverAddress ();
    String getEmonCMSserverPath ();
    String getEmonCMSwriteApiKey ();
#ifdef MQTT
    String getMQTTserver ();
    int16_t getMQTTport ();
    String getFridgeMQTTtopic ();
    String getTotalMQTTtopic ();
#endif
    int getMainsVoltage ();
};

#endif

