#pragma once
#include "Shader.hpp"

/// <summary>
/// Shader for diffuse, Lambertian surfaces of a single colour.
/// </summary>
class LambertianShader : public Shader
{
private:
	Eigen::Vector3f albedo_;
	bool shadowTest_;
public:
	LambertianShader(const Eigen::Vector3f& albedo, bool shadowTest=true)
		:albedo_(albedo), shadowTest_(shadowTest)
	{}

	virtual Eigen::Vector3f getColor(const HitInfo& hitInfo, 
		const Renderable* scene, 
		const std::vector<std::unique_ptr<Light>>& lights,
		const Eigen::Vector3f& ambientLight,
		int currBounceCount,
		const int maxBounces) const
	{
		Eigen::Vector3f color = coefftWiseMul(albedo_, ambientLight);

		for (auto& light : lights) {
			if (shadowTest_) {
				if (!light->visibilityCheck(hitInfo.location, scene))
					continue;
			}
			Eigen::Vector3f lightVec = light->getVecToLight(hitInfo.location);
			float dotProd = std::max(lightVec.dot(hitInfo.normal), 0.f);
			color += dotProd * coefftWiseMul(light->getIntensity(hitInfo.location), albedo_);
		}

		return color;
	}
};

