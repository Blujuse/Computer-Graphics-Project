#pragma once
#include "Renderable.hpp"
#include "Light.hpp"
#include <vector>

/// <summary>
/// ADT for a Shader class that can be run on intersection with an associated
/// Renderable instance.
/// </summary>
class Shader
{
public:
	virtual ~Shader() throw()
	{}

	virtual Eigen::Vector3f getColor(const HitInfo& hitInfo, 
		const Renderable* scene, 
		const std::vector<std::unique_ptr<Light>>& lights,
		const Eigen::Vector3f& ambientLight,
		int currBounceCount,
		const int maxBounces) const = 0;
};

