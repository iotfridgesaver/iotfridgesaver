// 
// 
// 

#include "JSON_messages.h"

extern WiFiClientSecure client;

#ifdef DEBUG_ENABLED
#define DEBUG_SERIAL
#include <RemoteDebug.h>  // https://github.com/JoaoLopesF/ESP8266-RemoteDebug-Telnet
extern RemoteDebug Debug;
#endif

//extern SoftwareSerial sserial;

String trimMacAddress(String mac) {
	String trimmedMac;

	for (int i = 0; i < mac.length(); i++) {
		if (mac[i] != ':') {
			trimmedMac += mac[i];
		}
	}
	return trimmedMac;
}

String getDate() {
	Serial.printf("");

	String date = "";
	date = String(year())
		+ '-' + (month() < 10 ? '0' + String(month()) : String(month()))
		+ '-' + (day() < 10 ? '0' + String(day()) : String(day()))
		+ ' ' + (hour() < 10 ? '0' + String(hour()) : String(hour()))
		+ ':' + (minute() < 10 ? '0' + String(minute()) : String(minute()))
		+ ':' + (second() < 10 ? '0' + String(second()) : String(second()));

#ifdef DEBUG_ENABLED
	if (Debug.isActive(Debug.VERBOSE))
		Debug.printf("%s: Fecha: %s\n", __FUNCTION__, date.c_str());
#endif
	return date;
}

/********** FUNCIÓN PARA HACER LA PETICIÓN PUT **********/
int peticionPut(String payload)
{

	// Cerramos cualquier conexión antes de enviar una nueva petición
	client.stop();
	client.flush();

	String macAddress = trimMacAddress(WiFi.macAddress());
	String date = getDate();

	if (!client.connect(HOSTFIREBASE, 443)) {
#ifdef DEBUG_ENABLED
		if (Debug.isActive(Debug.ERROR)) {
			Debug.printf("%s: Error al hacer la peticion\n", __FUNCTION__);
		}
#endif
		return -1;
	}

	String toSend = "PUT /" + macAddress + "/" + date + ".json HTTP/1.1\r\n";
	toSend += "Host: " + String(HOSTFIREBASE) + "\r\n";
	toSend += "Content-Type: application/json\r\n";
	toSend += "Content-Length: " + String(payload.length() + 2) + "\r\n\r\n";
	toSend += payload + "\r\n";

#ifdef DEBUG_ENABLED
	if (Debug.isActive(Debug.INFO)) {
		Debug.printf("%s: Request; ->\n %s\n", __FUNCTION__, toSend.c_str());
	}
#endif

	client.print(toSend);

	unsigned long timeout = millis();
	while (client.available() == 0) {
		if (millis() - timeout > 5000) {
#ifdef DEBUG_ENABLED
			if (Debug.isActive(Debug.ERROR))
				Debug.printf("%s: Firebase client Timeout !", __FUNCTION__);
#endif
			client.stop();
			return -2;
		}
	}

	while (client.available()) {
		String line = client.readStringUntil('\n');
#ifdef DEBUG_ENABLED
		if (Debug.isActive(Debug.INFO))
			Debug.printf("%s: Response: %s\n", __FUNCTION__, line.c_str());
#endif
	}


	client.stop();

#ifdef DEBUG_ENABLED
	if (Debug.isActive(Debug.INFO)) {
		Debug.printf("%s: JSON sent !!!\n", __FUNCTION__);
	}
#endif
	return 0;
}

void processJsonMessage(JsonObject & root) {
	String command = root["command"];

	// COMMAND: Send Firebase
	if (command.equals("Send Firebase")) {
		int result = process_send_firebase(root["payload"]);
		respond_send_firebase(result);
	}
	// Any other command
	else {
#ifdef DEBUG_ENABLED
		if (Debug.isActive(Debug.WARNING)) {
			Debug.println("JSON: Wrong command");
		}
#endif
	}

}

int process_send_firebase(JsonObject& payload) {
	//JsonObject& payload = message["payload"];
#ifdef DEBUG_ENABLED
	if (Debug.isActive(Debug.INFO)) {
		Debug.print("JSON: ");
		payload.prettyPrintTo(Debug);
		Debug.println();
	}
#endif
	String buffer = "";
	payload.printTo(buffer);

	int error = 0;
	if (error = peticionPut(buffer) < 0) {
#ifdef DEBUG_ENABLED
		if (Debug.isActive(Debug.ERROR))
			Debug.printf("JSON: Error enviando payload. %d\n", error);
#endif
	}
	return error;
}

void respond_send_firebase(int result) {

	StaticJsonBuffer<200> jsonBuffer;
	JsonObject& root = jsonBuffer.createObject();

	root["response"] = "Send Firebase";
	root["result"] = result;
	root["time"] = now(); // Time in long int format. Nukmber of seconds since 01/01/1970

#ifdef DEBUG_ENABLED
	if (Debug.isActive(Debug.INFO)) {
		Debug.println("Firebase send response");
		root.printTo(Debug);
		Debug.println();
	}
#endif //DEBUG_ENABLED


	root.printTo(Serial);
	Serial.print("\n");

}

