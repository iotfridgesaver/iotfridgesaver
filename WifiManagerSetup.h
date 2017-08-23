// WifiManagerSetup.h

#ifndef _WIFIMANAGERSETUP_h
#define _WIFIMANAGERSETUP_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "Arduino.h"
#else
	#include "WProgram.h"
#endif

#include <ESPAsyncWiFiManager.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
//#include <ESPAsyncWiFiManager.h>         // https://github.com/alanswx/ESPAsyncWiFiManager

//void configModeCallback(AsyncWiFiManager *myWiFiManager);
//void saveConfigCallback();

class MyWiFiManager : public AsyncWiFiManager {
protected:
    AsyncWiFiManagerParameter * customNTPServer;
    AsyncWiFiManagerParameter * customNTPLabel;
public:
	MyWiFiManager();
	void init();
};

#endif

