// 
// 
// 

#include "AverageCalculator.h"
#ifdef DEBUG_ENABLED
#include <TimeLib.h>
#endif

float_t AverageCalculator::feed (float value) {
    unsigned long nowTime = millis ();
    unsigned int interval = (nowTime - lastMillis)/1000; // Valor en segundos
    lastMillis = nowTime;

    accumTime += interval;
    sum += (float)(value*(float)interval);
    if (accumTime != 0)
        average = sum / accumTime;
#ifdef DEBUG_ENABLED
    Serial.printf ("%02d:%02d:%02d\n", hour (), minute (), second ());
    Serial.printf ("%s --> Valor alimentado: %f\n", __FUNCTION__, value);
    Serial.printf ("%s --> Intervalo: %u\n", __FUNCTION__, interval);
    Serial.printf ("%s --> Tiempo acumulado: %u\n", __FUNCTION__, accumTime);
    Serial.printf ("%s --> Acumulador temperatura: %f\n", __FUNCTION__, sum);
    Serial.printf ("%s --> Media: %f\n", __FUNCTION__, average);
#endif

    return average;
}

float_t AverageCalculator::reset () {
    float lastAve = average;
    sum = 0;
    accumTime = 0;
    //lastMillis = millis ();
    Serial.printf ("%s ------> Media: %f\n", __FUNCTION__, average);

    return lastAve;
}

