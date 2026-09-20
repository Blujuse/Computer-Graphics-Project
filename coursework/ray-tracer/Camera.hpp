#pragma once
#include "Ray.hpp"

/// <summary>
/// Movable camera class. Provide the camera location, forward direction and an up
/// vector, along with the image dimensions and vertical Field of View angle (radians).
/// The camera can then produce a ray passing through each pixel location.
/// </summary>
class Camera
{
private:
	Eigen::Vector3f location_, bottomLeftPix_, right1pix_, up1pix_;

public:
	Camera(
		const Eigen::Vector3f& location,
		const Eigen::Vector3f& forward,
		const Eigen::Vector3f& up,
		int pixWidth, int pixHeight,
		float vertFov)
		:location_(location)
	{
		Eigen::Vector3f forwardVec = forward.normalized();
		Eigen::Vector3f rightVec = (up.cross(forwardVec)).normalized();
		Eigen::Vector3f upVec = (forward.cross(rightVec)).normalized();

		float aspect = static_cast<float>(pixWidth) / static_cast<float>(pixHeight);

		float halfHeight = tan(vertFov / 2);
		float halfWidth = aspect * halfHeight;

		bottomLeftPix_ = location + forwardVec - (halfWidth * rightVec + halfHeight * upVec);

		right1pix_ = rightVec * halfWidth * 2.f / static_cast<float>(pixWidth);
		up1pix_ = upVec * halfHeight * 2.f / static_cast<float>(pixHeight);
	}

	Ray getRay(int pixX, int pixY)
	{
		Ray ray;
		ray.origin = location_;
		Eigen::Vector3f pixelPos = bottomLeftPix_ +
			static_cast<float>(pixX) * right1pix_ +
			static_cast<float>(pixY) * up1pix_;

		ray.direction = (pixelPos - location_).normalized();
		return ray;
	}
};

