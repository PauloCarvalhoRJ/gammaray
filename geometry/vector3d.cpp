#include "vector3d.h"
#include <cmath>

Vector3D Vector3D::operator-(Vector3D p) const {
	return Vector3D{x - p.x, y - p.y, z - p.z};
}

Vector3D Vector3D::operator+(Vector3D p) const
{
	return Vector3D{x + p.x, y + p.y, z + p.z};
}

Vector3D Vector3D::cross(Vector3D p) const {
	return Vector3D{
		y * p.z - p.y * z,
				z * p.x - p.z * x,
				x * p.y - p.x * y
	};
}

double Vector3D::dot(Vector3D p) const {
	return x * p.x + y * p.y + z * p.z;
}

double Vector3D::norm() const {
	return std::sqrt(x*x + y*y + z*z);
}


Vector3D operator *(double scalar, const Vector3D & vector)
{
	Vector3D result;
	result.x = scalar * vector.x;
	result.y = scalar * vector.y;
	result.z = scalar * vector.z;
	return result;
}
