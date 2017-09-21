#include "AverageCalculator.h"
#include "ConfigData.h"
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <OneWire.h>
#include <DallasTemperature.h>          // https://github.com/milesburton/Arduino-Temperature-Control-Library
#include <ArduinoOTA.h>
#include <ArduinoJson.h>
#include "FS.h"
#include "OTAhelper.h"
#include "WifiManagerSetup.h"
#include <WiFiManager.h>
#include <OneButton.h>

#include <NtpClientLib.h>
#include <TimeAlarms.h>
#include <RemoteDebug.h>

#ifdef EMONLIB
#include "EmonLib.h"                    // https://github.com/openenergymonitor/EmonLib
EnergyMonitor emon1;                    ///< Instancia del monitor de consumo
#elif defined MQTT
#include <PubSubClient.h>
WiFiClient client;
PubSubClient mqttClient (client);
#endif

bool OTAupdating;                       ///< Verdadero si se está haciendo una actualización OTA
RemoteDebug Debug;

#define NUMBER_OF_SENSORS 4             ///< Número de sensores esperados 
float temperatures[NUMBER_OF_SENSORS];  ///< Espacio para almacenar los valores de temperatura
float aveTemperatures[NUMBER_OF_SENSORS];
double fridgeWatts;                     ///< Potencia instantánea consumida por el conjunto
double houseWatts;                      ///< Potencia instantánea total consumida. Usualmente potencia de la acometida principal de la casa
uint8_t tempAmbient_idx;                ///< Índice del sensor que mide la temperatura ambiente
uint8_t tempRadiator_idx;               ///< Índice del sensor que mide la temperatura del radiador del frigorífico
uint8_t tempFridge_idx;                 ///< Índice del sensor que mide la temperatura del frigorífico
uint8_t tempFreezer_idx;                ///< Índice del sensor que mide la temperatura del congelador
uint8_t numberOfDevices = 0;            ///< Número de sensores detectados. Debería ser igual a NUMBER_OF_SENSORS

#define ONE_WIRE_BUS D2                 ///< Pin de datos para los sensores de temperatura. D5 = GPIO16
#define TEMPERATURE_PRECISION 11        ///< Número de bits con los que se calcula la temperatura
OneWire oneWire (ONE_WIRE_BUS);         ///< Instancia OneWire para comunicar con los sensores de temperatura
DallasTemperature sensors (&oneWire);   ///< Pasar el bus de datos como referencia
const int minTemperature = -100;        ///< Temperatura mínima válida

OneButton button (FAN_ENABLE_BUTTON, true);
bool fanEnabled = true;                 ///< Verdadero si el ventilador está activado
int fanSpeed = 0;                       ///< Velocidad del ventilador

bool shouldSaveConfig = false;          ///< Verdadero si WiFi Manager activa una configuración nueva
bool configLoaded = false;

String emonCMSserverAddress = "";  ///< Dirección del servidor EmonCMS
String emonCMSserverPath = "";
String emonCMSwriteApiKey = ""; ///< API key del usuario
int mainsVoltage = 230;       ///< Tensión de alimentación
#ifdef MQTT
String mqttServerName = "";
uint16_t mqttServerPort = 1883;
String mqttFridgePowerTopic = "";
String mqttTotalPowerTopic = "";
bool mqttStarted = false;
#endif
const char *configFileName = "config.json";

TimeAlarmsClass alarmDaily;
AverageCalculator ambientAverage;
bool sendAverage = false;
bool timeChanged = false;
AlarmID_t alarmID;

void debugPrintf (uint8_t debugLevel, const char* format, ...) {
    if (Debug.isActive (debugLevel)) {
        va_list argptr;
        va_start (argptr, format);
        char temp[64];
        char* buffer = temp;
        size_t len = vsnprintf (temp, sizeof (temp), format, argptr);
        Debug.print (temp);
        va_end (argptr);
        /*if (len > sizeof (temp) - 1) {
        buffer = new char[len + 1];
        if (!buffer) {
        return;
        }
        va_start (argptr, format);
        vsnprintf (buffer, len + 1, format, argptr);
        va_end (argptr);
        }
        Debug.write ((const uint8_t*)buffer, len);
        if (buffer != temp) {
        delete[] buffer;
        }*/
        //Debug.printf (format, argptr);
    }
}

