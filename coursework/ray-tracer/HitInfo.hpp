#pragma once
#include <Eigen/Dense>

class Shader;

/// <summary>
/// Structure encoding information from an intersection test.
/// </summary>
struct HitInfo
{
	float hitT; // Distance along the ray the hit occurred at.
	Eigen::Vector3f 
		normal, // Normal vector of the object at the hit location.
		location, // World-space location of hit point.
		inDirection; // Incoming ray direction.
	Eigen::Vector2f texCoords; // Texture coordinates at the hit location.
	const Shader* shader; // Shader associated with the hit object.
};
