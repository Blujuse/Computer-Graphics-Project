#pragma once
#include "Renderable.hpp"
#include "GeomUtil.hpp"
#include <vector>
#include <limits>

/// <summary>
/// A Scene is a container for other Renderable objects.
/// Scenes can be nested if desired, and changing the ModelToWorld will
/// transform the sub-scenes.
/// Add objects to the scene by pushing them into the renderables vector.
/// </summary>
class Scene : public Renderable
{
public:
	Scene(IntersectMask mask=DEFAULT_BITMASK)
		:Renderable(nullptr, mask)
	{}


	std::vector<std::shared_ptr<Renderable>> renderables;

	virtual bool intersect(const Ray& ray, float minT, float maxT, HitInfo& info, IntersectMask mask) const
	{
		if (!checkMask(mask)) return false;

		// Transform ray from world space to scene space.
		Ray tRay;
		tRay.origin = transformPosition(worldToModel(), ray.origin);
		tRay.direction = transformDirection(worldToModel(), ray.direction);

		// Identify closest valid hit.
		float t = std::numeric_limits<float>::max();
		for (const auto& object : renderables) {
			HitInfo currInfo;
			if (object->intersect(tRay, minT, maxT, currInfo, mask)) {
				if (currInfo.hitT < t) {
					info = currInfo;
					t = currInfo.hitT;
				}
			}
		}

		// Transform hit location and normal back into world space.
		info.location = transformPosition(modelToWorld(), info.location);
		info.normal = transformDirection(modelToWorld(), info.normal);

		return t < std::numeric_limits<float>::max();
	}

	AABB getAABB() const override
	{
		return getRenderablesAABB(renderables);
	}

	virtual std::string print() const override
	{
		return "Scene";
	}

};