#ifdef MQTT
void reconnect () {
    // Loop until we're reconnected
    while (!mqttClient.connected ()) {
        debugPrintf (Debug.INFO, "Attempting MQTT connection...");
        // Attempt to connect
        if (mqttClient.connect ("IotFridgeSaver")) {
            debugPrintf (Debug.INFO, "connected\n");
            // Once connected, publish an announcement...
            mqttClient.publish ("outTopic", "IotFridgeSaver/hello world");
            // ... and resubscribe
            //mqttClient.subscribe("inTopic");
            mqttClient.subscribe (mqttFridgePowerTopic.c_str ());
        } else {
            debugPrintf (Debug.INFO, "failed, rc=%d try again in 5 seconds\n", mqttClient.state ());
            // Wait 5 seconds before retrying
            delay (5000);
        }
    }
}
#endif

/********************************************//**
*  Función para buscar la posición del valor máximo en el array de temperaturas
***********************************************/
int MaxValue (float *temperatures, uint8_t size) {

    float max_v = minTemperature;
    int max_i = 0;

    for (int i = 0; i < size; i++) {
        if (temperatures[i] > max_v) {
            max_v = temperatures[i];
            max_i = i;
        }
    }
    return max_i;

}


/********************************************//**
*  Funcion para ordenar los sensores de temperatura

*  Ordena, de mayor a menor, los índices de los sensores según la secuencia
*       Radiador, Ambiente, Frigorífico, Congelador
***********************************************/
void sortSensors () {
    //sensors.setWaitForConversion (false);  // makes it async
    int max_i = 0;

#ifndef TEST
    sensors.requestTemperatures ();


    //  script to automatically allocate the postion of each sensor based on their temperature

    for (int i = 0; i < NUMBER_OF_SENSORS; i++) {
        temperatures[i] = sensors.getTempCByIndex (i);
    }
#else
    temperatures[0] = 30;
    temperatures[1] = 35;
    temperatures[2] = 6;
    temperatures[3] = -18;
#endif
    max_i = MaxValue (temperatures, NUMBER_OF_SENSORS);
    tempRadiator_idx = max_i;
    temperatures[max_i] = minTemperature;

    max_i = MaxValue (temperatures, NUMBER_OF_SENSORS);
    tempAmbient_idx = max_i;
    temperatures[max_i] = minTemperature;

    max_i = MaxValue (temperatures, NUMBER_OF_SENSORS);
    tempFridge_idx = max_i;
    temperatures[max_i] = minTemperature;

    max_i = MaxValue (temperatures, NUMBER_OF_SENSORS);
    tempFreezer_idx = max_i;
    //  temperatures[max_i]= minTemperature;


#ifdef DEBUG_ENABLED
    debugPrintf (Debug.INFO, "Posicion sensor radiador: %d\n", tempRadiator_idx);
    debugPrintf (Debug.INFO, "Posicion sensor ambiente: %d\n", tempAmbient_idx);
    debugPrintf (Debug.INFO, "Posicion sensor frigorifico: %d\n", tempFridge_idx);
    debugPrintf (Debug.INFO, "Posicion sensor congelador: %d\n", tempFreezer_idx);

#endif


}


/********************************************//**
*  Función para iniciar los sensores de temperatura
***********************************************/
uint8_t initTempSensors () {
    DeviceAddress tempDeviceAddress;    ///< Almacenamiento temporal para las direcciones encontradas
    uint8_t numberOfDevices;            ///< Número de sensores encontrados

#ifdef DEBUG_ENABLED
    debugPrintf (Debug.INFO, "Init Dallas Temperature Control Library\n");
#endif

    // Inicializar el bus de los sensores de temperatura 
    sensors.begin ();

    // Preguntar por el número de sensores detectados
    numberOfDevices = sensors.getDeviceCount ();

#ifdef DEBUG_ENABLED
    debugPrintf (Debug.INFO, "Locating devices...Found %u devices.\n", numberOfDevices);
    debugPrintf (Debug.INFO, "Parasite power is: %s\n", sensors.isParasitePowerMode ()?"ON":"OFF");
#endif

    // Ajustar la precisión de todos los sensores
    sensors.setResolution (TEMPERATURE_PRECISION);

#ifdef DEBUG_ENABLED
    // Imprime la dirección de cada sensor
    for (int i = 0; i < numberOfDevices; i++) {
        // Search the wire for address
        if (sensors.getAddress (tempDeviceAddress, i)) {
            debugPrintf (Debug.INFO, "Found device %u. Setting resolution to %d\n", i, TEMPERATURE_PRECISION);
            debugPrintf (Debug.INFO, "Resolution actually set to: %u\n", sensors.getResolution (tempDeviceAddress));
        } else {
            debugPrintf (Debug.INFO, "Found ghost device at %u but could not detect address. Check power and cabling\n", i);
        }

    }
#endif // DEBUG

    return numberOfDevices;
}

