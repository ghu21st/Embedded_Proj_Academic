//Stucture 2D vector basic defination
#ifndef SVECTOR2D_H
#define SVECTOR2D_H
//
#include "utility.h"
#include "math.h"
//
struct SVector2D{
	double x;
	double y;

	SVector2D(double x_In=0.0f, double y_In=0.0f):x(x_In), y(y_In){}

	//
	SVector2D &operator+=(const SVector2D &vectIn){
		x+=vectIn.x;
		y+=vectIn.y;
		return *this;
	}

	SVector2D &operator-=(const SVector2D &vectIn){
		x-=vectIn.x;
		y-=vectIn.y;
		return *this;
	}
	
	SVector2D &operator*=(const double &dScale){
		x*=dScale;
		y*=dScale;
		return *this;
	}

	SVector2D &operator/=(const double &dScale){
		x/=dScale;
		y/=dScale;
		return *this;
	}
};

inline double Vector2DLength(const SVector2D &vectIn)
{
	return sqrt(vectIn.x*vectIn.x+vectIn.y*vectIn.y);
}

inline void Vector2DNorm(SVector2D &vectIn)
{
	double vLen=Vector2DLength(vectIn);
	vectIn.x=vectIn.x/vLen;
	vectIn.y=vectIn.y/vLen;
}

inline double Vector2DDot(const SVector2D &vectIn1, const SVector2D &vectIn2)
{
	return vectIn1.x*vectIn2.x+vectIn1.y*vectIn2.y;
}

#endif