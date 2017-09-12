#pragma once
#ifndef _CONFIGDATA_h
#define _CONFIGDATA_h

#define DEBUG_ENABLED
#define DEBUG_SERIAL
#define WIFI_MANAGER

#ifndef WIFI_MANAGER
const char* WIFI_SSID = "NOMBRE_DE_MI_RED";
const char* WIFI_PASS = "CONTRASEÑA";
#endif // WIFI_MANAGER


#define FAN_ENABLE_BUTTON   D3      ///< Pin donde conectar el botón para activar el ventilador
#define FAN_PWM_PIN         D1      ///< Pin para el control de la velocidad del ventilador
#define MEASURE_PERIOD      60000   ///< Periodo de medida de temperatura y consumo, y envío de los datos al servicio en la nube

const int t_longPress =     10000;   ///< Tiempo para identificar pulsación larga

const int fanThreshold =    60;     ///< Valor en vatios por encima del cual se enciende el ventilador



#endif