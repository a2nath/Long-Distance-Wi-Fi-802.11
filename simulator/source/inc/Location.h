#pragma once
#include <math.h>

class Location
{
private:
	double x, y, z;

public:
	Location() : x(NAN), y(NAN), z(NAN) {}

	Location(double x, double y, double z) : x(x), y(y), z(z) {}

	Location sub(Location loc)
	{
		return Location(this->x - loc.getX(), this->y - loc.getY(), this->z - loc.getZ());
	}

	double angle(Location loc)
	{
		return atan2f(this->y - loc.getY(), this->x - loc.getX());
	}

	double getX()
	{
		return this->x;
	}

	double getY()
	{
		return this->y;
	}

	double getZ()
	{
		return this->z;
	}

	double getDistance()
	{
		return sqrt(pow(getX(), 2) + pow(getY(), 2) + pow(getZ(), 2));
	}
};
