#pragma once

#include <stdint.h>


/* Point2D */
class Point2D {
public:
	int32_t	X;
	int32_t	Y;

	Point2D();
	~Point2D();

	Point2D(int32_t _x, int32_t _y);


	void operator=(Point2D p) {
		X = p.X;
		Y = p.Y;
	}
};

/* Point2F */
class Point2F
{
public:
	float	X;
	float	Y;

	Point2F();
	~Point2F();

	Point2F(float _x, float _y);

	void operator=(Point2F p) {
		X = p.X;
		Y = p.Y;
	}
};

/* Extent2D */
class Extent2D {
public:
	Point2D		Start;
	Point2D		End;

	Extent2D();
	~Extent2D();

	Extent2D(Point2D _start, Point2D _end);
};