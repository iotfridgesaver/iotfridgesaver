/**
* @file ConfigData.h
* \~English
* @brief General configuration settings
*
* \~Spanish
* @brief Parámetros generales de configuración
*/
#pragma once
#ifndef _CONFIGDATA_h
#define _CONFIGDATA_h

#define DEBUG_ENABLED   ///<\~Spanish Activa la salida de debug por Telnet
#define DEBUG_SERIAL    ///<\~Spanish Activa la salida de debug por puerto serie
#define WIFI_MANAGER    ///<\~Spanish Activa la configuración con WiFiManager

#define FAN_ENABLE_BUTTON   D3      ///<\~Spanish Pin donde conectar el botón para activar el ventilador
#define FAN_PWM_PIN         D1      ///<\~Spanish Pin para el control de la velocidad del ventilador
#define MEASURE_PERIOD      60000   ///<\~Spanish Periodo de medida de temperatura y consumo en milisegundos, y envío de los datos al servicio en la nube

const int t_longPress =     10000;   ///<\~Spanish Tiempo en milisegundos para identificar pulsación larga

const int fanThreshold =    60;     ///<\~Spanish Valor en vatios por encima del cual se enciende el ventilador



#endif