void configModeCallback () {
#ifdef DEBUG_ENABLED
    debugPrintf (Debug.INFO, "%s: Guardar configuración\n", __FUNCTION__);
#endif // DEBUG_ENABLED
    shouldSaveConfig = true;
}

void getCustomData (MyWiFiManager &wifiManager) {
    //	Obtener datos de WifiManager
    emonCMSserverAddress = wifiManager.getEmonCMSserverAddress ();
    emonCMSserverPath = wifiManager.getEmonCMSserverPath ();
    emonCMSwriteApiKey = wifiManager.getEmonCMSwriteApiKey ();
    mainsVoltage = wifiManager.getMainsVoltage ();
#ifdef MQTT
    mqttServerName = wifiManager.getMQTTserver();
    mqttServerPort = wifiManager.getMQTTport ();
    mqttFridgePowerTopic = wifiManager.getMQTTtopic ();
#endif

#ifdef DEBUG_ENABLED
    debugPrintf (Debug.INFO, "Servidor: %s\n", emonCMSserverAddress.c_str ());
    debugPrintf (Debug.INFO, "Ruta: %s\n", emonCMSserverPath.c_str ());
    debugPrintf (Debug.INFO, "API Key: %s\n", emonCMSwriteApiKey.c_str ());
    debugPrintf (Debug.INFO, "Tensión: %d\n", mainsVoltage);

#ifdef MQTT
    debugPrintf (Debug.INFO, "Servidor MQTT: %s\n", mqttServerName.c_str());
    debugPrintf (Debug.INFO, "Puerto MQTT: %d\n", mqttServerPort);
    debugPrintf (Debug.INFO, "Topic MQTT: %s\n", mqttFridgePowerTopic.c_str());
#endif
#endif // DEBUG_ENABLED

    if (mainsVoltage == 0) {
#ifdef DEBUG_ENABLED
        debugPrintf (Debug.INFO, "ERROR. Tensión debe ser un número\n");
#endif // DEBUG_ENABLED
        wifiManager.resetSettings ();
        delay (1000);
        ESP.reset ();
    }

}

