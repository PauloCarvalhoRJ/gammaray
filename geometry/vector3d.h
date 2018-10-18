#ifndef VECTOR3D_H
#define VECTOR3D_H


class Vector3D {
public:
	double x, y, z;

	Vector3D operator-(Vector3D p) const;

	Vector3D operator+(Vector3D p) const;

	Vector3D cross(Vector3D p) const;

	double dot(Vector3D p) const;

	double norm() const;
};

Vector3D operator *( double scalar, const Vector3D& vector );

//make a point type with the same structure of the vector for clarity
typedef Vector3D Vertex3D;


#endif // VECTOR3D_H
