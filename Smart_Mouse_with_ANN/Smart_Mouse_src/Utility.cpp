//
#include "utility.h"
//
//integer rand()
int Rand_Int(int iRange_Min, int iRange_Max) //min<x<max
{
	return (int)(rand()%(iRange_Max-iRange_Min+1)+iRange_Min);
}

double Rand_Double()	//0.0<x<1.0
{
	return (double)(rand())/(RAND_MAX+1.0);
}

double Rand_NN()	//-1.0<x<1.0
{
	return (double)(Rand_Double()-Rand_Double());
}

bool Rand_Bool()	//0 or 1
{
	if(Rand_Int(0,1)==0){
		return false;
	}else{
		return true;
	}
}