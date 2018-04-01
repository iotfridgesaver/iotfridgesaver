/**
* @file iotfridgesaver.ino
* \~English
* @brief Fridge efficiency monitoring system
*
* IoT Fridge saver's aims is to monitor fridge and freezer temperatures and power consumption.
* Besides it monitors ambient and radiator temperature to check efficiency.
*
* \~Spanish
* @brief Sistema de monitorización de eficiencia energética para frigoríficos
*
* Iot Fridge Saver es un sistema que monitoriza la temperatura de congeladores y frigoríficos junto
* con su consumo de electricidad.
* Además mide también la temperatura ambiente y la del radiador para comprobar la eficiencia.
*
* @mainpage Sistema de monitorización de eficiencia energética para frigoríficos
*
* @section intro Introducción
*/

#include "ConfigData.h"
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <OneWire.h>
#include <DallasTemperature.h>          // https://github.com/milesburton/Arduino-Temperature-Control-Library
#include <ArduinoOTA.h>
#include <ArduinoJson.h>
#include "FS.h"
#include "OTAhelper.h"
#ifdef WIFI_MANAGER
#include <WiFiManager.h>
#include "WifiManagerSetup.h"
#endif // WIFI_MANAGER
#include <OneButton.h>
#ifdef DEBUG_ENABLED
#include <RemoteDebug.h>
#endif // DEBUG_ENABLED

//#include <NtpClientLib.h>

#ifdef EMONLIB
#include "EmonLib.h"                    // https://github.com/openenergymonitor/EmonLib
EnergyMonitor emon1;                    ///<\~Spanish Instancia del monitor de consumo
#endif // EMONLIB
#if defined MQTT_POWER_INPUT || defined MQTT_FEED_SEND
#include <PubSubClient.h>
WiFiClient client;
PubSubClient mqttClient (client);
#endif

bool OTAupdating;                       ///<\~Spanish Verdadero si se está haciendo una actualización OTA
#ifdef DEBUG_ENABLED
RemoteDebug Debug;                      ///<\~Spanish Remote debug telnet server instance
#endif // DEBUG_ENABLED

#define NUMBER_OF_SENSORS 4             ///<\~English Number of expected temp sensors \~Spanish Número de sensores esperados 
float temperatures[NUMBER_OF_SENSORS];  ///<\~Spanish Espacio para almacenar los valores de temperatura
float aveTemperatures[NUMBER_OF_SENSORS];
double fridgeWatts;                     ///<\~Spanish Potencia instantánea consumida por el conjunto
double houseWatts;                      ///<\~Spanish Potencia instantánea total consumida. Usualmente potencia de la acometida principal de la casa
uint8_t tempAmbient_idx;                ///<\~Spanish Índice del sensor que mide la temperatura ambiente
uint8_t tempRadiator_idx;               ///<\~Spanish Índice del sensor que mide la temperatura del radiador del frigorífico
uint8_t tempFridge_idx;                 ///<\~Spanish Índice del sensor que mide la temperatura del frigorífico
uint8_t tempFreezer_idx;                ///<\~Spanish Índice del sensor que mide la temperatura del congelador
uint8_t numberOfDevices = 0;            ///<\~Spanish Número de sensores detectados. Debería ser igual a NUMBER_OF_SENSORS

#define ONE_WIRE_BUS D2                 ///<\~Spanish Pin de datos para los sensores de temperatura. D5 = GPIO16
#define TEMPERATURE_PRECISION 11        ///<\~Spanish Número de bits con los que se calcula la temperatura
OneWire oneWire (ONE_WIRE_BUS);         ///<\~Spanish Instancia OneWire para comunicar con los sensores de temperatura
DallasTemperature sensors (&oneWire);   ///<\~Spanish Pasar el bus de datos como referencia
const int minTemperature = -100;        ///<\~Spanish Temperatura mínima válida

OneButton button (FAN_ENABLE_BUTTON, true); ///<\~Spanish Objeto que controla el botón. True significa activo a nivel bajo
bool fanEnabled = true;                 ///<\~Spanish Verdadero si el ventilador está activado
int fanSpeed = 0;                       ///<\~Spanish Velocidad del ventilador

