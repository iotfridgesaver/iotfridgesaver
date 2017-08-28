#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <OneWire.h>
#include <DallasTemperature.h>          // https://github.com/milesburton/Arduino-Temperature-Control-Library
#include <ArduinoOTA.h>
#include "OTAhelper.h"

#define EMONLIB
#ifdef EMONLIB
#include "EmonLib.h"                    // https://github.com/openenergymonitor/EmonLib
EnergyMonitor emon1;                    ///< Instancia del monitor de consumo
#endif

#define DEBUG

const char* WIFI_SSID = "NOMBRE_DE_MI_RED";
const char* WIFI_PASS = "CONTRASE�A";

bool OTAupdating;                       ///< Verdadero si se est� haciendo una actualizaci�n OTA

#define NUMBER_OF_SENSORS 4             ///< N�mero de sensores esperados 
float temperatures[NUMBER_OF_SENSORS];  ///< Espacio para almacenar los valores de temperatura
uint8_t tempAmbient_idx;                ///< �ndice del sensor que mide la temperatura ambiente
uint8_t tempRadiator_idx;               ///< �ndice del sensor que mide la temperatura del radiador del frigor�fico
uint8_t tempFridge_idx;                 ///< �ndice del sensor que mide la temperatura del frigor�fico
uint8_t tempFreezer_idx;                ///< �ndice del sensor que mide la temperatura del congelador
uint8_t numberOfDevices = 0;            ///< n�mero de sensores detectados. Deber�a ser igual a NUMBER_OF_SENSORS

#define ONE_WIRE_BUS D5                 ///< Pin de datos para los sensores de temperatura. D5 = GPIO16
#define TEMPERATURE_PRECISION 11        ///< N�mero de bits con los que se calcula la temperatura
OneWire oneWire (ONE_WIRE_BUS);         ///< Instancia OneWire para comunicar con los sensores de temperatura
DallasTemperature sensors (&oneWire);   ///< Pasar el bus de datos como referencia
const int minTemperature = -100;        ///< Temperatura m�nima v�lida

int fanOn = 0;

#define MEASURE_PERIOD 30000           ///< Periodo de medida de temperatura y consumo, y env�o de los datos al servicio en la nube

static const char* serverAddress = "192.168.5.18";
static const char* writeApiKey = "0e58ebf51f6436c663ec28bb62d5c547";


/********************************************//**
*  Funci�n para buscar la posici�on del valor m�ximo en el array de temperaturas
***********************************************/
int MaxValue (float temperatures[]) {

    float max_v = minTemperature;
    int max_i = 0;

    for (int i = 0; i < sizeof (temperatures) / sizeof (temperatures[0]); i++) {
        if (temperatures[i] > max_v) {
            max_v = temperatures[i];
            max_i = i;
        }
    }
    return max_i;

}


/********************************************//**
*  Funcion para ordenar los sensores de temperatura

*  Ordena, de mayor a menor, los �ndices de los sensores seg�n la secuencia
*       Radiador, Ambiente, Frigor�fico, Congelador
***********************************************/
void sortSensors () {
    //sensors.setWaitForConversion (false);  // makes it async
    sensors.requestTemperatures ();

    //  script to automatically allocate the postion of each sensor based on their temperature
    int max_i = 0;

    for (int i = 0; i < NUMBER_OF_SENSORS; i++) {
        temperatures[i] = sensors.getTempCByIndex (i);
    }

    max_i = MaxValue (temperatures);
    tempRadiator_idx = max_i;
    temperatures[max_i] = minTemperature;

    max_i = MaxValue (temperatures);
    tempAmbient_idx = max_i;
    temperatures[max_i] = minTemperature;

    max_i = MaxValue (temperatures);
    tempFridge_idx = max_i;
    temperatures[max_i] = minTemperature;

    max_i = MaxValue (temperatures);
    tempFreezer_idx = max_i;
    //  temperatures[max_i]= minTemperature;


#ifdef DEBUG
    Serial.print ("Posicion sensor radiador: ");
    Serial.println (tempRadiator_idx);
    Serial.print ("Posicion sensor ambiente: ");
    Serial.println (tempAmbient_idx);
    Serial.print ("Posicion sensor frigorifico: ");
    Serial.println (tempFridge_idx);
    Serial.print ("Posicion sensor congelador: ");
    Serial.println (tempFreezer_idx);

#endif


}


