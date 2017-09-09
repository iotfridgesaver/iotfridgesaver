// 
// 
// 

#include "AverageCalculator.h"

float_t AverageCalculator::feed (float value) {
    unsigned long nowTime = millis ();
    unsigned int interval = (nowTime - lastMillis)/1000; // Valor en segundos
    lastMillis = nowTime;

    accumTime += interval;
    sum += value*interval;
    if (accumTime != 0)
        average = sum / accumTime;

    Serial.printf ("%s --> Intervalo: %u\n", __FUNCTION__, interval);
    Serial.printf ("%s --> Tiempo acumulado: %u\n", __FUNCTION__, accumTime);
    Serial.printf ("%s --> Acumulador temperatura: %u\n", __FUNCTION__, sum);
    Serial.printf ("%s --> Media: %u\n", __FUNCTION__, average);

    return average;
}

float_t AverageCalculator::reset () {
    float lastAve = average;
    sum = 0;
    accumTime = 0;
    //lastMillis = millis ();
    Serial.printf ("%s --> Media: %u\n", __FUNCTION__, average);

    return lastAve;
}

