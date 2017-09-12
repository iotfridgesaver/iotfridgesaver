// 
// 
// 

#include "AverageCalculator.h"
#ifdef DEBUG_ENABLED
#include <TimeLib.h>
#endif

extern void debugPrintf (uint8_t debugLevel, const char* format, ...);

float_t AverageCalculator::feed (float value) {
    unsigned long nowTime = millis ();
    unsigned int interval = (nowTime - lastMillis)/1000; // Valor en segundos
    lastMillis = nowTime;

    accumTime += interval;
    sum += (float)(value*(float)interval);
    if (accumTime != 0)
        average = sum / accumTime;
#ifdef DEBUG_ENABLED
    debugPrintf (Debug.INFO, "%02d:%02d:%02d\n", hour (), minute (), second ());
    debugPrintf (Debug.INFO, "%s --> Valor alimentado: %f\n", __FUNCTION__, value);
    debugPrintf (Debug.INFO, "%s --> Intervalo: %u\n", __FUNCTION__, interval);
    debugPrintf (Debug.INFO, "%s --> Tiempo acumulado: %u\n", __FUNCTION__, accumTime);
    debugPrintf (Debug.INFO, "%s --> Acumulador temperatura: %f\n", __FUNCTION__, sum);
    debugPrintf (Debug.INFO, "%s --> Media: %f\n", __FUNCTION__, average);
#endif

    return average;
}

float_t AverageCalculator::reset () {
    float lastAve = average;
    sum = 0;
    accumTime = 0;
    //lastMillis = millis ();
    debugPrintf (Debug.INFO, "%s ------> Media: %f\n", __FUNCTION__, average);

    return lastAve;
}