/********************************************//**
*  Funcion para iniciar los sensores de temperatura
***********************************************/
uint8_t initTempSensors () {
    DeviceAddress tempDeviceAddress;    ///< Almacenamiento temporal para las direcciones encontradas
    uint8_t numberOfDevices;            ///< N�mero de sensores encontrados

#ifdef DEBUG
    Serial.println ("Init Dallas Temperature Control Library");
#endif

    // Inicializar el bus de los sensores de temperatura 
    sensors.begin ();

    // Preguntar por el n�mero de sensores detectados
    numberOfDevices = sensors.getDeviceCount ();

#ifdef DEBUG
    Serial.print ("Locating devices...");
    Serial.print ("Found ");
    Serial.print (numberOfDevices, DEC);
    Serial.println (" devices.");

    Serial.print ("Parasite power is: ");
    if (sensors.isParasitePowerMode ())
        Serial.println ("ON");
    else
        Serial.println ("OFF");
#endif

    // Ajustar la precisi�n de todos los sensores
    sensors.setResolution (TEMPERATURE_PRECISION);

#ifdef DEBUG
    // Imprime la direcci�n de cada sensor
    for (int i = 0; i < numberOfDevices; i++) {
        // Search the wire for address
        if (sensors.getAddress (tempDeviceAddress, i)) {

            Serial.print ("Found device ");
            Serial.print (i, DEC);
            Serial.print ("Setting resolution to ");
            Serial.println (TEMPERATURE_PRECISION, DEC);

            Serial.print ("Resolution actually set to: ");
            Serial.print (sensors.getResolution (tempDeviceAddress), DEC);
            Serial.println ();


        } else {

            Serial.print ("Found ghost device at ");
            Serial.print (i, DEC);
            Serial.print (" but could not detect address. Check power and cabling");

        }

    }
#endif // DEBUG

    return numberOfDevices;
}

/********************************************//**
*  Setup
***********************************************/
void setup () {
    Serial.begin (115200);

#ifdef WIFI_MANAGER
    //leer datos de la flash
    //Iniciar wifi manager

    //Si hay que guardar
    //	Obtener datos de WifiManager
    //	guardar datos en flash
#else // WIFI_MANAGER
    //--------------------- Conectar a la red WiFi -------------------------
    WiFi.begin (WIFI_SSID, WIFI_PASS);

    Serial.printf ("Conectando a la red %s ", WIFI_SSID);

    while (!WiFi.isConnected ()) {
        Serial.print (".");
        delay (500);
    }
    //----------------------------------------------------------------------
#endif // WIFI_MANAGER
    Serial.printf ("Conectado!!! IP: %s", WiFi.localIP().toString().c_str());

    MDNS.begin ("FridgeSaverMonitor"); //Iniciar servidor MDNS para llamar al dispositivo como FridgeSaverMonitor.local

    OTASetup ();

    //Iniciar RemoteDebug

    //Iniciar NTP

    // Control del ventilador
    pinMode (D3, INPUT_PULLUP); // D3 = GPIO0. Bot�n para controlar la activaci�n del ventilador
    analogWrite (D1, 0); // D1 = GPIO5. Pin PWM para controlar el ventilador

   // Iniciar el sensor de corriente
#ifdef EMONLIB
    emon1.current (A0, 3.05);             // Current: input pin, calibration.
#endif

    // Iniciar sensores de temperatura comprobando que el n�mero de sensores es el necesario
    while (numberOfDevices != NUMBER_OF_SENSORS) {
        numberOfDevices = initTempSensors ();
        if (numberOfDevices != NUMBER_OF_SENSORS) {
            Serial.printf ("Error en el numero de sensores: %d\n", numberOfDevices);
            delay (1000);
        }
    }

    sortSensors (); // Asignar los sensores autom�ticamente
}

