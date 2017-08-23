// JSON_messages.h

#ifndef _JSON_MESSAGES_h
#define _JSON_MESSAGES_h

#include "Global.h"

#if defined(ARDUINO) && ARDUINO >= 100
	#include "Arduino.h"
#else
	#include "WProgram.h"
#endif

int peticionPut(String payload);
void processJsonMessage(JsonObject& root);
int process_send_firebase(JsonObject& message);
void respond_send_firebase(int result);
String trimMacAddress(String mac);
String getDate();

#endif

