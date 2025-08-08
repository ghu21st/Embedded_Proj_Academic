//utility functions for ANN_MPR
#ifndef UTILITY_H
#define UTILITY_H
//
#include "math.h"
#include "stdlib.h"
//
int Rand_Int(int iRange_Min, int iRange_Max); //integer rand, min<x<max
double Rand_Double();	//double rand, 0<x<1
double Rand_NN();	//double rand, -1<x<1
bool Rand_Bool();	//bool rand, 0 or 1
//
#endif
