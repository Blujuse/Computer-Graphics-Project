#pragma once
# define M_PI           3.14159265358979323846
#include <Eigen/Dense>
#include "AABB.hpp"
#include "Ray.hpp"
#include "Renderable.hpp"

// Note all matrices in these functions are designed for left multiplication
// I.e. M*x not x*M.

/// <summary>
/// Make a matrix that translates a position vector.
/// </summary>
Eigen::Matrix4f makeTranslationMatrix(const Eigen::Vector3f& t)
{
	Eigen::Matrix4f transMat = Eigen::Matrix4f::Identity();
	transMat.block<3, 1>(0, 3) = t;
	return transMat;
}

/// <summary>
/// Make a matrix that rotates by theta radians about the x axis.
/// </summary>
Eigen::Matrix4f rotateX(float theta)
{
	// Convert to radians
	theta *= M_PI / 180.0f;

	Eigen::Matrix4f rotMat;
	rotMat <<
		1.f, 0.f, 0.f, 0.f,
		0.f, cosf(theta), -sinf(theta), 0.f,
		0.f, sinf(theta), cosf(theta), 0.f,
		0.f, 0.f, 0.f, 1.f;
	return rotMat;
}

/// <summary>
/// Make a matrix that rotates by theta radians about the y axis.
/// </summary>
Eigen::Matrix4f rotateY(float theta)
{
	// Convert to radians
	theta *= M_PI / 180.0f;

	Eigen::Matrix4f rotMat;
	rotMat <<
		cosf(theta), 0.f, sinf(theta), 0.f,
		0.f, 1.f, 0.f, 0.f,
		-sinf(theta), 0.f, cosf(theta), 0.f,
		0.f, 0.f, 0.f, 1.f;
	return rotMat;
}

/// <summary>
/// Make a matrix that rotates by theta radians about the z axis.
/// </summary>
Eigen::Matrix4f rotateZ(float theta)
{
	// Convert to radians
	theta *= M_PI / 180.0f;

	Eigen::Matrix4f rotMat;
	rotMat <<
		cosf(theta), -sinf(theta), 0.f, 0.f,
		sinf(theta), cosf(theta), 0.f, 0.f,
		0.f, 0.f, 1.f, 0.f,
		0.f, 0.f, 0.f, 1.f;
	return rotMat;
}

/// <summary>
/// Make a matrix that scales uniformly in x, y and z.
/// </summary>
Eigen::Matrix4f uniformScale(float scale)
{
	Eigen::Matrix4f scaleMat;
	scaleMat <<
		scale, 0.f, 0.f, 0.f,
		0.f, scale, 0.f, 0.f,
		0.f, 0.f, scale, 0.f,
		0.f, 0.f, 0.f, 1.f;
	return scaleMat;
}

/// <summary>
/// Apply a transform to a position. This multiplies by the matrix, setting w to 1.
/// This will apply the translation component of the matrix.
/// </summary>
Eigen::Vector3f transformPosition(const Eigen::Matrix4f& transform, const Eigen::Vector3f& position)
{
	Eigen::Vector4f transformed = transform * Eigen::Vector4f(position.x(), position.y(), position.z(), 1.f);
	return transformed.block<3, 1>(0, 0) / transformed.w();
}

/// <summary>
/// Apply a transform to a direction. This multiplies by the matrix, setting w to 0.
/// This will *not* apply the translation component of the matrix.
/// </summary>
Eigen::Vector3f transformDirection(const Eigen::Matrix4f& transform, const Eigen::Vector3f& direction)
{
	Eigen::Vector4f transformed = transform * Eigen::Vector4f(direction.x(), direction.y(), direction.z(), 0.f);
	return transformed.block<3, 1>(0, 0);
}

/// <summary>
/// Apply a transform to a normal vector. This multiplies by the inverse transpose of the 
/// 3x3 upper left corner of the matrix.
/// </summary>
Eigen::Vector3f transformNormal(const Eigen::Matrix4f& transform, const Eigen::Vector3f& normal)
{
	Eigen::Matrix3f normMat = transform.block<3, 3>(0, 0).inverse();
	return normMat * normal;
}

/// <summary>
/// Reflect a vector in a specified normal vector. BOTH VECTORS MUST BE NORMALISED.
/// </summary>
Eigen::Vector3f reflect(const Eigen::Vector3f& inDir, const Eigen::Vector3f& normal)
{
	return inDir - 2.f * (inDir.dot(normal)) * normal;
}

/// <summary>
/// Refract a vector in a specified normal vector. BOTH VECTORS MUST BE NORMALISED.
/// Assumes a transition between air of IOR 1 and a material with the specified IOR.
/// </summary>
Eigen::Vector3f refract(const Eigen::Vector3f& inDir, const Eigen::Vector3f& normal, float indexOfRefraction)
{
	// First need to work out if we're going into or coming out of material (based on normal)
	Eigen::Vector3f corrNorm;
	float iorRatio;
	if (inDir.dot(normal) < 0.f) {
		// Going into material
		corrNorm = normal;
		iorRatio = 1.f / indexOfRefraction;
	}
	else {
		// Exiting material
		corrNorm = -normal;
		iorRatio = indexOfRefraction;
	}
	float normalDotInDir = corrNorm.dot(inDir);
	float k = 1.0 - iorRatio * iorRatio * (1.0 - normalDotInDir * normalDotInDir);
	if (k < 0.0)
		return reflect(inDir, corrNorm); // Total internal reflection
	else
		return iorRatio * inDir - (iorRatio * normalDotInDir + sqrt(k)) * corrNorm;
}

/// <summary>
/// Multiply two 3D vectors coefficient-wise, producing a 3D vector as output.
/// Note this is NOT the cross or dot product of the vectors.
/// </summary>
Eigen::Vector3f coefftWiseMul(const Eigen::Vector3f& left, const Eigen::Vector3f& right)
{
	return (left.array() * right.array()).matrix();
}

/// <summary>
/// Given a list of renderables, finds an AABB surrounding them all.
/// </summary>
AABB getRenderablesAABB(const std::vector<std::shared_ptr<Renderable>>& renderables)
{
	AABB aabb;
	// **** YOUR CODE HERE ****
	// Iterate through the list of renderables, and find an AABB that surrounds all of them.
	// Make use of the getAABB() method that all Renderable instances have.

	// PARTIALLY TAKEN FROM GITHUB COPILOT
	aabb.min = Eigen::Vector3f::Constant(std::numeric_limits<float>::max());
	aabb.max = Eigen::Vector3f::Constant(std::numeric_limits<float>::lowest());

	for (const auto& renderable : renderables) 
	{
		AABB renderableAABB = renderable->getAABB();
#
		for (int i = 0; i < 3; ++i)
		{
			if (renderableAABB.min[i] < aabb.min[i]) aabb.min[i] = renderableAABB.min[i];
			if (renderableAABB.max[i] > aabb.max[i]) aabb.max[i] = renderableAABB.max[i];
		}
	}

	return aabb;
}