/********************************************//**
*  Funci�n para obtener la medida de consumo en Vatios
***********************************************/
double getPower () {
    const int fanThreshold = 60;
    const int mainsVoltage = 230;

    double watts;

#ifdef EMONLIB
    watts = emon1.calcIrms (1480) * mainsVoltage;  // Calculate Irms * V. Aparent power
#endif

#ifdef EMONLIB
    if (digitalRead (D3) && watts > fanThreshold) { // Si el consumo > 60W
#else
    if (digitalRead (D3)) {
#endif
        analogWrite (D1, 850); // Encender ventilador
        fanOn = 850;
    } else {
        analogWrite (D1, 0);
        fanOn = 0;
    }

    return watts;
}

/********************************************//**
*  Env�a los datos a la plataforma de EmonCMS
***********************************************/
int8_t sendDataEmonCMS (float tempRadiator,
                        float tempAmbient, 
                        float tempFridge, 
                        float tempFreezer, 
                        double watts,
                        int fanOn) {

    WiFiClientSecure client; ///< Cliente TCP con SSL
    const unsigned int maxTimeout = 5000; ///< Tiempo maximo de espera a la respuesta del servidor
    unsigned long timeout; ///< Contador para acumilar el tiempo de espera
    char* tempStr; ///< Cadena temporal para almacenar los numeros como texto

    // Conecta al servidor
    if (!client.connect (serverAddress, 443)) {
#ifdef DEBUG
        Serial.printf ("Error al conectar al servidor EmonCMS en %s", serverAddress);
#endif
        return -1; // Error de conexi�n
    }

    // Compone la peticion HTTP
    String httpRequest = "GET /input/post.json?node=IoTFridgeSaver&fulljson={";
    dtostrf (tempRadiator, 3, 3, tempStr);
    httpRequest += "\"tempRadiator\":" + String (tempStr) + ",";
    dtostrf (tempAmbient, 3, 3, tempStr);
    httpRequest += "\"tempAmbient\":" + String (tempStr) + ",";
    dtostrf (tempFridge, 3, 3, tempStr);
    httpRequest += "\"tempFridge\":" + String (tempStr) + ",";
    dtostrf (tempFreezer, 3, 3, tempStr);
    httpRequest += "\"tempFreezer\":" + String (tempStr) + ",";
    dtostrf (watts, 3, 3, tempStr);
    httpRequest += "\"watts\":" + String (tempStr) + ",";
    httpRequest += "\"watts\":" + fanOn;
    httpRequest += ",}&apikey=" + String (writeApiKey) + " HTTP/1.1\r\n";
    httpRequest += "Host: " + String (serverAddress) + "\r\n\r\n";

#ifdef DEBUG
    Serial.printf ("%s: Request; ->\n %s\n", __FUNCTION__, httpRequest.c_str ());
#endif \\ DEBUG

    // Envia la peticion
    client.print (httpRequest);

    // Espera la respuesta
    timeout = millis ();
    while (!client.available ()) {
        if (millis () - timeout > maxTimeout) {
#ifdef DEBUG
            Serial.printf ("%s: Firebase client Timeout !", __FUNCTION__);
#endif
            client.stop ();
            return -2; // Timeout
        }
    }

    // Recupera la respuesta
    while (client.available ()) {
        String line = client.readStringUntil ('\n');
#ifdef DEBUG
        Serial.printf ("%s: Response: %s\n", __FUNCTION__, line.c_str ());
#endif
    }

    client.stop (); // Desconecta el cliente

#ifdef DEBUG
    Serial.printf ("%s: Data sent !!!\n", __FUNCTION__);
#endif // DEBUG

    return 0; // OK
}

/********************************************//**
*  Bucle principal
***********************************************/
void loop () {
    static long lastRun = 0;
    double watts;

    ArduinoOTA.handle ();

    if (millis () - lastRun > MEASURE_PERIOD) {
        lastRun = millis ();

        sensors.requestTemperatures ();
        temperatures[tempRadiator_idx] = sensors.getTempCByIndex (tempRadiator_idx);
        temperatures[tempAmbient_idx] = sensors.getTempCByIndex (tempAmbient_idx);
        temperatures[tempFridge_idx] = sensors.getTempCByIndex (tempFridge_idx);
        temperatures[tempFreezer_idx] = sensors.getTempCByIndex (tempFreezer_idx);

        watts = getPower ();

        sendDataEmonCMS (temperatures[tempRadiator_idx], temperatures[tempAmbient_idx], temperatures[tempFridge_idx], temperatures[tempFreezer_idx], watts, fanOn);

    }


}
