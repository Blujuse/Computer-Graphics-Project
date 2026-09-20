#pragma once
#include <Eigen/Dense>
#include "Ray.hpp"

struct AABB
{
	Eigen::Vector3f min, max;

	bool overlaps(const AABB& other) const
	{
		return (this->min.x() <= other.max.x() && this->max.x() >= other.min.x()) &&
			(this->min.y() <= other.max.y() && this->max.y() >= other.min.y()) &&
			(this->min.z() <= other.max.z() && this->max.z() >= other.min.z());
	}

	Eigen::Vector3f centre() const
	{
		return (min + max) * 0.5f;
	}

	bool intersect(const Ray& ray, float minT, float maxT) const
	{
		// Quick check for intersection with AABB.
		// Code from https://raytracing.github.io/books/RayTracingTheNextWeek.html
		float minTtmp = minT, maxTtmp = maxT;
		for (int a = 0; a < 3; a++) {
			auto invD = 1 / ray.direction[a];
			auto orig = ray.origin[a];

			auto t0 = (min[a] - orig) * invD;
			auto t1 = (max[a] - orig) * invD;

			if (invD < 0)
				std::swap(t0, t1);

			if (t0 > minTtmp) minTtmp = t0;
			if (t1 < maxTtmp) maxTtmp = t1;

			if (maxTtmp <= minTtmp)
				return false;
		}
		return true;
	}
};

