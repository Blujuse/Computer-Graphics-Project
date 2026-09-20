#pragma once
#include <Eigen/Dense>

/// <summary>
/// An Entity is any object in the world with a 6DoF transform
/// encoded as a 4x4 modelToWorld matrix.
/// </summary>
class Entity
{
private:
	Eigen::Matrix4f modelToWorld_;

public:
	Entity()
		:modelToWorld_(Eigen::Matrix4f::Identity())
	{}

	virtual ~Entity() noexcept
	{}

	Eigen::Matrix4f modelToWorld() const
	{
		return modelToWorld_;
	}

	Eigen::Matrix4f worldToModel() const
	{
		return modelToWorld_.inverse();
	}

	virtual void modelToWorld(const Eigen::Matrix4f& m)
	{
		modelToWorld_ = m;
	}
};

