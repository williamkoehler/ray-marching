#pragma once
#include "common.h"
#include "Objects.h"

struct Ray
{
	glm::vec3 origin;
	glm::vec3 direction;
};

class RayMarcher
{
private:
	glm::uvec2 size;
	float aspectRatio;
	float fovFactor;

	glm::vec3 cameraPosition;
	glm::mat3 cameraRotation;

	glm::vec3* pixels;

	Entity* scene;

	std::shared_mutex mutex;

	Ray GetCameraRay(glm::uvec2 coord);

	glm::vec3 GetNormal(glm::vec3 position, float distance);

	glm::vec3 CastRay(glm::vec3 origin, glm::vec3 direction, float depth = 0.0f, uint32_t reflections = 0);

	void RenderBatch(glm::uvec2 topLeft, glm::uvec2 bottomRight);

public:
	RayMarcher(glm::uvec2 size, float fov);

	glm::vec3* Render(uint32_t batchSize = 32);
	std::future<void> AsyncRender(std::function<void(glm::vec3*, glm::uvec2)> update, uint32_t batchSize = 32);

	void Wait();
};

