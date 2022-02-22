#include "RayMarcher.h"

RayMarcher::RayMarcher(glm::uvec2 size, float fov)
	: size(size), aspectRatio((float)size.x / (float)size.y),
	fovFactor(1.0f / tan(fov)),
	cameraPosition(glm::vec3(-6.0f, 3.0f, -6.0f)),
	pixels(new glm::vec3[size.x * size.y])
{
	memset(pixels, 0.0f, sizeof(glm::vec3) * size.x * size.y);

	glm::mat4 matrix(1.0f);
	matrix = glm::rotate(matrix, 0.7f, glm::vec3(0.0f, 1.0f, 0.0f));
	matrix = glm::rotate(matrix, 0.48f, glm::vec3(1.0f, 0.0f, 0.0f));

	cameraRotation = glm::mat3(matrix);

	//Create scene
	scene = new Union(
		new Union(
			new Sphere(glm::vec3(0.0f, 0.0f, -12.0f), 7.0f, { glm::vec3(0.9f, 0.999f, 0.999f) }),
			new Sphere(glm::vec3(-1.5f, 0.0f, 0.0f), 1.0f, { glm::vec3(0.999f, 0.9f, 0.9f) })
		),
		new Union3(
			new Box(glm::vec3(20.0f, 0.0f, 0.0f), glm::vec3(0.001f, 5.0f, 5.0f), { glm::vec3(0.999f, 0.9f, 0.9f) }),
			new Box(glm::vec3(0.0f, 0.0f, 20.0f), glm::vec3(5.0f, 5.0f, 0.001f), { glm::vec3(0.9f, 0.9f, 0.999f) }),
			new Union3(
				new Box(glm::vec3(-20.0f, 0.0f, 0.0f), glm::vec3(0.001f, 5.0f, 5.0f), { glm::vec3(0.999f, 0.999f, 0.9f) }),
				new Box(glm::vec3(0.0f, 0.0f, -20.0f), glm::vec3(5.0f, 5.0f, 0.001f), { glm::vec3(0.9f, 0.999f, 0.999f) }),
				new Union(
					new Box(glm::vec3(0.0f, 20.0f, 0.0f), glm::vec3(5.0f, 0.001f, 5.0f), { glm::vec3(0.999f, 0.999f, 0.9f) }),
					new Box(glm::vec3(0.0f, -20.0f, 0.0f), glm::vec3(5.0f, 0.001f, 5.0f), { glm::vec3(0.9f, 0.999f, 0.999f) })
				)
			)
		)
	);
}

Ray RayMarcher::GetCameraRay(glm::uvec2 coord)
{
	glm::vec2 uv = (glm::vec2(coord) / glm::vec2(size)) * 2.0f - 1.0f;

	Ray ray = {
		cameraPosition,
		glm::normalize(cameraRotation * glm::vec3(uv * glm::vec2(aspectRatio, 1.0f), fovFactor))
	};

	return ray;
}

glm::vec3 RayMarcher::GetNormal(glm::vec3 position, float distance)
{
	return glm::normalize((glm::vec3(
		scene->CalculateDistanceToSurface(position + glm::vec3(0.0001f, 0.0f, 0.0f)).distance,
		scene->CalculateDistanceToSurface(position + glm::vec3(0.0f, 0.0001f, 0.0f)).distance,
		scene->CalculateDistanceToSurface(position + glm::vec3(0.0f, 0.0f, 0.0001f)).distance
	) - distance) / 0.0001f);
}

glm::vec3 RayMarcher::CastRay(glm::vec3 origin, glm::vec3 direction, float depth, uint32_t reflections)
{
	for (; depth < 100.0f;)
	{
		glm::vec3 position = origin + direction * depth;
		Surface surface = scene->CalculateDistanceToSurface(position);

		if (surface.distance < 0.0001f)
		{
			if (reflections < 5)
			{
				glm::vec3 normal = GetNormal(position, surface.distance);
				direction = glm::reflect(direction, normal);

				return surface.material.color * CastRay(position, direction, 0.01f, reflections + 1);
			}
			else
				return surface.material.color;
		}

		depth += surface.distance;
	}

	return glm::vec3(0.99f, 0.99f, 0.99f);
}

void RayMarcher::RenderBatch(glm::uvec2 topLeft, glm::uvec2 bottomRight)
{
	glm::uvec2 coord;
	for (coord.x = topLeft.x; coord.x < bottomRight.x; coord.x++)
	{
		for (coord.y = topLeft.y; coord.y < bottomRight.y; coord.y++)
		{
			Ray ray = GetCameraRay(coord);

			glm::vec3* pixel = &pixels[coord.y * size.x + coord.x];
			*pixel = CastRay(ray.origin, ray.direction);
		}
	}
}

glm::vec3* RayMarcher::Render(uint32_t batchSize)
{
	glm::uvec2 batchCount = glm::uvec2(glm::ceil((float)size.x / (float)batchSize), glm::ceil((float)size.y / (float)batchSize));

	glm::uvec2 coord = glm::uvec2(0, 0);
	glm::uvec2 coord2 = glm::uvec2(0, 0);

	for (uint32_t x = 0; x < batchCount.x; x++)
	{
		coord.y = 0;
		coord2.x = glm::min(coord.x + batchSize, size.x);

		for (uint32_t y = 0; y < batchCount.y; y++)
		{
			coord2.y = glm::min(coord.y + batchSize, size.y);

			std::async(std::launch::async, std::bind(&RayMarcher::RenderBatch, this, coord, coord2));

			coord.y = coord2.y;
		}

		coord.x = coord2.x;
	}

	mutex.lock();
	mutex.unlock();

	return pixels;
}

std::future<void> RayMarcher::AsyncRender(std::function<void(glm::vec3*, glm::uvec2)> update, uint32_t batchSize)
{
	return std::async(std::launch::async,
		[this, batchSize, update]() {
			glm::uvec2 batchCount = glm::uvec2(glm::ceil((float)size.x / (float)batchSize), glm::ceil((float)size.y / (float)batchSize));

			glm::uvec2 coord = glm::uvec2(0, 0);
			glm::uvec2 coord2 = glm::uvec2(0, 0);

			for (uint32_t x = 0; x < batchCount.x; x++)
			{
				coord.y = 0;
				coord2.x = glm::min(coord.x + batchSize, size.x);

				for (uint32_t y = 0; y < batchCount.y; y++)
				{
					coord2.y = glm::min(coord.y + batchSize, size.y);

					printf("%i %i\n", x, y);

					RayMarcher::RenderBatch(coord, coord2);

					update(pixels, size);

					coord.y = coord2.y;
				}

				coord.x = coord2.x;
			}
		}
	);
}

void RayMarcher::Wait()
{
	mutex.lock();
	mutex.unlock();
}