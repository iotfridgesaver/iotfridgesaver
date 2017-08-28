#pragma once
#ifndef _GLOBAL_h
#define _GLOBAL_h

#define DEBUG_ENABLED

const char* WIFI_SSID = "NOMBRE_DE_MI_RED";
const char* WIFI_PASS = "CONTRASEÑA";

static const char* emonCMSserverAddress = "cloud.iotfridgesaver.com";  ///< Dirección del servidor EmonCMS
static const char* emonCMSwriteApiKey = "API KEY DE ESCRITURA"; ///< API key del usuario

#define MEASURE_PERIOD 30000        ///< Periodo de medida de temperatura y consumo, y envío de los datos al servicio en la nube

const int fanThreshold = 60;        ///< Valor en vatios por encima del cual se enciende el ventilador
const int mainsVoltage = 230;       ///< Tensión de alimentacion


#endif
