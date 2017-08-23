// OTAhelper.h

#ifndef _OTAHELPER_h
#define _OTAHELPER_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "Arduino.h"
#else
	#include "WProgram.h"
#endif
//#include <RemoteDebug.h>

void OTASetup();

#endif