void loadConfigData () {
    //clean FS, for testing
    //SPIFFS.format();

    //read configuration from FS json
#ifdef DEBUG_ENABLED
    debugPrintf (Debug.INFO, "Montando sistema de archivos...\n");
#endif // DEBUG_ENABLED

    if (SPIFFS.begin ()) {
#ifdef DEBUG_ENABLED
        debugPrintf (Debug.INFO, "Sistema de archivos montado. Abriendo %s\n", configFileName);
#endif // DEBUG_ENABLED
        if (SPIFFS.exists (configFileName)) {
            //file exists, reading and loading
#ifdef DEBUG_ENABLED
            debugPrintf (Debug.INFO, "Leyendo el archivo de configuración: %s\n", configFileName);
#endif // DEBUG_ENABLED
            File configFile = SPIFFS.open (configFileName, "r");
            if (configFile) {
#ifdef DEBUG_ENABLED
                debugPrintf (Debug.INFO, "Archivo de configuración abierto\n");
#endif // DEBUG_ENABLED
                size_t size = configFile.size ();
                // Allocate a buffer to store contents of the file.
                std::unique_ptr<char[]> buf (new char[size]);

                configFile.readBytes (buf.get (), size);
                DynamicJsonBuffer jsonBuffer;
                JsonObject& json = jsonBuffer.parseObject (buf.get ());
#ifdef DEBUG_ENABLED
                if (Debug.isActive(Debug.INFO))
                    json.printTo (Debug);
#endif // DEBUG_ENABLED
                if (json.success ()) {
#ifdef DEBUG_ENABLED
                    debugPrintf (Debug.INFO, "\nparsed json\n");
#endif // DEBUG_ENABLED

                    //strcpy (mqtt_server, json["mqtt_server"]);
                    emonCMSserverAddress = json.get<String> ("emonCMSserver");
                    emonCMSserverPath = json.get<String> ("emonCMSpath");
                    emonCMSwriteApiKey = json.get<String> ("emonCMSapiKey");
                    mainsVoltage = json.get<int> ("mainsVoltage");

#ifdef MQTT
                    mqttServerName = json.get<String> ("mqttServerName");
                    mqttServerPort = json.get<int> ("mqttServerPort");
                    mqttFridgePowerTopic = json.get<String> ("mqttPowerTopic");
#endif

#ifdef DEBUG_ENABLED
                    debugPrintf (Debug.INFO, "emonCMSserverAddress: %s\n", emonCMSserverAddress.c_str ());
                    debugPrintf (Debug.INFO, "emonCMSserverPath: %s\n", emonCMSserverPath.c_str ());
                    debugPrintf (Debug.INFO, "emonCMSwriteApiKey: %s\n", emonCMSwriteApiKey.c_str ());
                    debugPrintf (Debug.INFO, "mainsVoltage: %d\n", mainsVoltage);

#ifdef MQTT
                    debugPrintf (Debug.INFO, "mqttServerName: %s\n", mqttServerName.c_str ());
                    debugPrintf (Debug.INFO, "mqttServerPort: %d\n", mqttServerPort);
                    debugPrintf (Debug.INFO, "mqttPowerTopic: %s\n", mqttFridgePowerTopic.c_str ());
#endif

#endif // DEBUG_ENABLED

                    //strcpy (mqtt_port, json["mqtt_port"]);
                    //strcpy (blynk_token, json["blynk_token"]);
                    configLoaded = true;
                } 
#ifdef DEBUG_ENABLED
                else {
                    debugPrintf (Debug.INFO, "Error al leer el archivo de configuración\n");
                }
#endif // DEBUG_ENABLED
            } 
#ifdef DEBUG_ENABLED
            else {
                debugPrintf (Debug.INFO, "Error al abrir el archivo de configuración\n");
            }
#endif // DEBUG_ENABLED
        } 
#ifdef DEBUG_ENABLED
        else {
            debugPrintf (Debug.INFO, "El archivo de configuración no existe\n");
        }
#endif // DEBUG_ENABLED

        SPIFFS.end ();
    } else {
#ifdef DEBUG_ENABLED
        debugPrintf (Debug.INFO, "Error al abrir el sistema de archivos. Formateando\n");
#endif // DEBUG_ENABLED
        SPIFFS.format ();
        SPIFFS.end ();
        ESP.reset ();
    }
    //end read

}

void saveConfigData () {
#ifdef DEBUG_ENABLED
    debugPrintf (Debug.INFO, "saving config\n");
#endif // DEBUG_ENABLED
    if (SPIFFS.begin ()) {
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.createObject ();
        json["emonCMSserver"] = emonCMSserverAddress;
        json["emonCMSpath"] = emonCMSserverPath;
        json["emonCMSapiKey"] = emonCMSwriteApiKey;
        json["mainsVoltage"] = mainsVoltage;

#ifdef MQTT
        json["mqttServerName"] = mqttServerName;
        json["mqttServerPort"] = mqttServerPort;
        json["mqttPowerTopic"] = mqttFridgePowerTopic;
#endif

        File configFile = SPIFFS.open (configFileName, "w");
        if (!configFile) {
#ifdef DEBUG_ENABLED
            debugPrintf (Debug.INFO, "Error al abrir el archivo de configuración\n");
#endif // DEBUG_ENABLED
            return;
        }

#ifdef DEBUG_ENABLED
        json.prettyPrintTo (Debug);
#endif // DEBUG_ENABLED
        json.printTo (configFile);
        configFile.close ();
    } 
#ifdef DEBUG_ENABLED
    else {
        debugPrintf (Debug.INFO, "Error al montar el sistema de archivos\n");
    }
#endif // DEBUG_ENABLED
    SPIFFS.end ();
    //end save
}

