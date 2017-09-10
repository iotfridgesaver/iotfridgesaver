// AverageCalculator.h

#ifndef _AVERAGECALCULATOR_h
#define _AVERAGECALCULATOR_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "Arduino.h"
#else
	#include "WProgram.h"
#endif

class AverageCalculator
{
 protected:
     double sum;
     time_t accumTime;
     float average;
     unsigned long lastMillis;
 public:
    float_t feed (float value);
    float getAverage () { return average; }
    float_t reset ();
};

#endif

