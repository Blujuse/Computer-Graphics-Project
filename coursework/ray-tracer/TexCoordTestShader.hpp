#pragma once
#include "Shader.hpp"

/// <summary>
/// Shader used for testing that colours objects according to their texture coordinates.
/// </summary>
class TexCoordTestShader : public Shader
{
public:
	virtual Eigen::Vector3f getColor(const HitInfo& hitInfo, 
		const Renderable* scene, 
		const std::vector<std::unique_ptr<Light>>& lights,
		const Eigen::Vector3f& ambientLight,
		int currBounceCount,
		const int maxBounces) const
	{
		Eigen::Vector3f color = Eigen::Vector3f(hitInfo.texCoords.x(), hitInfo.texCoords.y(), 0.f);
		return color;
	}
};