void startWifiManager (MyWiFiManager &wifiManager) {

    wifiManager.setSaveConfigCallback (configModeCallback);
    wifiManager.init ();
}

void button_click () {
    fanEnabled = !fanEnabled;
#ifdef DEBUG_ENABLED
    debugPrintf (Debug.INFO, "Ventilador %s\n", fanEnabled ? "activado" : "desactivado");
#endif // DEBUG_ENABLED
}

void long_click () {
#ifdef DEBUG_ENABLED
    debugPrintf (Debug.INFO, "---------------Reset config\n");
#endif // DEBUG_ENABLED
    //wifiManager.resetSettings ();
    WiFi.disconnect (true);
    delay (1000);
    ESP.reset ();
}

void getDailyAmbientAverage () {
        aveTemperatures[tempAmbient_idx] = ambientAverage.reset ();
        sendAverage = true;
}

void processSyncEvent (NTPSyncEvent_t ntpEvent) {
    if (ntpEvent) {
        debugPrintf (Debug.INFO, "%s Time Sync error: ", __FUNCTION__);
        if (ntpEvent == noResponse)
            debugPrintf (Debug.INFO, "NTP server not reachable\n");
        else if (ntpEvent == invalidAddress)
            debugPrintf (Debug.INFO, "Invalid NTP server address\n");
    } else {
        debugPrintf (Debug.INFO, "%s Got NTP time: %s\n", __FUNCTION__, NTP.getTimeDateString (NTP.getLastNTPSync ()).c_str());
        timeChanged = true;
    }
}

#ifdef MQTT
void getPowerMeasurement (char* topic, byte* payload, unsigned int length) {
    String powerStr = "";
    //String topicStr = String (topic);
    
    for (int i = 0; i < length; i++) {
        powerStr += (char)payload[i];
    }

#ifdef DEBUG_ENABLED
    debugPrintf (Debug.INFO, "Message arrived [%s]: %s\n", topic, powerStr.c_str());
#endif
    
    if (!strcmp(topic, "emon/ccost/1")) {
        fridgeWatts = strtod (powerStr.c_str(), NULL);
#ifdef DEBUG_ENABLED
        debugPrintf (Debug.INFO, "Valor traducido %f\n", fridgeWatts);
#endif
    }
    else if (!strcmp (topic, "emon/ccost/0")) {
        houseWatts = strtod (powerStr.c_str (), NULL);
#ifdef DEBUG_ENABLED
        debugPrintf (Debug.INFO, "Valor traducido %f\n", houseWatts);
#endif
    }

}
#endif

