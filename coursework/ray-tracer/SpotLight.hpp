#pragma once
#include "Light.hpp"

#include "GeomUtil.hpp"

class Spotlight : public Light
{
private:
	Eigen::Vector3f _location;
	Eigen::Vector3f _direction;
	Eigen::Vector3f _intensity;
	float _cosAngle;

public:
	Spotlight(const Eigen::Vector3f& intensity, const Eigen::Vector3f& location, const Eigen::Vector3f& direction, float angle)
		:_intensity(intensity), _location(location), _direction(direction.normalized()), _cosAngle(cosf(angle))
	{}

	virtual bool visibilityCheck(const Eigen::Vector3f& location, const Renderable* renderable) const override
	{
		Ray shadowRay;
		shadowRay.origin = location;
		shadowRay.direction = (_location - location).normalized();
		float maxT = (_location - location).norm();
		HitInfo info;
		return !renderable->intersect(shadowRay, 1e-4f, maxT, info, SHADOW_BITMASK);
	}

	virtual Eigen::Vector3f getIntensity(const Eigen::Vector3f& location) const override
	{
		auto surfaceDir = (location - _location).normalized();

		if (surfaceDir.dot(_direction) < _cosAngle)
		{
			return Eigen::Vector3f::Zero();
		}

		float distance = (_location - location).norm();
		return _intensity / (distance * distance);
	}

	virtual Eigen::Vector3f getVecToLight(const Eigen::Vector3f& location) const override
	{
		return (_location - location).normalized();
	}
};