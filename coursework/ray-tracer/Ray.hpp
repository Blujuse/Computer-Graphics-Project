#pragma once
#include <Eigen/Dense>

/// <summary>
/// Struct encoding a Ray, with an origin and a direction.
/// </summary>
struct Ray
{
	Eigen::Vector3f origin, direction;
};

std::ostream& operator <<(std::ostream& str, const Ray& ray)
{
	str << "Origin: " << ray.origin << "\n" << "Direction: " << ray.direction;
	return str;
}