/********************************************//**
*  Setup
***********************************************/
void setup () {
    MyWiFiManager wifiManager;              ///< WiFi Manager. Configura los datos WiFi y otras configuraciones

    Serial.begin (115200);
    // Control del ventilador y pequeño "aceleron"
    analogWrite (FAN_PWM_PIN, 900); // D1 = GPIO5. Pin PWM para controlar el ventilador
    delay(500);
    analogWrite (FAN_PWM_PIN, 0); // D1 = GPIO5. Pin PWM para controlar el ventilador
    pinMode (FAN_ENABLE_BUTTON, INPUT);
    button.attachClick (button_click);
    button.setPressTicks (t_longPress);
    button.attachPress (long_click);

#ifdef DEBUG_SERIAL
    Debug.setSerialEnabled (true);
#else
    Debug.showColors (true);
#endif // DEBUG_SERIAL

#ifdef WIFI_MANAGER
    //leer datos de la flash
    configFileName = "\config.json";
    loadConfigData ();

    // Si no se han configurado los datos del servidor borrar la configuración
    if (emonCMSserverAddress == "" || emonCMSserverPath == "" || emonCMSwriteApiKey == ""
#ifdef MQTT
        || mqttServerName == "" || mqttFridgePowerTopic == ""
#endif
        ) {
        wifiManager.resetSettings ();
    }

    startWifiManager (wifiManager);

    //Si hay que guardar
    if (shouldSaveConfig) {
        getCustomData (wifiManager);

        //	guardar datos en flash
        saveConfigData ();
    }

#else // WIFI_MANAGER
    //--------------------- Conectar a la red WiFi -------------------------
    WiFi.begin (WIFI_SSID, WIFI_PASS);

    debugPrintf (Debug.INFO, "Conectando a la red %s ", WIFI_SSID);

    while (!WiFi.isConnected ()) {
        debugPrintf (Debug.INFO, ".");
        delay (500);
    }
    //----------------------------------------------------------------------
#endif // WIFI_MANAGER
    Debug.begin ("IoTFridgeSaver");
    
    debugPrintf (Debug.INFO, "Conectado!!! IP: %s\n", WiFi.localIP ().toString ().c_str ());

    NTP.begin ("es.pool.ntp.org", 1, true);

#ifdef DEBUG_ENABLED
    debugPrintf (Debug.INFO, "emonCMSserverAddress: %s\n", emonCMSserverAddress.c_str ());
    debugPrintf (Debug.INFO, "emonCMSserverPath: %s\n", emonCMSserverPath.c_str ());
    debugPrintf (Debug.INFO, "emonCMSwriteApiKey: %s\n", emonCMSwriteApiKey.c_str ());
    debugPrintf (Debug.INFO, "mainsVoltage: %d\n", mainsVoltage);
#ifdef MQTT
    debugPrintf (Debug.INFO, "mqttServerName: %s\n", mqttServerName.c_str());
    debugPrintf (Debug.INFO, "mqttServerPort: %d\n", mqttServerPort);
    debugPrintf (Debug.INFO, "mqttPowerTopic: %s\n", mqttFridgePowerTopic.c_str());

#endif
#endif // DEBUG_ENABLED

#ifdef MQTT
    if (mqttServerName != "" && mqttServerPort != 0) {
        mqttClient.setServer (mqttServerName.c_str (), mqttServerPort);
        mqttClient.setCallback (getPowerMeasurement);
        mqttStarted = true;
    }
#endif

    MDNS.begin ("FridgeSaverMonitor"); //Iniciar servidor MDNS para llamar al dispositivo como FridgeSaverMonitor.local

    OTASetup ();

    //Iniciar RemoteDebug

    //Iniciar NTP

#ifndef TEST
   // Iniciar el sensor de corriente
#ifdef EMONLIB
    emon1.current (A0, 3.05);             // Current: input pin, calibration.
#endif

    // Iniciar sensores de temperatura comprobando que el número de sensores es el necesario
    while (numberOfDevices != NUMBER_OF_SENSORS) {
        numberOfDevices = initTempSensors ();
        if (numberOfDevices != NUMBER_OF_SENSORS) {
#ifdef DEBUG_ENABLED
            debugPrintf (Debug.INFO, "Error en el numero de sensores: %d\n", numberOfDevices);
#endif // DEBUG_ENABLED
            delay (1000);
        }
    }

#endif
    sortSensors (); // Asignar los sensores automáticamente

#ifndef TEST
    Alarm.alarmRepeat (0, 0, 1, getDailyAmbientAverage);
#else
    Alarm.alarmRepeat (10, 21, 1, getDailyAmbientAverage);
#endif

    NTP.onNTPSyncEvent (processSyncEvent);
}

