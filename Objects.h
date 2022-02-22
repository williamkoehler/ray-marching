#pragma once
#include "common.h"

struct Material
{
	glm::vec3 color = glm::vec3(0.0f, 0.0f, 0.0f);
};

struct Surface
{
	float distance;
	Material material;
};

struct Entity
{
	virtual Surface CalculateDistanceToSurface(glm::vec3 position) = 0;
};

struct Object : public Entity
{
	glm::vec3 center;
	Material material;

	Object(glm::vec3 center, Material material) :
		center(center),
		material(material)
	{}
};

struct Sphere : public Object
{
	float radius;

	Sphere(glm::vec3 center, float radius, Material material) : Object(center, material),
		radius(radius)
	{}

	virtual Surface CalculateDistanceToSurface(glm::vec3 position) override
	{
		Surface surface = {
			length(position - center) - radius,
			material,
		};

		return surface;
	}
};

struct Box : public Object
{
	glm::vec3 extents;

	Box(glm::vec3 center, glm::vec3 extents, Material material) : Object(center, material),
		extents(extents)
	{}

	virtual Surface CalculateDistanceToSurface(glm::vec3 position) override
	{
		glm::vec3 q = abs(position - center) - extents;
		Surface surface = {
			glm::length(glm::max(q, glm::vec3(0.0f, 0.0f, 0.0f))) + glm::min(glm::max(q.x, glm::max(q.y, q.z)), 0.0f),
			material,
		};

		return surface;
	}
};

struct Union : public Entity
{
	Entity* entity1, * entity2;

	Union(Entity* entity1, Entity* entity2) :
		entity1(entity1), entity2(entity2)
	{}

	virtual Surface CalculateDistanceToSurface(glm::vec3 position) override
	{
		Surface surface1 = entity1->CalculateDistanceToSurface(position);
		Surface surface2 = entity2->CalculateDistanceToSurface(position);

		if (surface1.distance < surface2.distance)
			return surface1;
		else
			return surface2;
	}
};
struct Union3 : public Entity
{
	Entity* entity1, * entity2, * entity3;

	Union3(Entity* entity1, Entity* entity2, Entity* entity3) :
		entity1(entity1), entity2(entity2), entity3(entity3)
	{}

	virtual Surface CalculateDistanceToSurface(glm::vec3 position) override
	{
		Surface surface1 = entity1->CalculateDistanceToSurface(position);
		Surface surface2 = entity2->CalculateDistanceToSurface(position);
		Surface surface3 = entity3->CalculateDistanceToSurface(position);

		if (surface1.distance < surface2.distance)
		{
			if (surface1.distance < surface3.distance)
				return surface1;
			else
				return surface3;
		}
		else
		{
			if (surface2.distance < surface3.distance)
				return surface2;
			else
				return surface3;
		}
	}
};
