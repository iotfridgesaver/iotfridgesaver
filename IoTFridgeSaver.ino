#include "Global.h"
#include "JSON_messages.h"
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
//#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include "WifiManagerSetup.h"
#include "OTAhelper.h"
#include <ArduinoOTA.h>
#include <ESP8266HTTPClient.h>
#include <TimeLib.h>
#include <NtpClientLib.h> // https://github.com/gmag11/NtpClient



//#include <TimeLib.h>  

// Temperature
#include <OneWire.h>
#include <DallasTemperature.h>

#define TEMPERATURE_PRECISION 11 // default (9 bits 0.5°C 93.75 ms) changed to (11 bits 0.125°C 375 ms)
int numberOfDevices; // Number of temperature devices found
DeviceAddress tempDeviceAddress; // We'll use this variable to store a found device address

#define EMONLIB

#ifdef EMONLIB
// Power Consumption
#include "EmonLib.h"                   // Include Emon Library
EnergyMonitor emon1;                   // Create an instance
#endif



// variables to automatically allocate the thermometer positions.
float temperatures[4] = { 29.2 ,5.12,-19.12,33.12 };
byte tempOut_tposition;               // position of the sensor that measure the external temperature from the front of the fridge.
byte tempInt_tposition;               // position of the sensor that measure the external temperature from the back  of the fridge.
byte tempFri_tposition;               // position of the sensor that measure the internal temperature of the fridge.
byte tempFre_tposition;               // position of the sensor that measure the internal temperature of the freezer.


// Temperature configuration
// Data wire is plugged into port 2 on the Arduino
#define ONE_WIRE_BUS D5
// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);
// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);


//buffer string for double to char conversion
char outstr[10];



#define TIMEZONE 1
#define NTP_SERVER "es.pool.ntp.org"
//SoftwareSerial sserial(4, 5);
bool OTAupdating = false;

#ifdef DEBUG_ENABLED
#define DEBUG_SERIAL
#include <RemoteDebug.h>  // https://github.com/JoaoLopesF/ESP8266-RemoteDebug-Telnet
RemoteDebug Debug;
#endif

// Cliente WiFi
WiFiClientSecure client;

#include <stdint.h>
#include <stddef.h>

//#include "WString.h"
//#include "Printable.h"

void initTempSensors() {
	#ifdef DEBUG
	Serial.println("Init Dallas Temperature Control Library ");
	#endif

	// Start up the temperatura library 
	sensors.begin();

	// Grab a count of devices on the wire
	numberOfDevices = sensors.getDeviceCount();

	#ifdef DEBUG
	// locate devices on the bus
	Serial.print("Locating devices...");
	Serial.print("Found ");
	Serial.print(numberOfDevices, DEC);
	Serial.println(" devices.");

	// report parasite power requirements
	Serial.print("Parasite power is: ");
	if (sensors.isParasitePowerMode())
		Serial.println("ON");
	else
		Serial.println("OFF");
	#endif


	// Loop through each device, print out address
	for (int i = 0; i < numberOfDevices; i++)
	{
		// Search the wire for address
		if (sensors.getAddress(tempDeviceAddress, i))
		{

	#ifdef DEBUG
			Serial.print("Found device ");
			Serial.print(i, DEC);
			Serial.print("Setting resolution to ");
			Serial.println(TEMPERATURE_PRECISION, DEC);
	#endif

			// set the resolution to TEMPERATURE_PRECISION bit (Each Dallas/Maxim device is capable of several different resolutions)
			sensors.setResolution(tempDeviceAddress, TEMPERATURE_PRECISION);

	#ifdef DEBUG
			Serial.print("Resolution actually set to: ");
			Serial.print(sensors.getResolution(tempDeviceAddress), DEC);
			Serial.println();
	#endif


		}
		else {

	#ifdef DEBUG
			Serial.print("Found ghost device at ");
			Serial.print(i, DEC);
			Serial.print(" but could not detect address. Check power and cabling");
	#endif

		}
	}

	delay(2000);
	sensors.setWaitForConversion(false);  // makes it async
	sensors.requestTemperatures();
	delay(1000);


	//  script to automatically allocate the postion of each sensor based on their temperature
	int max_i = 0;
	temperatures[0] = sensors.getTempCByIndex(0);
	temperatures[1] = sensors.getTempCByIndex(1);
	temperatures[2] = sensors.getTempCByIndex(2);
	temperatures[3] = sensors.getTempCByIndex(3);

	max_i = MaxValue();
	tempInt_tposition = max_i;
	temperatures[max_i] = -100;

	max_i = MaxValue();
	tempOut_tposition = max_i;
	temperatures[max_i] = -100;

	max_i = MaxValue();
	tempFri_tposition = max_i;
	temperatures[max_i] = -100;

	max_i = MaxValue();
	tempFre_tposition = max_i;
	//  temperatures[max_i]=-100;


#ifdef DEBUG
	Serial.print("Pos tempInt: ");
	Serial.println(tempInt_tposition);
	Serial.print("Pos tempOut: ");
	Serial.println(tempOut_tposition);
	Serial.print("Pos tempFri: ");
	Serial.println(tempFri_tposition);
	Serial.print("Pos tempFre: ");
	Serial.println(tempFre_tposition);

#endif



}