/********************************************//**
*  Función para obtener la medida de consumo en Vatios
***********************************************/
double getPower () {
    double watts;

#ifdef EMONLIB
    watts = emon1.calcIrms (1480) * mainsVoltage;  // Calculate Irms * V. Aparent power
#endif

#if defined EMONLIB || defined MQTT
    //fanEnabled = digitalRead (FAN_ENABLE_BUTTON);
    if (fanEnabled && watts > fanThreshold) { // Si el consumo > 60W
#else
    if (fanEnabled) {
#endif
        analogWrite (FAN_PWM_PIN, 850); // Encender ventilador
        fanSpeed = 850;
    } else {
        analogWrite (FAN_PWM_PIN, 0);
        fanSpeed = 0;
    }

    return watts;
}

/********************************************//**
*  Envía los datos a la plataforma de EmonCMS
***********************************************/
int8_t sendDataEmonCMS (float tempRadiator,
                        float tempAmbient, 
                        float tempFridge, 
                        float tempFreezer, 
                        double watts,
                        double totalWatts,
                        int fanOn,
                        float aveTempAmbient = -100) {

    WiFiClientSecure client; ///< Cliente TCP con SSL
    const unsigned int maxTimeout = 5000; ///< Tiempo maximo de espera a la respuesta del servidor
    unsigned long timeout; ///< Contador para acumilar el tiempo de espera
    char tempStr[10]; ///< Cadena temporal para almacenar los numeros como texto

    // Conecta al servidor
    if (!client.connect (emonCMSserverAddress.c_str (), 443)) {
#ifdef DEBUG_ENABLED
        debugPrintf (Debug.INFO, "Error al conectar al servidor EmonCMS en %s\n", emonCMSserverAddress.c_str ());
#endif
        return -1; // Error de conexión
    }

    // Compone la peticion HTTP
    String httpRequest = "GET "+ emonCMSserverPath +"/input/post.json?node=IoTFridgeSaver&fulljson={";
    dtostrf (tempRadiator, 3, 3, tempStr);
    httpRequest += "\"tempRadiator\":" + String (tempStr) + ",";
    dtostrf (tempAmbient, 3, 3, tempStr);
    httpRequest += "\"tempAmbient\":" + String (tempStr) + ",";
    if (aveTempAmbient > -100) {
        dtostrf (tempAmbient, 3, 3, tempStr);
        httpRequest += "\"tempAmbient_ave\":" + String (tempStr) + ",";
    }
    dtostrf (tempFridge, 3, 3, tempStr);
    httpRequest += "\"tempFridge\":" + String (tempStr) + ",";
    dtostrf (tempFreezer, 3, 3, tempStr);
    httpRequest += "\"tempFreezer\":" + String (tempStr) + ",";
    dtostrf (watts, 3, 3, tempStr);
    httpRequest += "\"watts\":" + String (tempStr) + ",";
    dtostrf (totalWatts, 3, 3, tempStr);
    httpRequest += "\"house_watts\":" + String (tempStr) + ",";
    httpRequest += "\"fan\":" + String(fanOn);
    httpRequest += "}&apikey=" + emonCMSwriteApiKey + " HTTP/1.1\r\n";
    httpRequest += "Host: " + emonCMSserverAddress + "\r\n\r\n";

#ifdef DEBUG_ENABLED
    Serial.printf("%s: Request; ->\n %s\n", __FUNCTION__, httpRequest.c_str ());
#endif // DEBUG_ENABLED

    // Envia la peticion
    client.print (httpRequest);

    // Espera la respuesta
    timeout = millis ();
    while (!client.available ()) {
        if (millis () - timeout > maxTimeout) {
#ifdef DEBUG_ENABLED
            debugPrintf (Debug.INFO, "%s: EmonCMS client Timeout !", __FUNCTION__);
#endif
            client.stop ();
            return -2; // Timeout
        }
    }

    // Recupera la respuesta
    while (client.available ()) {
        String line = client.readStringUntil ('\n');
#ifdef DEBUG_ENABLED
        Serial.printf( "%s: Response: %s\n", __FUNCTION__, line.c_str ());
#endif
    }

    client.stop (); // Desconecta el cliente

#ifdef DEBUG_ENABLED
    debugPrintf (Debug.INFO, "%s: Data sent !!!\n", __FUNCTION__);
#endif // DEBUG

    return 0; // OK
}

/********************************************//**
*  Bucle principal
***********************************************/
void loop () {
    static long lastRun = 0;

    ArduinoOTA.handle ();
    button.tick ();
    Alarm.delay (0);
    Debug.handle ();

    if (timeChanged) {
#ifndef TEST
        alarmID = Alarm.alarmRepeat (0, 0, 1, getDailyAmbientAverage);
#else
        alarmID = Alarm.alarmRepeat (16, 46, 0, getDailyAmbientAverage);
#endif
        timeChanged = false;
    }

    if (millis () - lastRun > MEASURE_PERIOD) {
        lastRun = millis ();


#ifndef TEST
        sensors.requestTemperatures ();
        temperatures[tempRadiator_idx] = sensors.getTempCByIndex (tempRadiator_idx);
        temperatures[tempAmbient_idx] = sensors.getTempCByIndex (tempAmbient_idx);
        ambientAverage.feed (temperatures[tempAmbient_idx]);
        temperatures[tempFridge_idx] = sensors.getTempCByIndex (tempFridge_idx);
        temperatures[tempFreezer_idx] = sensors.getTempCByIndex (tempFreezer_idx);
#ifdef EMONLIB
        fridgeWatts = getPower ();
        //houseWatts = getPower (0);
#endif //EMONLIB
#else
        temperatures[tempRadiator_idx] = random (3500, 4000)/(float)100;
        temperatures[tempAmbient_idx] = random (2500, 3000) / (float)100;
        ambientAverage.feed (temperatures[tempAmbient_idx]);
        temperatures[tempFridge_idx] = random (0, 1000) / (float)100;
        temperatures[tempFreezer_idx] = random (-2000, -1500) / (float)100;
#ifndef MQTT
        watts = random (0,100);
#endif //MQTT
#endif

#ifdef DEBUG_ENABLED
        debugPrintf (Debug.INFO, "Temperatura radiador: %f\n", temperatures[tempRadiator_idx]);
        debugPrintf (Debug.INFO, "Temperatura ambiente: %f\n", temperatures[tempAmbient_idx]);
        debugPrintf (Debug.INFO, "Temperatura frigorifico: %f\n", temperatures[tempFridge_idx]);
        debugPrintf (Debug.INFO, "Temperatura congelador: %f\n", temperatures[tempFreezer_idx]);
        debugPrintf (Debug.INFO, "Consumo frigorífico: %f\n", fridgeWatts);
        debugPrintf (Debug.INFO, "Consumo total: %f\n", houseWatts);
#endif // DEBUG_ENABLED
        
        if (sendAverage) {
            sendDataEmonCMS (temperatures[tempRadiator_idx], temperatures[tempAmbient_idx], temperatures[tempFridge_idx], temperatures[tempFreezer_idx], fridgeWatts, houseWatts, fanSpeed, aveTemperatures[tempAmbient_idx]);
#ifdef MQTT
            mqttClient.publish ("iotfridgesaver/tempRadiator", String (temperatures[tempRadiator_idx]).c_str ());
            mqttClient.publish ("iotfridgesaver/tempAmbient", String (temperatures[tempAmbient_idx]).c_str ());
            mqttClient.publish ("iotfridgesaver/tempAmbient_ave", String (aveTemperatures[tempAmbient_idx]).c_str (), true);
            mqttClient.publish ("iotfridgesaver/tempFridge", String (temperatures[tempFridge_idx]).c_str ());
            mqttClient.publish ("iotfridgesaver/tempFreezer", String (temperatures[tempFreezer_idx]).c_str ());
            mqttClient.publish ("iotfridgesaver/watts", String (fridgeWatts).c_str ());
            mqttClient.publish ("iotfridgesaver/fanSpeed", String (fanSpeed).c_str ());
#endif
            sendAverage = false;
        } else {
            sendDataEmonCMS (temperatures[tempRadiator_idx], temperatures[tempAmbient_idx], temperatures[tempFridge_idx], temperatures[tempFreezer_idx], fridgeWatts, houseWatts, fanSpeed);
#ifdef MQTT
            mqttClient.publish ("iotfridgesaver/tempRadiator", String (temperatures[tempRadiator_idx]).c_str());
            mqttClient.publish ("iotfridgesaver/tempAmbient", String (temperatures[tempAmbient_idx]).c_str ());
            mqttClient.publish ("iotfridgesaver/tempFridge", String (temperatures[tempFridge_idx]).c_str ());
            mqttClient.publish ("iotfridgesaver/tempFreezer", String (temperatures[tempFreezer_idx]).c_str ());
            mqttClient.publish ("iotfridgesaver/watts", String (fridgeWatts).c_str ());
            mqttClient.publish ("iotfridgesaver/fanSpeed", String (fanSpeed).c_str ());
#endif
        }

    }

#ifdef MQTT
    if (mqttStarted) {
        if (!client.connected ()) {
            debugPrintf (Debug.INFO, "MQTT Reconnect");
            reconnect ();
        }
        mqttClient.loop ();

    }
#endif

}
