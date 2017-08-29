#pragma once
#ifndef _GLOBAL_h
#define _GLOBAL_h

#define DEBUG_ENABLED
#define WIFI_MANAGER

#ifndef WIFI_MANAGER
const char* WIFI_SSID = "NOMBRE_DE_MI_RED";
const char* WIFI_PASS = "CONTRASEÑA";
#endif // WIFI_MANAGER

String emonCMSserverAddress;  ///< Dirección del servidor EmonCMS
String emonCMSwriteApiKey; ///< API key del usuario

#define MEASURE_PERIOD 30000        ///< Periodo de medida de temperatura y consumo, y envío de los datos al servicio en la nube

const int fanThreshold = 60;        ///< Valor en vatios por encima del cual se enciende el ventilador
int mainsVoltage = 230;       ///< Tensión de alimentación


#endif