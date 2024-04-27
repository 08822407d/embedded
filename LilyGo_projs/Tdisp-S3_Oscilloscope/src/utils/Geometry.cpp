#include "Geometry.hpp"


/** Point2D **/
Point2D::Point2D() {
	X = 0;
	Y = 0;
}
Point2D::~Point2D() {}

Point2D::Point2D(int32_t _x, int32_t _y) {
	X = _x;
	Y = _y;
}


/** Point2F **/
Point2F::Point2F() {
	X = 0.0;
	Y = 0.0;
}
Point2F::~Point2F() {}

Point2F::Point2F(float _x, float _y) {
	X = _x;
	Y = _y;
}


/** Extent2D **/
Extent2D::Extent2D() {
	Start = Point2D();
	End = Point2D();
}
Extent2D::~Extent2D() {}

Extent2D::Extent2D(Point2D _start, Point2D _end) {
	Start = _start;
	End = _end;
}