bool shouldSaveConfig = false;          ///<\~Spanish Verdadero si WiFi Manager activa una configuración nueva
bool configLoaded = false;              ///<\~Spanish Indica si se ha cargado la configuración de la flash. Sin uso actualmente

#ifdef WIFI_MANAGER
MyWiFiManager wifiManager;              // WiFi Manager. Configura los datos WiFi y otras configuraciones
const char * configFileName = "config.json";  ///<\~Spanish Nombre del archivo de configuración que se genera en la flash
#else
const char* WIFI_SSID = "NOMBRE_DE_MI_RED"; ///<\~Spanish Nombre de la red WiFi. Solo si no se usa WiFiManager
const char* WIFI_PASS = "CONTRASEÑA";       ///<\~Spanish Contraseña de la red WiFi. Solo si no se usa WiFiManager
#endif // WIFI_MANAGER

config_t config = { ///<\~Spanish Configuración del dispositivo
#ifndef WIFI_MANAGER
    "SERVIDOR_EMONCMS", // emonCMSserverAddress
    "RUTA_EMONCMS",     // emonCMSserverPath
    "CLAVE_API_EMONCMS",// emonCMSwriteApiKey
    230,                // mainsVoltage
#if defined MQTT_POWER_INPUT || defined MQTT_FEED_SEND
    "BROKER_MQTT",      // mqttServerName
    1883,               // mqttServerPort
#ifdef MQTT_POWER_INPUT
    "TOPIC_CONSUMO_FRIGO",       // mqttFridgePowerTopic
    "TOPIC_CONSUMO_TOTAL_CASA",  // mqttTotalPowerTopic
#endif // MQTT_POWER_INPUT
    false               // mqttStarted
#endif

#else
    "",                 // emonCMSserverAddress
    "",                 // emonCMSserverPath
    "",                 // emonCMSwriteApiKey
    230,                // mainsVoltage
#if defined MQTT_POWER_INPUT || defined MQTT_FEED_SEND
    "",                 // mqttServerName
    1883,               // mqttServerPort
#ifdef MQTT_POWER_INPUT
    "",                 // mqttFridgePowerTopic
    "",                 // mqttTotalPowerTopic
#endif // MQTT_POWER_INPUT
    false               // mqttStarted
#endif

#endif // WIFI_MANAGER
};

int MaxValue (float *temperatures, uint8_t size);
void sortSensors ();
double getPower ();
uint8_t initTempSensors ();
#ifdef DEBUG_ENABLED
void debugPrintf (uint8_t debugLevel, const char* format, ...);
#endif // DEBUG_ENABLED
#ifdef WIFI_MANAGER
void configModeCallback ();
void getCustomData (MyWiFiManager &wifiManager);
void loadConfigData ();
void saveConfigData ();
void startWifiManager (MyWiFiManager &wifiManager);
void long_click ();
#endif // WIFI_MANAGER
void button_click ();
int8_t sendDataEmonCMS (float tempRadiator, float tempAmbient, float tempFridge, float tempFreezer, double watts, double totalWatts, int fanOn);


#ifdef DEBUG_ENABLED
/**
@brief Envuelve la función Debug.printf para ayudar al deshabilitar la salida de debug.

@param[in]     debugLevel Nivel de debug para la librería RemoteDebug.
@param[in]     format Cadena a imprimir con marcadores de formatos.
@param[in]     ... Lista de variables.
*/
void debugPrintf (uint8_t debugLevel, const char* format, ...) {
    if (Debug.isActive (debugLevel)) {
        va_list argptr;
        va_start (argptr, format);
        char temp[64];
        //char* buffer = temp;
        /*size_t len = */vsnprintf (temp, sizeof (temp), format, argptr);
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
#else
#define debugPrintf(...)
#endif // DEBUG_ENABLED

#if defined MQTT_POWER_INPUT || defined MQTT_FEED_SEND
/**
@brief Conecta al servidor MQTT al iniciar el programa y en caso de desconexión.

También se subscribe a los topics de consumo del frigorífico y de la vivienda
*/
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
#ifdef MQTT_POWER_INPUT
            mqttClient.subscribe (config.mqttFridgePowerTopic.c_str ());
            mqttClient.subscribe (config.mqttTotalPowerTopic.c_str ());
#endif // MQTT_POWER_INPUT
        } else {
            debugPrintf (Debug.INFO, "failed, rc=%d try again in 5 seconds\n", mqttClient.state ());
            // Wait 5 seconds before retrying
            delay (5000);
        }
    }
}
#endif