void initRemoteDebug() {
	#ifdef DEBUG_ENABLED
	Debug.begin("FridgeSaverMonitor");
	Debug.setResetCmdEnabled(true);
	Debug.showTime(true);
		#ifdef DEBUG_SERIAL
		Debug.showColors(false);
		Debug.setSerialEnabled(true);
		#else
		Debug.showColors(true);
		#endif
	#endif
}

void setup() {
	//MyWiFiManager wifiManager;

	Serial.begin(115200);

	//sserial.begin(9600);

	//wifiManager.setTimeout(15);
	//wifiManager.init();
	//wifiManager.setAPCallback(configModeCallback);
	//wifiManager.setSaveConfigCallback(saveConfigCallback);

	delay(2000);

	MDNS.begin("FridgeSaverMonitor");

	OTASetup();

	initRemoteDebug();

	NTP.begin(NTP_SERVER, TIMEZONE, true);
	
	initTempSensors();

#ifdef DEBUG
	Serial.println("Compiled: " __DATE__ ", " __TIME__ ", " __VERSION__);
#endif
	
	// FAN control we will update to control from the could, currently there is a switch on pin 10 to disable the FAN 
	pinMode(D3, INPUT_PULLUP);
	analogWrite(D1, 0);
	// FAN control


	// Consumption calibration
#ifdef EMONLIB
	emon1.current(A0, 3.05);             // Current: input pin, calibration.
#endif
}

void getTemperatures(float *temperatures) {

	temperatures[0] = sensors.getTempCByIndex(tempOut_tposition);
	temperatures[1] = sensors.getTempCByIndex(tempInt_tposition);
	temperatures[2] = sensors.getTempCByIndex(tempFri_tposition);
	temperatures[3] = sensors.getTempCByIndex(tempFre_tposition);
}

void getPower(double *watts) {

#ifdef EMONLIB
	*watts = emon1.calcIrms(1480)*230;  // Calculate Irms * V. Aparent power
#endif

}

