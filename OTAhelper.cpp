// 
// 
// 

#include "OTAhelper.h"

#include <ArduinoOTA.h>
//#define OTA_DEBUG

//extern RemoteDebug Debug;
extern bool OTAupdating;

void OTASetup() {
    ArduinoOTA.onStart([]() {
		OTAupdating = true;
#ifdef OTA_DEBUG
		String type;
		if (ArduinoOTA.getCommand() == U_FLASH)
			type = "sketch";
		else // U_SPIFFS
			type = "filesystem";

#endif // OTA_DEBUG
        // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
#ifdef OTA_DEBUG
        Serial.printf("Start updating %s\n", type.c_str());
#endif // OTA_DEBUG
	});
#ifdef OTA_DEBUG
	ArduinoOTA.onEnd([]() {
		Serial.printf("\nEnd\n");
	});

	ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
		//delay(1);
    });
#endif // OTA_DEBUG
	ArduinoOTA.onError([](ota_error_t error) {
		OTAupdating = false;

#ifdef OTA_DEBUG
		Serial.printf("\nError[%u]: ", error);
		if (error == OTA_AUTH_ERROR) {
			Serial.println("Auth Failed");
		}
		else if (error == OTA_BEGIN_ERROR) {
			Serial.println("Begin Failed");
		}
		else if (error == OTA_CONNECT_ERROR) {
			Serial.println("Connect Failed");
		}
		else if (error == OTA_RECEIVE_ERROR) {
			Serial.println("Receive Failed");
		}
		else if (error == OTA_END_ERROR) {
			Serial.println("End Failed");
		}

#endif // OTA_DEBUG
    });
    ArduinoOTA.begin();
}
