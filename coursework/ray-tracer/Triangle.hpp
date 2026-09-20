#pragma once
#include "Renderable.hpp"
#include "GeomUtil.hpp"

/// <summary>
/// The Triangle consists of a single triangle.
/// UV coordinates are based on barycentric coordinates of the triangle.
/// </summary>
class Triangle : public Renderable
{
private:
	Eigen::Vector3f v0_, v1_, v2_;
	bool culling_;
public:
	Triangle(const Shader* shader, 
		const Eigen::Vector3f& v0, const Eigen::Vector3f& v1, const Eigen::Vector3f& v2, 
		bool culling=false, IntersectMask mask=DEFAULT_BITMASK)
		:Renderable(shader, mask), v0_(v0), v1_(v1), v2_(v2), culling_(culling)
	{}


	virtual bool intersect(const Ray& ray, float minT, float maxT, HitInfo& info, IntersectMask mask) const override
	{
		if (!checkMask(mask)) return false;
		// Intersection code from
		// https://www.scratchapixel.com/lessons/3d-basic-rendering/ray-tracing-rendering-a-triangle/moller-trumbore-ray-triangle-intersection.html
		Eigen::Vector3f v0World = transformPosition(modelToWorld(), v0_);
		Eigen::Vector3f v1World = transformPosition(modelToWorld(), v1_);
		Eigen::Vector3f v2World = transformPosition(modelToWorld(), v2_);

		Eigen::Vector3f v0v1 = v1World - v0World;
		Eigen::Vector3f v0v2 = v2World - v0World;
		Eigen::Vector3f pvec = ray.direction.cross(v0v2);
		float det = v0v1.dot(pvec);

		if (culling_) {
			// if the determinant is negative, the triangle is 'back facing'
			// if the determinant is close to 0, the ray misses the triangle
			if (det < 1e-6) return false;
		}
		else {
			// ray and triangle are parallel if det is close to 0
			if (fabs(det) < 1e-6) return false;
		}

		float invDet = 1 / det;

		Eigen::Vector3f tvec = ray.origin - v0World;
		float u = tvec.dot(pvec) * invDet;
		if (u < 0 || u > 1) return false;

		Eigen::Vector3f qvec = tvec.cross(v0v1);
		float v = ray.direction.dot(qvec) * invDet;
		if (v < 0 || u + v > 1) return false;

		float t = v0v2.dot(qvec) * invDet;

		if (t < minT || t > maxT) return false;

		info.hitT = t;
		info.inDirection = ray.direction;
		info.location = ray.origin + t * ray.direction;
		info.normal = v0v1.cross(v0v2).normalized();
		info.shader = shader();
		info.texCoords = Eigen::Vector2f(u, v);

		return true;
	}

	virtual AABB getAABB() const override
	{
		Eigen::Vector3f v0World = transformPosition(modelToWorld(), v0_);
		Eigen::Vector3f v1World = transformPosition(modelToWorld(), v1_);
		Eigen::Vector3f v2World = transformPosition(modelToWorld(), v2_);

		AABB aabb;
		aabb.min = v0World;
		aabb.max = v0World;

		for (int axis = 0; axis < 3; ++axis) {
			if (v1World[axis] < aabb.min[axis]) aabb.min[axis] = v1World[axis];
			if (v1World[axis] > aabb.max[axis]) aabb.max[axis] = v1World[axis];
			if (v2World[axis] < aabb.min[axis]) aabb.min[axis] = v2World[axis];
			if (v2World[axis] > aabb.max[axis]) aabb.max[axis] = v2World[axis];
		}

		return aabb;
	}

	virtual std::string print() const override
	{
		return "Triangle";
	}
};