void loop() {
	// burst variables
	static boolean burst = true;
	static uint32_t previousburst = 0;
	const long period_burst = 15000;  // 30000 mean that we will ship each 60 seconds

	static float temperatures[4];
	double watts;


	/*
	String date = "";

	date = String(year())
		+ '-' + (month() < 10 ? '0' + String(month()) : String(month()))
		+ '-' + (day() < 10 ? '0' + String(day()) : String(day()))
		+ ' ' + (hour() < 10 ? '0' + String(hour()) : String(hour()))
		+ ':' + (minute() < 10 ? '0' + String(minute()) : String(minute()))
		+ ':' + (second() < 10 ? '0' + String(second()) : String(second()));
	*/

	// burst timer for the JSON 
	unsigned long currentMillis = millis();
	if (currentMillis - previousburst >= period_burst) {
		// save the last time you blinked
		previousburst = currentMillis;
		burst = !burst;
		// if true we send the JSON if not we just request the temperature, to be ready for the next burst of information
		if (burst) {


#ifdef DEBUG
			Serial.print("Fecha: ");
			Serial.println(date);
#endif

			getTemperatures(temperatures);
			getPower(&watts);

			String timeFan = "0";

			// we active the fan, only if the compressor is on (this mean a consumption is over 60W => Irms> 60/235 = 0.255 

#ifdef EMONLIB
			if (digitalRead(D3) == 1 && irms > 0.255) {
#else
			if (digitalRead(D3) == 1) {
#endif
				analogWrite(D1, 850);
				timeFan = "1";
			}
			else {
				analogWrite(D1, 0);
			}


			//peticionPut(date,	tempOut, tempInt, tempFri, tempFre, consumption, timeFan);


		}

		else {

			sensors.requestTemperatures();

		}
	}



}


/********** FUNCIÓN PARA HACER LA PETICIÓN PUT **********/

void peticionPut(String date,
	String tempOut,
	String tempInt,
	String tempFri,
	String tempFre,
	String consumption,
	String timeFan)
{



	String payload = "{\"\
tempOut\":";
	payload += tempOut;
	payload += ",\"\
tempInt\":";
	payload += tempInt;
	payload += ",\"\
tempFri\":";
	payload += tempFri;
	payload += ",\"\
tempFre\":";
	payload += tempFre;
	payload += ",\"\
consumption\":";
	payload += consumption;
	payload += ",\"\
timeFan\":";
	payload += timeFan;
	payload += "}\
";

	// PUT JSON
	String toSend =
		"{\"command\":\"Send Firebase\",\"\
payload\":";
	toSend += payload;
	//  toSend += "}\r\n";
	toSend += "}\r";


	// sending JSON string to the ESP using soft serial port.
	//  sSerial.println(toSend);

	//JsonObject& root = jsonBuffer.parseObject(toSend);


	String string_buffer;
	DynamicJsonBuffer jsonBuffer(200);
#ifdef SIMULATE
	delay(5000);
	String macAddress = WiFi.macAddress();
#ifdef DEBUG_ENABLED
	if (Debug.isActive(Debug.INFO))
		Debug.printf("MAC Address: %s\n", macAddress.c_str());
#endif
	string_buffer =
		"{\"command\":\"Send Firebase\",\"\
payload\":{\"\
tempOut\":34.5,\"\
tempInt\":29.0,\"\
tempFri\":4.0,\"\
tempFre\":-18.0,\"\
consumption\":4.0,\"\
timeFan\":0}\
}";
	if (true) {
#else
	//  if (sserial.available()) {
	if (true) {
		//    string_buffer = sserial.readStringUntil('\n');
		string_buffer = toSend;

#endif

		JsonObject& root = jsonBuffer.parseObject(string_buffer);

#ifdef DEBUG_ENABLED
		if (Debug.isActive(Debug.INFO)) {
			Debug.print("JSON: ");
			root.prettyPrintTo(Debug);
			Debug.println();
		}
#endif

		if (!root.success()) {
#ifdef DEBUG_ENABLED
			if (Debug.isActive(Debug.WARNING)) {
				Debug.println("JSON: parseObject() failed");
			}
#endif
			return;
		}

		processJsonMessage(root);


	}



























#ifdef DEBUG_ENABLED
	Debug.handle();
#endif
	ArduinoOTA.handle();
}








// funtion that provice the position of the max value on the array temperatures.
int MaxValue() {

	float max_v = -90;
	int max_i = 0;

	for (int i = 0; i < sizeof(temperatures) / sizeof(temperatures[0]); i++)
	{
		if (temperatures[i] > max_v)
		{
			max_v = temperatures[i];
			max_i = i;
		}
	}
	return max_i;

}


