
#include "vector.h"
#include <math.h>

VECTOR VGet(double x, double y, double z)
{
	VECTOR _vect = { x, y, z };
	return _vect;
}

VECTOR VAdd(VECTOR vect1, VECTOR vect2)
{
	VECTOR _vect;
	_vect.x = vect1.x + vect2.x;
	_vect.y = vect1.y + vect2.y;
	_vect.z = vect1.z + vect2.z;
	return _vect;
}

VECTOR VSub(VECTOR vect1, VECTOR vect2)
{
	VECTOR _vect;
	_vect.x = vect1.x - vect2.x;
	_vect.y = vect1.y - vect2.y;
	_vect.z = vect1.z - vect2.z;
	return _vect;
}

double VDot(VECTOR vect1, VECTOR vect2)
{
	return vect1.x * vect2.x + vect1.y * vect2.y + vect1.z + vect2.z;
}

double VSize(VECTOR vect)
{
	return sqrt(vect.x * vect.x + vect.y * vect.y + vect.z * vect.z);
}

VECTOR VScale(VECTOR vect, double scale)
{
	VECTOR _vect;
	_vect.x = vect.x * scale;
	_vect.y = vect.y * scale;
	_vect.z = vect.z * scale;
	return _vect;
}

VECTOR VCross(VECTOR vect1, VECTOR vect2)
{
	VECTOR _vect;
	_vect.x = vect1.y * vect2.z - vect1.z * vect2.y;
	_vect.y = vect1.z * vect2.x - vect1.x * vect2.z;
	_vect.z = vect1.x * vect2.y - vect1.y * vect2.x;
	return _vect;
}

VECTOR VNorm(VECTOR vect)
{
	VECTOR _vect;
	double size = VSize(vect);
	_vect.x = vect.x / size;
	_vect.y = vect.y / size;
	_vect.z = vect.z / size;
	return _vect;
}

