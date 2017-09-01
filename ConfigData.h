#pragma once
#ifndef _CONFIGDATA_h
#define _CONFIGDATA_h

#define DEBUG_ENABLED
#define WIFI_MANAGER

#ifndef WIFI_MANAGER
const char* WIFI_SSID = "Iot";
const char* WIFI_PASS = "1234567890";
#endif // WIFI_MANAGER



#define MEASURE_PERIOD 30000        ///< Periodo de medida de temperatura y consumo, y envío de los datos al servicio en la nube

const int fanThreshold = 60;        ///< Valor en vatios por encima del cual se enciende el ventilador



#endif
