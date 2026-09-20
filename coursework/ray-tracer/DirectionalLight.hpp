#pragma once
#include "Light.hpp"

class DirectionalLight : public Light
{
private:
	Eigen::Vector3f direction_, intensity_;
public:
	DirectionalLight(const Eigen::Vector3f& direction, const Eigen::Vector3f& intensity)
		:direction_(direction.normalized()), intensity_(intensity)
	{}


	virtual bool visibilityCheck(const Eigen::Vector3f& location, const Renderable* renderable) const override
	{
		Ray shadowRay;
		shadowRay.origin = location;
		shadowRay.direction = -direction_;
		HitInfo info;
		return !renderable->intersect(shadowRay, 1e-4f, 1e4f, info, SHADOW_BITMASK);
	}

	virtual Eigen::Vector3f getIntensity(const Eigen::Vector3f& location) const override
	{
		return intensity_;
	}

	virtual Eigen::Vector3f getVecToLight(const Eigen::Vector3f& location) const override
	{
		return -direction_;
	}

};
