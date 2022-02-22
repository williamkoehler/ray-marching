#version 460

in vec2 texCoord;

// uniform sampler2D image;

in vec2 screenCoord;

uniform float time;
uniform float aspectRatio;
uniform float fovFactor; // 1.0f / tan(fov / 2.0f)
uniform vec3 cameraPosition;
uniform mat3 cameraMatrix;

vec3 GetCameraRay()
{
	return normalize(cameraMatrix * vec3(screenCoord.x * aspectRatio, screenCoord.y, fovFactor));
}

struct Material
{
	float distance;
	vec3 color;
	float reflection;
	vec3 normal;
};

struct Hit
{
	Material material;
	vec3 position;
	vec3 direction;
};

float SDToSphere(vec3 position, vec3 center, float radius)
{
	return length(position - center) - radius;
}
Material SDToSphereMaterial(vec3 position, vec3 center, float radius, vec3 color, float reflection)
{
	return Material(
		length(position - center) - radius,
		color,
		reflection,
		vec3(0.0f, 0.0f, 0.0f)
	);
}
float SDToBox(vec3 position, vec3 center, vec3 extents)
{
	vec3 q = abs(position - center) - extents;
	return length(max(q,0.0)) + min(max(q.x,max(q.y,q.z)),0.0);
}
Material SDToBoxMaterial(vec3 position, vec3 center, vec3 extents, vec3 color, float reflection)
{
	vec3 q = abs(position - center) - extents;
	return Material(
		length(max(q,0.0)) + min(max(q.x,max(q.y,q.z)),0.0),
		color,
		reflection,
		vec3(0.0f, 0.0f, 0.0f)
	);
}
float SDToDonut(vec3 position, vec2 torus)
{
	vec2 q = vec2(length(position.xz)-torus.x,position.y);
	return length(q)-torus.y;
}
Material SDToDonutMaterial(vec3 position, vec2 torus, vec3 color, float reflection)
{
	vec2 q = vec2(length(position.xz)-torus.x,position.y);
	return Material(
		length(q)-torus.y,
		color,
		reflection,
		vec3(0.0f, 0.0f, 0.0f)
	);
}

float sdTorus( vec3 p, vec2 t )
{
  vec2 q = vec2(length(p.xz)-t.x,p.y);
  return length(q)-t.y;
}

float SDToUnion(float object1, float object2)
{
	return min(object1, object2);
}
Material SDToUnionMaterial(Material object1, Material object2)
{
	if (object1.distance < object2.distance)
		return object1;
	else
		return object2;
}
float SDToSmoothUnion(float object1, float object2, float k)
{
    float h = max( k-abs(object1-object2), 0.0 )/k;
    return min( object1, object2 ) - h*h*h*k*(1.0/6.0);
}
Material SDToSmoothUnionMaterial(Material object1, Material object2, float k)
{
	if (object1.distance < object2.distance)
		return object1;
	else
		return object2;
}

float SDToScene(vec3 position)
{
	float box = SDToDonut(position, vec2(1.0f, 0.5f));
	float plane = SDToBox(position, vec3(0.0f, -2.5f, 0.0f), vec3(10.0f, 0.5f, 10.0f));

	float blob = SDToSphere(position, vec3(sin(time / 2.0f) * 6.0f, sin(time / 2.0f), 0.0f), 0.5f);
	
	return SDToSmoothUnion(SDToUnion(box, plane), blob, 2.0f);
}
Material SDToSceneMaterial(vec3 position)
{
	Material box = SDToDonutMaterial(position, vec2(1.0f, 0.5f), vec3(0.8f, 0.6f, 0.6f), 0.9f);
	Material plane = SDToBoxMaterial(position,  vec3(0.0f, -2.5f, 0.0f), vec3(10.0f, 0.5f, 10.0f), vec3(0.8f, 0.8f, 0.8f), 0.9f);

	Material blob = SDToSphereMaterial(position, vec3(sin(time / 2.0f) * 6.0f, 0.0f, 0.0f), 0.5f, vec3(0.6f, 0.6f, 0.8f), 0.9f);
	
	return SDToUnionMaterial(SDToUnionMaterial(box, plane), blob);
}

vec3 GetNormal(vec3 position, float distance)
{
	return normalize((vec3(
		SDToScene(position + vec3(0.0001f, 0.0f, 0.0f)),
		SDToScene(position + vec3(0.0f, 0.0001f, 0.0f)),
		SDToScene(position + vec3(0.0f, 0.0f, 0.0001f))
	) - distance) / 0.0001f);
}

Hit CastRay(vec3 origin, vec3 direction)
{
	float depth = 0.0f;
	for(uint steps = 0; steps < 100; steps++)
	{
		vec3 position = origin + direction * depth;
		float distance = SDToScene(position);

		if(distance < 0.001f)
		{
			Material material = SDToSceneMaterial(position);
			material.normal = GetNormal(position, distance);
			return Hit(material, position, direction);
		}
		
		depth += distance;

		if(depth >= 200.0f)
		{
			break;
		}
	}

	return Hit(
		Material(
			0.0,
			vec3(0.9f, 0.9f, 0.9f),
			0.0f,
			vec3(0.0f, 0.0f, 0.0f)
		),
		vec3(0.0f, 0.0f, 0.0f),
		vec3(0.0f, 0.0f, 0.0f)
	);
}

vec3 CastCameraRay(vec3 origin, vec3 direction)
{
	vec3 color = vec3(0.9f, 0.9f, 0.9f);

	for(int i = 0; i < 6; i++)
	{
		Hit hit = CastRay(origin, direction);

		color *= hit.material.color;

		if(hit.material.reflection > 0.001f)
		{
			direction = reflect(hit.direction, hit.material.normal);
			origin = hit.position + direction * 0.001f;
		}
		else
			break;
	}

	return color;
}

void main()
{
	// gl_FragColor.xyz = texture(image, texCoord).xyz;
	vec3 direction = GetCameraRay();
	gl_FragColor.xyz = CastCameraRay(cameraPosition, direction);
	// gl_FragColor.xy = texCoord;
	// gl_FragColor.z = 1.0f;
	gl_FragColor.w = 1.0f;
}