#pragma once
#include "Renderable.hpp"
#include "GeomUtil.hpp"
#include "Model.hpp"

/// <summary>
/// An Mesh is a regular triangle mesh. Intersections are found by testing all triangles in the
/// mesh. Optionally, a different index list to that from the Model instance can be provided
/// e.g. to render just some of the triangles in the mesh. This is used by the BVHNode class.
/// </summary>
class Mesh : public Renderable
{
private:
	AABB aabb_;
	std::vector<std::vector<VertexIndices>> indexList_;
protected:
	const Model* model_;
	bool culling_, checkAABB_;
public:
	Mesh(const Shader* shader, const Model* model,
		const std::vector<std::vector<VertexIndices>>* indexList = nullptr,
		bool culling = true, bool checkAABB = true, IntersectMask mask = DEFAULT_BITMASK)
		:Renderable(shader, mask), model_(model), culling_(culling), checkAABB_(checkAABB)
	{
		if (indexList) {
			indexList_ = std::vector<std::vector<VertexIndices>>(*indexList);
		}
		computeAABB();
	}

	int nfaces() const
	{
		if (indexList_.size() >= 1)
			return indexList_.size();
		else
			return model_->nfaces();

	}

	virtual bool intersect(const Ray& ray, float minT, float maxT, HitInfo& info, IntersectMask mask) const override
	{
		if (!checkMask(mask)) return false;

		if (checkAABB_ && !aabb_.intersect(ray, minT, maxT)) return false;

		float closestT = std::numeric_limits<float>::max();
		HitInfo hitInfo;


		for (int f = 0; f < nfaces(); ++f) {

			Eigen::Vector3f v0, v1, v2;
			if (indexList_.size() >= 1) {
				v0 = model_->vert(indexList_[f][0].vert);
				v1 = model_->vert(indexList_[f][1].vert);
				v2 = model_->vert(indexList_[f][2].vert);
			}
			else {
				v0 = model_->vert(model_->face(f)[0].vert);
				v1 = model_->vert(model_->face(f)[1].vert);
				v2 = model_->vert(model_->face(f)[2].vert);
			}

			// Intersection code from
			// https://www.scratchapixel.com/lessons/3d-basic-rendering/ray-tracing-rendering-a-triangle/moller-trumbore-ray-triangle-intersection.html
			Eigen::Vector3f v0World = transformPosition(Entity::modelToWorld(), v0);
			Eigen::Vector3f v1World = transformPosition(Entity::modelToWorld(), v1);
			Eigen::Vector3f v2World = transformPosition(Entity::modelToWorld(), v2);

			Eigen::Vector3f v0v1 = v1World - v0World;
			Eigen::Vector3f v0v2 = v2World - v0World;
			Eigen::Vector3f pvec = ray.direction.cross(v0v2);
			float det = v0v1.dot(pvec);

			if (culling_) {
				// if the determinant is negative, the triangle is 'back facing'
				// if the determinant is close to 0, the ray misses the triangle
				if (det < 1e-6) continue;
			}
			else {
				// ray and triangle are parallel if det is close to 0
				if (fabs(det) < 1e-6) continue;
			}

			float invDet = 1 / det;

			Eigen::Vector3f tvec = ray.origin - v0World;
			float u = tvec.dot(pvec) * invDet;
			if (u < 0 || u > 1) continue;

			Eigen::Vector3f qvec = tvec.cross(v0v1);
			float v = ray.direction.dot(qvec) * invDet;
			if (v < 0 || u + v > 1) continue;

			float t = v0v2.dot(qvec) * invDet;

			if (t >= closestT) continue;

			if (t < minT || t > maxT) continue;

			info.hitT = t;
			info.inDirection = ray.direction;
			info.location = ray.origin + t * ray.direction;
			info.shader = shader();

			if (model_->hasNormals()) {
				Eigen::Vector3f vn0, vn1, vn2;
				if (indexList_.size() >= 1) {
					vn0 = model_->normal(indexList_[f][0].norm);
					vn1 = model_->normal(indexList_[f][1].norm);
					vn2 = model_->normal(indexList_[f][2].norm);
				}
				else {
					vn0 = model_->normal(model_->face(f)[0].norm);
					vn1 = model_->normal(model_->face(f)[1].norm);
					vn2 = model_->normal(model_->face(f)[2].norm);
				}
				vn0 = transformNormal(Entity::modelToWorld(), vn0);
				vn1 = transformNormal(Entity::modelToWorld(), vn1);
				vn2 = transformNormal(Entity::modelToWorld(), vn2);
				info.normal = ((1 - (u + v)) * vn0 + u * vn1 + v * vn2).normalized();
			}
			else 
				info.normal = v0v1.cross(v0v2).normalized();

			Eigen::Vector2f vt0, vt1, vt2;
			if (indexList_.size() >= 1) {
				vt0 = model_->texCoord(indexList_[f][0].tex);
				vt1 = model_->texCoord(indexList_[f][1].tex);
				vt2 = model_->texCoord(indexList_[f][2].tex);
			}
			else {
				vt0 = model_->texCoord(model_->face(f)[0].tex);
				vt1 = model_->texCoord(model_->face(f)[1].tex);
				vt2 = model_->texCoord(model_->face(f)[2].tex);
			}
			info.texCoords = (1 - (u + v)) * vt0 + u * vt1 + v * vt2;

			closestT = t;
		}

		if (closestT == std::numeric_limits<float>::max()) {
			return false;
		}

		return true;
	}

	void computeAABB()
	{

		for (int i = 0; i < 3; ++i) {
			aabb_.min[i] = std::numeric_limits<float>::max();
			aabb_.max[i] = std::numeric_limits<float>::min();
		}
		for (int f = 0; f < nfaces(); ++f) {
			for (int v = 0; v < 3; ++v) {

				Eigen::Vector3f v0;
				if (indexList_.size() > 0) 
					v0 = model_->vert(indexList_[f][v].vert);
				else
					v0 = model_->vert(model_->face(f)[v].vert);

				v0 = transformPosition(Entity::modelToWorld(), v0);
				for (int i = 0; i < 3; ++i) {
					if (v0[i] < aabb_.min[i]) aabb_.min[i] = v0[i];
					if (v0[i] > aabb_.max[i]) aabb_.max[i] = v0[i];
				}
			}
		}
	}

	virtual void modelToWorld(const Eigen::Matrix4f& m) override
	{
		Entity::modelToWorld(m);

		// When changing modelToWorld, also update the world-space AABB.
		computeAABB();
	}

	virtual AABB getAABB() const override
	{
		return aabb_;
	}
	virtual std::string print() const override
	{
		return "Mesh";
	}
};