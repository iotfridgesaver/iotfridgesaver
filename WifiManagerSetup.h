// WifiManagerSetup.h

#ifndef _WIFIMANAGERSETUP_h
#define _WIFIMANAGERSETUP_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "Arduino.h"
#else
	#include "WProgram.h"
#endif

#include <ESPAsyncWiFiManager.h>    // https://github.com/alanswx/ESPAsyncWiFiManager
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>

//void configModeCallback(AsyncWiFiManager *myWiFiManager);
//void saveConfigCallback();

class MyWiFiManager : public AsyncWiFiManager {
protected:
    //AsyncWiFiManagerParameter * customNTPServer;
    //AsyncWiFiManagerParameter * customNTPLabel;
    AsyncWiFiManagerParameter * _emonCMSserverAddressCParam;
    AsyncWiFiManagerParameter * _emonCMSserverPathCParam;
    AsyncWiFiManagerParameter * _emonCMSwriteApiKeyCParam;
    AsyncWiFiManagerParameter * _mainsVoltageCParam;
    /*String emonCMSserverAddress;
    String emonCMSwriteApiKey;
    int mainsVoltage;*/
public:
	MyWiFiManager();
	void init();
    String getEmonCMSserverAddress ();
    String getEmonCMSserverPath ();
    String getEmonCMSwriteApiKey ();
    int getMainsVoltage ();
};

#endif

