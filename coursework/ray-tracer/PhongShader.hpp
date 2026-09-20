#pragma once
#include "Shader.hpp"

/// <summary>
/// Shader using the classic Phong reflectance model to add specular highlights.
/// </summary>
class PhongShader : public Shader
{
private:
	const std::vector<uint8_t>* albedoTexture_;
	int texWidth_, texHeight_;
	Eigen::Vector3f specular_;
	float shininess_;
	bool shadowTest_;
public:
	PhongShader(const std::vector<uint8_t>* albedoTexture, int texWidth, int texHeight, const Eigen::Vector3f& specular, float shininess, bool shadowTest=true)
		:albedoTexture_(albedoTexture), texWidth_(texWidth), texHeight_(texHeight), specular_(specular), shininess_(shininess), shadowTest_(shadowTest)
	{}

	virtual Eigen::Vector3f getColor(const HitInfo& hitInfo, 
		const Renderable* scene, 
		const std::vector<std::unique_ptr<Light>>& lights,
		const Eigen::Vector3f& ambientLight,
		int currBounceCount,
		const int maxBounces) const
	{
		Eigen::Vector2f tex = hitInfo.texCoords;
		int pixX = static_cast<int>(tex.x() * texWidth_);
		int pixY = static_cast<int>((1.f - tex.y()) * texHeight_);
		pixX = std::max(pixX, 0);
		pixY = std::max(pixY, 0);
		pixX = std::min(pixX, texWidth_ - 1);
		pixY = std::min(pixY, texHeight_ - 1);

		Eigen::Vector3f albedo;
		albedo.x() = static_cast<float>((*albedoTexture_)[(pixX + texWidth_ * pixY) * 4 + 0]) / 255.f;
		albedo.y() = static_cast<float>((*albedoTexture_)[(pixX + texWidth_ * pixY) * 4 + 1]) / 255.f;
		albedo.z() = static_cast<float>((*albedoTexture_)[(pixX + texWidth_ * pixY) * 4 + 2]) / 255.f;

		float metallic_ = 0.5f;

		Eigen::Vector3f color = coefftWiseMul(albedo * (1.f - metallic_), ambientLight);

		for (auto& light : lights)
		{
			if (shadowTest_ && !light->visibilityCheck(hitInfo.location, scene))
				continue;

			Eigen::Vector3f lightVec = light->getVecToLight(hitInfo.location);
			float diff = std::max(lightVec.dot(hitInfo.normal), 0.f);
			color += diff * coefftWiseMul(light->getIntensity(hitInfo.location), albedo * (1.f - metallic_));

			Eigen::Vector3f reflectVec = reflect(-lightVec, hitInfo.normal);
			float dotSpec = std::max(reflectVec.dot(-hitInfo.inDirection.normalized()), 0.f);
			dotSpec = std::pow(dotSpec, shininess_);
			color += dotSpec * coefftWiseMul(light->getIntensity(hitInfo.location),
				specular_ * (1.f - metallic_) + albedo * metallic_);
		}

		return color;
	}
};

