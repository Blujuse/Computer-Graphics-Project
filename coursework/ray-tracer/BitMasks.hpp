#pragma once

/// <summary>
/// IntersectMask is a bit mask which can be used to restrict which intersection
/// tests apply to which objects in the scene. For example, a shadow test may use
/// the SHADOW_BITMASK bit. Any Renderables which don't cast shadows should not have
/// this bit set, and then will not be include in the test.
/// </summary>
typedef unsigned int IntersectMask;

const IntersectMask VISIBLE_BITMASK = 0b1; // Setting this bit indicates a Renderable can be viewed directly (i.e. camera rays should hit it).
const IntersectMask SHADOW_BITMASK = 0b10; // Setting this bit indicates a Renderable should cast shadows (i.e. shadow rays should hit it).

const IntersectMask ALL_BITMASK = ~0b0; // Sets all bits of the mask.

const IntersectMask DEFAULT_BITMASK = VISIBLE_BITMASK | SHADOW_BITMASK; // By default, objects are visible and cast shadows.