/**
@brief Función para buscar la posición del valor máximo en el array de temperaturas.

@param[in]     temperatures Array de valores en formato float.
@param[in]     size Tamaño del Array.
@returns       El índice del mayor valor del array
*/
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


/**
@brief Función para ordenar los sensores de temperatura.

Ordena, de mayor a menor, los índices de los sensores según la secuencia
Radiador, Ambiente, Frigorífico, Congelador
*/
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
    
    debugPrintf (Debug.INFO, "Posicion sensor radiador: %d\n", tempRadiator_idx);
    debugPrintf (Debug.INFO, "Posicion sensor ambiente: %d\n", tempAmbient_idx);
    debugPrintf (Debug.INFO, "Posicion sensor frigorifico: %d\n", tempFridge_idx);
    debugPrintf (Debug.INFO, "Posicion sensor congelador: %d\n", tempFreezer_idx);

}

/**
@brief Obtiene la medida de consumo en Vatios

@returns Valor de la medida en Vatios, en formato double
*/
double getPower () {
    double watts = 0;

#ifdef EMONLIB
    watts = emon1.calcIrms (1480) * config.mainsVoltage;  // Calculate Irms * V. Aparent power
#endif // EMONLIB

#if defined EMONLIB || defined MQTT_POWER_INPUT
    if (fanEnabled && watts > fanThreshold) { // Si el consumo > 60W
#else
    if (fanEnabled) {
#endif // EMONLIB
        analogWrite (FAN_PWM_PIN, 850); // Encender ventilador
        fanSpeed = 850;
    } else {
        analogWrite (FAN_PWM_PIN, 0);
        fanSpeed = 0;
    }

    return watts;
}



/**
@brief Inicia los sensores de temperatura.

@returns       Número de sensores se han encontrado
*/
uint8_t initTempSensors () {
    DeviceAddress tempDeviceAddress;    // Almacenamiento temporal para las direcciones encontradas
    uint8_t numberOfDevices;            // Número de sensores encontrados

    debugPrintf (Debug.INFO, "Init Dallas Temperature Control Library\n");

    // Inicializar el bus de los sensores de temperatura 
    sensors.begin ();

    // Preguntar por el número de sensores detectados
    numberOfDevices = sensors.getDeviceCount ();

    debugPrintf (Debug.INFO, "Locating devices...Found %u devices.\n", numberOfDevices);
    debugPrintf (Debug.INFO, "Parasite power is: %s\n", sensors.isParasitePowerMode ()?"ON":"OFF");

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

#ifdef WIFI_MANAGER
/**
@brief Se ejecuta cuando WiFiManager se ha conectado a la red WiFi ha recuperado la configuración del formulario.

Activa la bandera que controla el inicio el almacenamiento de la configuración personalizada en la memoria flash, en el sistema de archivos SPIFFS
*/
void configModeCallback () {
    debugPrintf (Debug.INFO, "%s: Guardar configuración\n", __FUNCTION__);
    shouldSaveConfig = true;
}

/**
@brief Rellena las variables de consfiguración con los datos que entrega WiFiManager.

Comprueba él formato de los datos numéricos, para evitar errores de ejecución.
Si hay un error reinicia el dispositivo

@param[in]     wifiManager Referencia al objeto WiFiManager.
*/
void getCustomData (MyWiFiManager &wifiManager) {
    //	Obtener datos de WifiManager
    config.emonCMSserverAddress = wifiManager.getEmonCMSserverAddress ();
    config.emonCMSserverPath = wifiManager.getEmonCMSserverPath ();
    config.emonCMSwriteApiKey = wifiManager.getEmonCMSwriteApiKey ();
    config.mainsVoltage = wifiManager.getMainsVoltage ();
#if defined MQTT_POWER_INPUT || defined MQTT_FEED_SEND
    config.mqttServerName = wifiManager.getMQTTserver();
    config.mqttServerPort = wifiManager.getMQTTport ();
#ifdef MQTT_POWER_INPUT
    config.mqttFridgePowerTopic = wifiManager.getFridgeMQTTtopic ();
    config.mqttTotalPowerTopic = wifiManager.getTotalMQTTtopic ();
#endif // MQTT_POWER_INPUT
#endif

    debugPrintf (Debug.INFO, "Servidor: %s\n", config.emonCMSserverAddress.c_str ());
    debugPrintf (Debug.INFO, "Ruta: %s\n", config.emonCMSserverPath.c_str ());
    debugPrintf (Debug.INFO, "API Key: %s\n", config.emonCMSwriteApiKey.c_str ());
    debugPrintf (Debug.INFO, "Tensión: %d\n", config.mainsVoltage);

#if defined MQTT_POWER_INPUT || defined MQTT_FEED_SEND
    debugPrintf (Debug.INFO, "Servidor MQTT: %s\n", config.mqttServerName.c_str());
    debugPrintf (Debug.INFO, "Puerto MQTT: %d\n", config.mqttServerPort);
#ifdef MQTT_POWER_INPUT
    debugPrintf (Debug.INFO, "Topic MQTT frigorífico: %s\n", config.mqttFridgePowerTopic.c_str());
    debugPrintf (Debug.INFO, "Topic MQTT total: %s\n", config.mqttTotalPowerTopic.c_str ());
#endif // MQTT_POWER_INPUT
#endif

    if (config.mainsVoltage == 0) {
        config.mainsVoltage = 230;
    }

}

/**
@brief Carga los datos de configuración desde el sistema de archivos SPIFFS, desde la flash.

Inicia el sistema de archivos si éste es incorrecto.
*/
void loadConfigData () {
    //SPIFFS.format();     // Borra el sistema de archivos, solo para pruebas

    //read configuration from FS json
    debugPrintf (Debug.INFO, "Montando sistema de archivos...\n");

    if (SPIFFS.begin ()) {
        debugPrintf (Debug.INFO, "Sistema de archivos montado. Abriendo %s\n", configFileName);
        if (SPIFFS.exists (configFileName)) {
            //file exists, reading and loading
            debugPrintf (Debug.INFO, "Leyendo el archivo de configuración: %s\n", configFileName);
            File configFile = SPIFFS.open (configFileName, "r");
            if (configFile) {
                debugPrintf (Debug.INFO, "Archivo de configuración abierto\n");
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
                    debugPrintf (Debug.INFO, "\nparsed json\n");

                    config.emonCMSserverAddress = json.get<String> ("emonCMSserver");
                    config.emonCMSserverPath = json.get<String> ("emonCMSpath");
                    config.emonCMSwriteApiKey = json.get<String> ("emonCMSapiKey");
                    config.mainsVoltage = json.get<int> ("mainsVoltage");

#if defined MQTT_POWER_INPUT || defined MQTT_FEED_SEND
                    config.mqttServerName = json.get<String> ("mqttServerName");
                    config.mqttServerPort = json.get<int> ("mqttServerPort");
#ifdef MQTT_POWER_INPUT
                    config.mqttFridgePowerTopic = json.get<String> ("mqttFridgePowerTopic");
                    config.mqttTotalPowerTopic = json.get<String> ("mqttTotalPowerTopic");
#endif // MQTT_POWER_INPUT
#endif

                    debugPrintf (Debug.INFO, "emonCMSserverAddress: %s\n", config.emonCMSserverAddress.c_str ());
                    debugPrintf (Debug.INFO, "emonCMSserverPath: %s\n", config.emonCMSserverPath.c_str ());
                    debugPrintf (Debug.INFO, "emonCMSwriteApiKey: %s\n", config.emonCMSwriteApiKey.c_str ());
                    debugPrintf (Debug.INFO, "mainsVoltage: %d\n", config.mainsVoltage);

#if defined MQTT_POWER_INPUT || defined MQTT_FEED_SEND
                    debugPrintf (Debug.INFO, "mqttServerName: %s\n", config.mqttServerName.c_str ());
                    debugPrintf (Debug.INFO, "mqttServerPort: %d\n", config.mqttServerPort);
#ifdef MQTT_POWER_INPUT
                    debugPrintf (Debug.INFO, "mqttFridgePowerTopic: %s\n", config.mqttFridgePowerTopic.c_str ());
                    debugPrintf (Debug.INFO, "mqttTotalPowerTopic: %s\n", config.mqttTotalPowerTopic.c_str ());
#endif // MQTT_POWER_INPUT
#endif

                    //strcpy (mqtt_port, json["mqtt_port"]);
                    //strcpy (blynk_token, json["blynk_token"]);
                    if (config.emonCMSserverAddress != "" && config.emonCMSwriteApiKey != "")
                        configLoaded = true;
                } 
                else {
                    debugPrintf (Debug.INFO, "Error al leer el archivo de configuración\n");
                }
            } 
            else {
                debugPrintf (Debug.INFO, "Error al abrir el archivo de configuración\n");
            }
        } 
        else {
            debugPrintf (Debug.INFO, "El archivo de configuración no existe\n");
        }

        SPIFFS.end ();
    } else {
        debugPrintf (Debug.INFO, "Error al abrir el sistema de archivos. Formateando\n");
        SPIFFS.format ();
        SPIFFS.end ();
        ESP.reset ();
    }
    //end read

}

/**
@brief Guarda los datos de configuración en el sistema de archivos SPIFFS, desde la flash.
*/
void saveConfigData () {
    debugPrintf (Debug.INFO, "saving config\n");
    if (SPIFFS.begin ()) {
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.createObject ();
        json["emonCMSserver"] = config.emonCMSserverAddress;
        json["emonCMSpath"] = config.emonCMSserverPath;
        json["emonCMSapiKey"] = config.emonCMSwriteApiKey;
        json["mainsVoltage"] = config.mainsVoltage;

#if defined MQTT_POWER_INPUT || defined MQTT_FEED_SEND
        json["mqttServerName"] = config.mqttServerName;
        json["mqttServerPort"] = config.mqttServerPort;
#ifdef MQTT_POWER_INPUT 
        json["mqttFridgePowerTopic"] = config.mqttFridgePowerTopic;
        json["mqttTotalPowerTopic"] = config.mqttTotalPowerTopic;
#endif // MQTT_POWER_INPUT
#endif

        File configFile = SPIFFS.open (configFileName, "w");
        if (!configFile) {
            debugPrintf (Debug.INFO, "Error al abrir el archivo de configuración\n");
            return;
        }

#ifdef DEBUG_ENABLED
        json.prettyPrintTo (Debug);
#endif // DEBUG_ENABLED
        json.printTo (configFile);
        configFile.close ();
    } 
    else {
        debugPrintf (Debug.INFO, "Error al montar el sistema de archivos\n");
    }
    SPIFFS.end ();
    //end save
}

/**
@brief Inicia el servidor WiFiManager.
*/
void startWifiManager (MyWiFiManager &wifiManager) {

    wifiManager.setSaveConfigCallback (configModeCallback);
    wifiManager.setConfig (config, configLoaded);
    wifiManager.init ();
}
#endif //WIFI_MANAGER

/**
@brief Se activa al hacer una pulsación corta en el botón.

Conmuta el funcionamiento del ventilador
*/
void button_click () {
    fanEnabled = !fanEnabled;
    debugPrintf (Debug.INFO, "Ventilador %s\n", fanEnabled ? "activado" : "desactivado");
}

#ifdef WIFI_MANAGER
/**
@brief Se activa al hacer una pulsación larga en el botón.

Reinicia el dispositivo a la configuración de fábrica y lo reinicia para que pida
de nuevo los datos de configuración al usuario
*/
void long_click () {
    debugPrintf (Debug.INFO, "---------------Reset config\n");
    //wifiManager.resetSettings ();
    WiFi.disconnect (true);
    delay (1000);
    ESP.reset ();
}
#endif // WIFI_MANAGER

/*void processSyncEvent (NTPSyncEvent_t ntpEvent) {
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
}*/

#ifdef MQTT_POWER_INPUT
/**
@brief Decodifica los valores de consumo recibidos por MQTT.

@param[in] topic Topic del mensaje recibido
@param[in] payload Cadena de texto con el contenido del mensaje
@param[in] length Longitud del contenido
*/
void getPowerMeasurement (char* topic, byte* payload, unsigned int length) {
    String powerStr = "";
    //String topicStr = String (topic);
    
    for (unsigned int i = 0; i < length; i++) {
        powerStr += (char)payload[i];
    }

    debugPrintf (Debug.INFO, "Message arrived [%s]: %s\n", topic, powerStr.c_str());
    
    if (!strcmp(topic, config.mqttFridgePowerTopic.c_str())) {
        fridgeWatts = strtod (powerStr.c_str(), NULL);
        debugPrintf (Debug.INFO, "Valor traducido %f\n", fridgeWatts);
    }
    else if (!strcmp (topic, config.mqttTotalPowerTopic.c_str ())) {
        houseWatts = strtod (powerStr.c_str (), NULL);
        debugPrintf (Debug.INFO, "Valor traducido %f\n", houseWatts);
    }

}
#endif

/********************************************//**
*  Setup
***********************************************/
void setup () {
    Serial.begin (115200);
    // Control del ventilador y pequeño "aceleron"
    analogWrite (FAN_PWM_PIN, 900); // D1 = GPIO5. Pin PWM para controlar el ventilador
    delay(500);
    analogWrite (FAN_PWM_PIN, 0); // D1 = GPIO5. Pin PWM para controlar el ventilador
    pinMode (FAN_ENABLE_BUTTON, INPUT);
    button.attachClick (button_click);
#ifdef WIFI_MANAGER
    button.setPressTicks (t_longPress);
    button.attachPress (long_click);
#endif // WIFI_MANAGER
#ifdef DEBUG_ENABLED
#ifdef DEBUG_SERIAL
    Debug.setSerialEnabled (true);
#else
    Debug.showColors (true); // Habilita los colores si la salida no es Serie, ya que no funciona bien.
#endif // DEBUG_SERIAL
#endif // DEBUG_ENABLED

#ifdef WIFI_MANAGER
    //leer datos de la flash
    loadConfigData ();

    // Si no se han configurado los datos del servidor borrar la configuración
    if (config.emonCMSserverAddress == "" || config.emonCMSserverPath == "" || config.emonCMSwriteApiKey == ""
#if defined MQTT_POWER_INPUT || defined MQTT_FEED_SEND
        || config.mqttServerName == ""
#ifdef MQTT_POWER_INPUT
        || config.mqttFridgePowerTopic == "" || config.mqttTotalPowerTopic == ""
#endif // MQTT_POWER_INPUT
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

    Serial.printf ("Conectado!!! IP: %s\n", WiFi.localIP ().toString ().c_str ());

    //NTP.begin ("es.pool.ntp.org", 1, true);

#ifdef DEBUG_ENABLED
    Debug.begin ("IoTFridgeSaver");
#endif // DEBUG_ENABLED

    debugPrintf (Debug.INFO, "emonCMSserverAddress: %s\n", config.emonCMSserverAddress.c_str ());
    debugPrintf (Debug.INFO, "emonCMSserverPath: %s\n", config.emonCMSserverPath.c_str ());
    debugPrintf (Debug.INFO, "emonCMSwriteApiKey: %s\n", config.emonCMSwriteApiKey.c_str ());
    debugPrintf (Debug.INFO, "mainsVoltage: %d\n", config.mainsVoltage);
#if defined MQTT_POWER_INPUT || defined MQTT_FEED_SEND
    debugPrintf (Debug.INFO, "mqttServerName: %s\n", config.mqttServerName.c_str());
    debugPrintf (Debug.INFO, "mqttServerPort: %d\n", config.mqttServerPort);
#ifdef MQTT_POWER_INPUT
    debugPrintf (Debug.INFO, "mqttFridgePowerTopic: %s\n", config.mqttFridgePowerTopic.c_str());
    debugPrintf (Debug.INFO, "mqttTotalPowerTopic: %s\n", config.mqttTotalPowerTopic.c_str ());
#endif // MQTT_POWER_INPUT
#endif

#if defined MQTT_POWER_INPUT || defined MQTT_FEED_SEND
    if (config.mqttServerName != "" && config.mqttServerPort != 0) {
        mqttClient.setServer (config.mqttServerName.c_str (), config.mqttServerPort);
#ifdef MQTT_POWER_INPUT
        mqttClient.setCallback (getPowerMeasurement);
#endif // MQTT_POWER_INPUT
        config.mqttStarted = true;
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
            debugPrintf (Debug.INFO, "Error en el numero de sensores: %d\n", numberOfDevices);
            //ArduinoOTA.handle ();
            button.tick ();
#ifdef DEBUG_ENABLED
            Debug.handle ();
#endif // DEBUG_ENABLED
            delay (500);

        }
    }

#endif
    sortSensors (); // Asignar los sensores automáticamente

    //NTP.onNTPSyncEvent (processSyncEvent);
}

/**
@brief  Envía los datos a la plataforma de EmonCMS.

@param[in]  tempRadiator    Temperatura del radiador del frigorífico
@param[in]  tempAmbient     Temperatura ambiente en el exterior del frigorífico
@param[in]  tempFridge      Temperatura del interior del frigorífico
@param[in]  tempFreezer     Temperatura del interior del congelador
@param[in]  watts           Consumo del frigorífico
@param[in]  totalWatts      Consumo total de la casa
@param[in]  fanOn           Velocidad del ventilador

@returns -1 si hay error de conexión.
@returns -2 si ha pasado el tiempo de espera al enviar los datos o esperar la respuesta
@returns  0 si el envío ha sido correcto
*/
int8_t sendDataEmonCMS (float tempRadiator,
                        float tempAmbient, 
                        float tempFridge, 
                        float tempFreezer, 
                        double watts,
                        double totalWatts,
                        int fanOn) {

    WiFiClientSecure client; // Cliente TCP con SSL
    const unsigned int maxTimeout = 5000; // Tiempo maximo de espera a la respuesta del servidor
    unsigned long timeout; // Contador para acumilar el tiempo de espera
    char tempStr[10]; // Cadena temporal para almacenar los numeros como texto

    // Conecta al servidor
    if (!client.connect (config.emonCMSserverAddress.c_str (), 443)) {
        debugPrintf (Debug.INFO, "Error al conectar al servidor EmonCMS en %s\n", config.emonCMSserverAddress.c_str ());
        return -1; // Error de conexión
    }

    // Compone la peticion HTTP
    String httpRequest = "GET "+ config.emonCMSserverPath +"/input/post.json?node=IoTFridgeSaver&fulljson={";
    dtostrf (tempRadiator, 3, 3, tempStr);
    httpRequest += "\"tempRadiator\":" + String (tempStr) + ",";
    dtostrf (tempAmbient, 3, 3, tempStr);
    httpRequest += "\"tempAmbient\":" + String (tempStr) + ",";
    /*if (aveTempAmbient > -100) {
        dtostrf (tempAmbient, 3, 3, tempStr);
        httpRequest += "\"tempAmbient_ave\":" + String (tempStr) + ",";
    }*/
    dtostrf (tempFridge, 3, 3, tempStr);
    httpRequest += "\"tempFridge\":" + String (tempStr) + ",";
    dtostrf (tempFreezer, 3, 3, tempStr);
    httpRequest += "\"tempFreezer\":" + String (tempStr) + ",";
    dtostrf (watts, 3, 3, tempStr);
    httpRequest += "\"watts\":" + String (tempStr) + ",";
    dtostrf (totalWatts, 3, 3, tempStr);
    httpRequest += "\"house_watts\":" + String (tempStr) + ",";
    httpRequest += "\"fan\":" + String(fanOn);
    httpRequest += "}&apikey=" + config.emonCMSwriteApiKey + " HTTP/1.1\r\n";
    httpRequest += "Host: " + config.emonCMSserverAddress + "\r\n\r\n";

    debugPrintf (Debug.INFO, "%s: Request; ->\n %s\n", __FUNCTION__, httpRequest.c_str ());

    // Envia la peticion
    client.print (httpRequest);

    // Espera la respuesta
    timeout = millis ();
    while (!client.available ()) {
        if (millis () - timeout > maxTimeout) {
            debugPrintf (Debug.INFO, "%s: EmonCMS client Timeout !", __FUNCTION__);
            client.stop ();
            return -2; // Timeout
        }
    }

    // Recupera la respuesta
    while (client.available ()) {
        String line = client.readStringUntil ('\n');
        debugPrintf (Debug.INFO, "%s: Response: %s\n", __FUNCTION__, line.c_str ());
    }

    client.stop (); // Desconecta el cliente

    debugPrintf (Debug.INFO, "%s: Data sent !!!\n", __FUNCTION__);

    return 0; // OK
}

/**
@brief Bucle principal
*/
void loop () {
    static long lastRun = 0;

    ArduinoOTA.handle ();
    button.tick ();
#ifdef DEBUG_ENABLED
    Debug.handle ();
#endif // DEBUG_ENABLED

    if (millis () - lastRun > MEASURE_PERIOD) {
        lastRun = millis ();


#ifndef TEST
        sensors.requestTemperatures ();
        temperatures[tempRadiator_idx] = sensors.getTempCByIndex (tempRadiator_idx);
        temperatures[tempAmbient_idx] = sensors.getTempCByIndex (tempAmbient_idx);
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
#ifndef MQTT_POWER_INPUT
        watts = random (0,100);
        houseWatts = random (0, 3000);
#endif //MQTT_POWER_INPUT
#endif

        debugPrintf (Debug.INFO, "Temperatura radiador: %f\n", temperatures[tempRadiator_idx]);
        debugPrintf (Debug.INFO, "Temperatura ambiente: %f\n", temperatures[tempAmbient_idx]);
        debugPrintf (Debug.INFO, "Temperatura frigorifico: %f\n", temperatures[tempFridge_idx]);
        debugPrintf (Debug.INFO, "Temperatura congelador: %f\n", temperatures[tempFreezer_idx]);
        debugPrintf (Debug.INFO, "Consumo frigorífico: %f\n", fridgeWatts);
        debugPrintf (Debug.INFO, "Consumo total: %f\n", houseWatts);

        sendDataEmonCMS (temperatures[tempRadiator_idx], temperatures[tempAmbient_idx], temperatures[tempFridge_idx], temperatures[tempFreezer_idx], fridgeWatts, houseWatts, fanSpeed);
#ifdef MQTT_FEED_SEND
        mqttClient.publish ("iotfridgesaver/tempRadiator", String (temperatures[tempRadiator_idx]).c_str());
        mqttClient.publish ("iotfridgesaver/tempAmbient", String (temperatures[tempAmbient_idx]).c_str ());
        mqttClient.publish ("iotfridgesaver/tempFridge", String (temperatures[tempFridge_idx]).c_str ());
        mqttClient.publish ("iotfridgesaver/tempFreezer", String (temperatures[tempFreezer_idx]).c_str ());
        mqttClient.publish ("iotfridgesaver/watts", String (fridgeWatts).c_str ());
        mqttClient.publish ("iotfridgesaver/wattsTotal", String (houseWatts).c_str ());
        mqttClient.publish ("iotfridgesaver/fanSpeed", String (fanSpeed).c_str ());
#endif // MQTT_FEED_SEND

    }

#if defined MQTT_POWER_INPUT || defined MQTT_FEED_SEND
    if (config.mqttStarted) {
        if (!client.connected ()) {
            debugPrintf (Debug.INFO, "MQTT Reconnect");
            reconnect ();
        }
        mqttClient.loop ();

    }
#endif

}
