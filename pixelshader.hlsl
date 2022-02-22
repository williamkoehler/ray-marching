struct PixelInputType
{
	float4 position : SV_POSITION;
	float2 screenPos : SCREEN;
};

cbuffer World
{
	float3 cameraPosition;
	float aspectRatio;
	float time;
	float3 emtpy;
	matrix cameraMatrix;
};

#define ALPHA 0.00001f
#define INFINITE 100.0f

float3 GetCameraRayDirection(float2 uv)
{
	float fovFactor = 1.0f / tan(3.14f / 7.0f); // 1 / tan(fov / 2) with fov = 90
	return normalize(mul(cameraMatrix, float3(uv.x * aspectRatio, uv.y, fovFactor)));
}

struct Surface
{
	float distanceToPoint;
	float3 color;
};

Surface SDSphereSurface(float3 position, float3 center, float radius, float3 color)
{
	Surface surface;
	surface.distanceToPoint = length(position - center) - radius;
	surface.color = color;

	return surface;
}
float SDSphere(float3 position, float3 center, float radius)
{
	return length(position - center) - radius;
}

Surface SDBoxSurface(float3 position, float3 center, float3 box, float3 color)
{
	Surface surface;
	float3 q = abs(position - center) - box;
	surface.distanceToPoint = length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0);
	surface.color = color;

	return surface;

}
float SDBox(float3 position, float3 center, float3 box)
{
	float3 q = abs(position - center) - box;
	return length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0);
}
Surface SDPlaneSurface(float3 position, float4 normal, float3 color)
{
	Surface surface;
	surface.distanceToPoint = dot(position - float3(0.0f, normal.w, 0.0f), normal.xyz);
	surface.color = color;

	return surface;
}
float SDPlane(float3 position, float4 normal)
{
	return dot(position - float3(0.0f, normal.w, 0.0f), normal.xyz);
}
float SDCappedCylinder(float3 position, float3 center, float height, float radius)
{
	position -= center;
	float2 d = abs(float2(length(position.xz), position.y)) - float2(radius, height);
	return min(max(d.x, d.y), 0.0) + length(max(d, 0.0));
}

Surface SDIntersectSurface(Surface surfaceA, Surface surfaceB)
{
	if (surfaceA.distanceToPoint > surfaceB.distanceToPoint)
		return surfaceA;
	return surfaceB;
}
float SDIntersect(float distA, float distB)
{
	return max(distA, distB);
}
Surface SDUnionSurface(Surface surfaceA, Surface surfaceB)
{
	if (surfaceA.distanceToPoint > surfaceB.distanceToPoint)
		return surfaceB;
	return surfaceA;
}
float SDUnion(float distA, float distB)
{
	return min(distA, distB);
}
Surface SDDifferenceSurface(Surface surfaceA, Surface surfaceB)
{
	if (surfaceA.distanceToPoint > -surfaceB.distanceToPoint)
		return surfaceA;
	return surfaceB;
}
float SDDifference(float distA, float distB)
{
	return max(distA, -distB);
}

Surface SDSmoothIntersectSurface(Surface surfaceA, Surface surfaceB, float k)
{
	Surface surface;

	float h = clamp(0.5 - 0.5 * (surfaceB.distanceToPoint - surfaceA.distanceToPoint) / k, 0.0, 1.0);
	surface.distanceToPoint = lerp(surfaceB.distanceToPoint, surfaceA.distanceToPoint, h) + k * h * (1.0 - h);
	surface.color = surfaceA.color;

	return surface;
}
float SDSmoothIntersect(float distA, float distB, float k)
{
	float h = clamp(0.5 - 0.5 * (distB - distA) / k, 0.0, 1.0);
	return lerp(distB, distA, h) + k * h * (1.0 - h);
}
Surface SDSmoothUnionSurface(Surface surfaceA, Surface surfaceB, float k)
{
	Surface surface;

	float h = max(k - abs(surfaceA.distanceToPoint - surfaceB.distanceToPoint), 0.0) / k;
	surface.distanceToPoint = min(surfaceA.distanceToPoint, surfaceB.distanceToPoint) - h * h * k * (1.0 / 4.0);
	//surface.color = lerp(surfaceA.color, surfaceB.color, (surfaceA.distanceToPoint - surfaceB.distanceToPoint) / max(surfaceA.distanceToPoint, surfaceB.distanceToPoint));
	if (surfaceA.distanceToPoint < surfaceB.distanceToPoint)
		surface.color = surfaceA.color;
	else
		surface.color = surfaceB.color;

	return surface;
}
float SDSmoothUnion(float distA, float distB, float k)
{
	float h = max(k - abs(distA - distB), 0.0) / k;
	return min(distA, distB) - h * h * k * (1.0 / 4.0);
}
Surface SDSmoothDifferenceSurface(Surface surfaceA, Surface surfaceB, float k)
{
	Surface surface;

	float h = clamp(0.5 - 0.5 * (surfaceB.distanceToPoint + surfaceA.distanceToPoint) / k, 0.0, 1.0);
	surface.distanceToPoint = lerp(surfaceB.distanceToPoint, -surfaceA.distanceToPoint, h) + k * h * (1.0 - h);
	surface.color = surfaceA.color;

	return surface;
}
float SDSmoothDifference(float distA, float distB, float k)
{
	float h = clamp(0.5 - 0.5 * (distB + distA) / k, 0.0, 1.0);
	return lerp(distB, -distA, h) + k * h * (1.0 - h);
}

