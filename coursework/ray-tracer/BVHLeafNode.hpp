#pragma once
#include "Renderable.hpp"
#include "GeomUtil.hpp"
#include "Mesh.hpp"
#include "BVHNode.hpp"
#include <vector>

class BVHLeafNode : public Renderable
{
	AABB aabb_;
	std::vector<std::shared_ptr<Renderable>> renderables_;
public:
	BVHLeafNode(const std::vector<std::shared_ptr<Renderable>>& renderables)
		:Renderable(nullptr), renderables_(renderables)
	{
		aabb_ = getRenderablesAABB(renderables);
	}

	virtual AABB getAABB() const override
	{
		return aabb_;
	}

	virtual bool intersect(const Ray& ray, float minT, float maxT, HitInfo& info, IntersectMask mask) const override
	{
		Ray tRay;
		tRay.origin = transformPosition(worldToModel(), ray.origin);
		tRay.direction = transformDirection(worldToModel(), ray.direction);

		// If we don't hit the AABB associated with this node at all, exit early!
		if (!aabb_.intersect(tRay, minT, maxT)) return false;

		// We did hit the AABB, so now we test the children.
		bool hitSomething = false; // Keep track of whether a hit occurred.

		// Try intersecting all renderables.
		float t = std::numeric_limits<float>::max();
		for (const auto& object : renderables_) {
			HitInfo currInfo;
			if (object->intersect(tRay, minT, maxT, currInfo, mask)) {
				if (currInfo.hitT < t) {
					info = currInfo;
					t = currInfo.hitT;
					hitSomething = true;
				}
			}
		}

		return hitSomething;
	}

	virtual std::string print() const override
	{
		std::stringstream ss;
		for (auto& renderable : renderables_) {
			ss << renderable->print() << "\n";
		}
		return ss.str();
	}
};

