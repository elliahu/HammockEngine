// Intersection function for sphere
vec2 raySphereDst(vec3 ro, vec3 rd, vec3 center, float radius) {
    vec3 oc = ro - center;
    float b = dot(oc, rd);
    float c = dot(oc, oc) - radius * radius;
    float disc = b * b - c;

    // No intersection: return zeros.
    if (disc < 0.0)
    return vec2(0.0, 0.0);

    float sqrtDisc = sqrt(disc);
    float t0 = -b - sqrtDisc;
    float t1 = -b + sqrtDisc;

    // If both intersections are behind the ray origin, treat as no intersection.
    if (t1 < 0.0)
    return vec2(0.0, 0.0);

    // If the ray starts inside the sphere, return 0 for distance to p1 and t1 as the chord.
    if (t0 < 0.0)
    return vec2(0.0, t1);

    // Otherwise, the ray starts outside the sphere.
    return vec2(t0, t1 - t0);
}

// Intersection function for troposphere (the layer of atmosphere where clouds appear)
// The troposphere is defined by two spheres and it is the leyer in between
vec2 rayTroposphereDst(vec3 ro, vec3 rd, vec3 center, float innerRadius, float outerRadius) {
    // Intersect the ray with the outer sphere (atmosphere top).
    vec2 outer = raySphereDst(ro, rd, center, outerRadius);
    // If there is no intersection with the outer sphere, the ray misses the atmosphere.
    if (outer.y == 0.0)
    return vec2(0.0, 0.0);

    // Intersect the ray with the inner sphere (planet surface).
    vec2 inner = raySphereDst(ro, rd, center, innerRadius);
    // If the ray does not hit the inner sphere, then the full outer chord lies in the atmosphere.
    if (inner.y == 0.0)
    return outer;

    // Compute the entry and exit distances for both spheres.
    float tOuter0 = outer.x;
    float tOuter1 = outer.x + outer.y;
    float tInner0 = inner.x;
    float tInner1 = inner.x + inner.y;

    // The atmospheric layer is the region between the outer sphere and the inner sphere.
    // There are two main cases:

    // Case 1: The ray enters the atmosphere and then hits the inner sphere.
    // (e.g. the ray starts outside and then crosses into the planet's atmosphere before hitting the surface)
    if (tOuter0 < tInner0) {
        // If the first inner hit is behind the ray origin, then the ray must be inside the inner sphere.
        if (tInner0 < 0.0)
        // Return the chord from when the ray exits the inner sphere until it leaves the outer sphere.
        return vec2(tInner1, tOuter1 - tInner1);
        else
        // Otherwise, the atmospheric segment is from the outer sphere entry until the inner sphere entry.
        return vec2(tOuter0, tInner0 - tOuter0);
    }
    // Case 2: The ray starts inside the inner sphere. (usual case)
    // Then the atmosphere is only encountered after the ray exits the inner sphere.
    else {
        return vec2(tInner1, tOuter1 - tInner1);
    }
}

// Returns x = distance from camera to the first intersection, y = distance from the first intersection to the second intersection
vec2 rayBoxDst(vec3 boundMin, vec3 boundMax, vec3 rayOrigin, vec3 rayDirection) {
    vec3 t0 = (boundMin - rayOrigin) / rayDirection;
    vec3 t1 = (boundMax - rayOrigin) / rayDirection;

    vec3 tmin = min(t0, t1);
    vec3 tmax = max(t0, t1);

    float dstA = max(max(tmin.x, tmin.y), tmin.z);
    float dstB = min(tmax.x, min(tmax.y, tmax.z));

    float dstToBox = max(0, dstA);
    float dstInsideBox = max(0, dstB - dstToBox);
    return vec2(dstToBox, dstInsideBox);
}

vec2 computeIntersectionLengths(vec3 bbmin, vec3 bbmax) {
    vec3 dims = bbmax - bbmin;
    float longest = length(dims);
    float shortest = min(dims.x, min(dims.y, dims.z));
    return vec2(shortest, longest);
}