Surface SDSceneSurface(float3 position)
{
	Surface objects = SDSmoothUnionSurface(
		SDSphereSurface(position,
			float3(
				cos(time / 1000.0f) * 3.0f * sin(time / 10000.0f),
				sin(time / 200.0f) * 0.2f + 1.0f,
				sin(time / 1000.0f) * 3.0f * sin(time / 10000.0f)
				),
			1.0f,
			float3(0.5f, 0.5f, 0.9f)),
		SDBoxSurface(position,
			float3(
				0.0f,
				0.0f,
				0.0f
				),
			float3(1.0f, 1.0f, 1.0f),
			float3(0.9f, 0.5f, 0.5f)),
		1.0f
	);

	return SDUnionSurface(
		SDPlaneSurface(position,
			float4(
				0.0f,
				1.0f,
				0.0f,
				-1.0f
				),
			float3(0.5f, 0.9f, 0.5f)),
		objects
	);
}
float SDScene(float3 position)
{
	float objects = SDSmoothUnion(
		SDSphere(position,
			float3(
				cos(time / 1000.0f) * 3.0f * sin(time / 10000.0f),
				sin(time / 200.0f) * 0.2f + 1.0f,
				sin(time / 1000.0f) * 3.0f * sin(time / 10000.0f)
				),
			1.0f),
		SDBox(position,
			float3(
				0.0f,
				0.0f,
				0.0f
				),
			float3(1.0f, 1.0f, 1.0f)),
		1.0f
	);

	return SDUnion(
		SDPlane(position,
			float4(
				0.0f,
				1.0f,
				0.0f,
				-1.0f
				)
		),
		objects
	);
}
float3 SceneSurfaceNormal(float3 position)
{
	return normalize(float3(
		SDScene(float3(position.x + ALPHA, position.y, position.z)) - SDScene(float3(position.x - ALPHA, position.y, position.z)),
		SDScene(float3(position.x, position.y + ALPHA, position.z)) - SDScene(float3(position.x, position.y - ALPHA, position.z)),
		SDScene(float3(position.x, position.y, position.z + ALPHA)) - SDScene(float3(position.x, position.y, position.z - ALPHA))
		));
}

float CastLightRay(float3 origin, float3 direction, float maxDepth)
{
	float shadow = 1.0f;
	float ph = 1000000000000.0f;

	for (float depth = 0.01f; depth < maxDepth;)
	{
		float distance = SDScene(origin + direction * depth);
		if (distance < 0.00001f)
		{
			return 0.0f;
		}
		float squareDistance = distance * distance;
		float y = squareDistance / (2.0 * ph);
		float d = sqrt(squareDistance - y * y);
		shadow = min(shadow, 0.4f * d / max(0.0, depth - y));
		ph = distance;

		depth += distance;
	}
	return shadow;
}

static float3 lightDirection = normalize(float3(1.0f, 6.0f, 2.0f));

float3 CastReflectionRay(float3 origin, float3 direction, float maxDepth)
{
	uint reflections = 0;

	float3 color = float3(1.0f, 1.0f, 1.0f);

	for (float depth = 0.01f; depth < maxDepth;)
	{
		float3 position = origin + direction * depth;
		Surface surface = SDSceneSurface(position);
		if (surface.distanceToPoint < 0.00001f)
		{
			color *= surface.color;
			reflections++;

			if (reflections > 2)
				return color /** CastLightRay(position, lightDirection, 50.0f)*/;

			float3 normal = SceneSurfaceNormal(position);

			depth = 0.01f;
			origin = position;
			direction = reflect(direction, normal);
		}

		depth += surface.distanceToPoint;
	}
	return color * float3(0.8f, 1.0f, 0.6f);
}

float3 CastRay(float3 origin, float3 direction)
{
	float3 color = float3(0.0f, 0.0f, 0.0f);

	float depth = 0;
	uint steps = 0;
	for (; steps < 100; steps++)
	{
		float3 position = origin + direction * depth;
		Surface surface = SDSceneSurface(position);

		if (surface.distanceToPoint < ALPHA)
		{
			float3 normal = SceneSurfaceNormal(position);

			float shadow = CastLightRay(position, lightDirection, 30.0f);
			//float shadow = 0.4f;

			float diffuse = clamp(dot(normal, lightDirection), 0.0f, 1.0f) * shadow /** CastReflectionRay(position, reflect(direction, normal), 10.0f)*/;

			float3  hal = normalize(lightDirection - direction);
			float specular = pow(clamp(dot(normal, hal), 0.0f, 1.0f), 16.0f) *
				diffuse *
				(0.04f + 0.96f * pow(clamp(1.0f + dot(hal, direction), 0.0f, 1.0f), 5.0f));

			color = lerp(color, surface.color * 4.0f * diffuse, 0.9f) /* * light color*/;
			color += 9.0f * specular /* * light color*/;

			color = surface.color;

			break;
		}

		depth += surface.distanceToPoint;

		if (depth > INFINITE)
			break;
	}

	color = lerp(color, float3(0.0f, 0.0f, 0.0f), 1.0f - (1.0f / (float)steps));

	return color;
}

float4 main(PixelInputType input) : SV_TARGET
{
	float4 color = float4(0.0f, 0.0f, 0.0f, 1.0f);

	color.xyz = CastRay(cameraPosition, GetCameraRayDirection(input.screenPos));

	color = pow(color, 0.7);

	return color;
}