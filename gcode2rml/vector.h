#pragma once

typedef struct
{
	double x, y, z;
}VECTOR;

VECTOR VGet(double x, double y, double z);

VECTOR VAdd(VECTOR vect1, VECTOR vect2);

VECTOR VSub(VECTOR vect1, VECTOR vect2);

double VDot(VECTOR vect1, VECTOR vect2);

double VSize(VECTOR vect);

VECTOR VScale(VECTOR vect, double scale);

VECTOR VCross(VECTOR vect1, VECTOR vect2);

VECTOR VNorm(VECTOR vect);

