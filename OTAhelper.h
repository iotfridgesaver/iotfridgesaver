/**
* @file OTAhelper.h
* \~English
* @brief Helper function to setup OTA update via Arduino IDE
*
* \~Spanish
* @brief Función auxiliar para configurar la actualización OTA con IDE Arduino
*/
#ifndef _OTAHELPER_h
#define _OTAHELPER_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "Arduino.h"
#else
	#include "WProgram.h"
#endif
//#include <RemoteDebug.h>

/**
@brief Inicia el servidor OTA, con todo lo necesario.
*/
void OTASetup();

#endif

