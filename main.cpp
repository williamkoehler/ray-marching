#include "common.h"
#include <chrono>
#include "RayMarcher.h"

#define IMAGE_SIZE_X 240
//#define IMAGE_SIZE_X 1920
#define IMAGE_SIZE_Y 135
//#define IMAGE_SIZE_Y 1080

uint32_t CompileShaderFromFile(uint32_t shaderType, std::string filename)
{
	std::ifstream file(filename);
	if (!file.is_open())
	{
		printf("Failed to open file : %s", filename.c_str());
		return 0;
	}

	std::string source((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

	if (source.empty())
	{
		printf("Failed to read file (emtpy file) : %s\n", filename.c_str());
		return 0;
	}

	uint32_t shader = glCreateShader(shaderType);
	char* sourcec = (char*)source.c_str();
	glShaderSource(shader, 1, &sourcec, nullptr);
	glCompileShader(shader);

	int32_t status;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if (!status)
	{
		int32_t infoLogLength;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);
		char* infoLog = new char[infoLogLength + 1];
		glGetShaderInfoLog(shader, infoLogLength, nullptr, infoLog);
		printf("Failed to compile shader : %s : %s\n", filename.c_str(), infoLog);
		delete[] infoLog;

		glDeleteShader(shader);
		return 0;
	}

	return shader;
}

uint32_t CreateProgram(uint32_t vertexShader, uint32_t fragmentShader)
{
	uint32_t program = glCreateProgram();

	if (vertexShader)
		glAttachShader(program, vertexShader);
	if (fragmentShader)
		glAttachShader(program, fragmentShader);
	//if (geometryShader != 0) glAttachShader(program, geometryShader);

	glLinkProgram(program);

	int32_t status;
	glGetProgramiv(program, GL_LINK_STATUS, &status);
	if (status == GL_FALSE)
	{
		int32_t infoLogLength;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);

		char* infoLog = new GLchar[infoLogLength + 1];
		glGetProgramInfoLog(program, infoLogLength, nullptr, infoLog);
		printf("Failed to link program : %s\n", infoLog);
		delete[] infoLog;
	}

	if (vertexShader != 0)
		glDetachShader(program, vertexShader);
	if (fragmentShader != 0)
		glDetachShader(program, fragmentShader);
	//if (geometryShader != 0) glDetachShader(program, geometryShader);

	return program;
}

struct FragmetShaderUniforms
{
	uint32_t time;
	uint32_t aspectRatio;
	uint32_t fovFactor;
	uint32_t cameraPosition;
	uint32_t cameraMatrix;
} fragShaderUniforms;

std::mutex mutex;
std::atomic_bool changed = false;
glm::vec3* pixels2;

void Update(glm::vec3* pixels, glm::uvec2 size)
{
	mutex.lock();

	memcpy(pixels2, pixels, size.x * size.y);
	changed = true;
	mutex.unlock();
}

int main()
{
	GLFWwindow* window;

	if (!glfwInit())
		return -1;

	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
	glfwWindowHint(GLFW_DOUBLEBUFFER, GL_TRUE);

	const int32_t windowWidth = 1920, windowHeight = 1080;
	const float aspectRatio = (float)windowWidth / (float)windowHeight;
	//window = glfwCreateWindow(windowWidth, windowHeight, "Ray Tracing", glfwGetPrimaryMonitor(), nullptr);
	window = glfwCreateWindow(windowWidth, windowHeight, "Ray Tracing", nullptr, nullptr);
	if (!window)
	{
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);

	if (glewInit() != GLEW_OK)
		return -1;

	//Create shader
	uint32_t program = CreateProgram(
		CompileShaderFromFile(GL_VERTEX_SHADER, "vs.glsl"),
		CompileShaderFromFile(GL_FRAGMENT_SHADER, "fs.glsl")
	);
	if (!program)
		return -1;

	fragShaderUniforms.time = glGetUniformLocation(program, "time");
	fragShaderUniforms.aspectRatio = glGetUniformLocation(program, "aspectRatio");
	fragShaderUniforms.fovFactor = glGetUniformLocation(program, "fovFactor");
	const float fovFactor = 1.0f / tan(3.1415f / 7.0f);

	fragShaderUniforms.cameraPosition = glGetUniformLocation(program, "cameraPosition");
	fragShaderUniforms.cameraMatrix = glGetUniformLocation(program, "cameraMatrix");

	//Create VBO
	uint32_t vbo = 0;
	glGenBuffers(1, &vbo);
	if (!vbo)
	{
		printf("Failed to create vbo\n");
		return -1;
	}
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	struct Vertex
	{
		glm::vec3 position;
		glm::vec2 texCoord;
	} vertices[] = {
		{ glm::vec3(1.0f,  1.0f, 0.0f),	 glm::vec2(1.0f, 1.0f) },
		{ glm::vec3(1.0f, -1.0f, 0.0f),	 glm::vec2(1.0f, 0.0f) },
		{ glm::vec3(-1.0f, -1.0f, 0.0f), glm::vec2(0.0f, 0.0f) },
		{ glm::vec3(-1.0f,  1.0f, 0.0f), glm::vec2(0.0f, 1.0f) },
	};

	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * 4, vertices, GL_STATIC_DRAW);

	//Create VAO
	uint32_t vao = 0;
	glGenVertexArrays(1, &vao);
	if (!vao)
	{
		printf("Failed to create vao\n");
		return -1;
	}
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(sizeof(glm::vec3)));
	glEnableVertexAttribArray(1);

	//Texture
	uint32_t image = 0;
	glGenTextures(1, &image);
	if (!image)
	{
		printf("Failed to create iamge\n");
		return -1;
	}

	glBindTexture(GL_TEXTURE_2D, image);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	//glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, IMAGE_SIZE_X, IMAGE_SIZE_Y);

	//RayMarcher rayMarcher = RayMarcher(glm::uvec2(IMAGE_SIZE_X, IMAGE_SIZE_Y), 3.1415f / 4.0f);
	//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, IMAGE_SIZE_X, IMAGE_SIZE_Y, 0, GL_RGB, GL_FLOAT, rayMarcher.Render());
	//pixels2 = new glm::vec3[IMAGE_SIZE_X * IMAGE_SIZE_Y];
	//std::future<void> async = rayMarcher.AsyncRender(std::bind(&Update, std::placeholders::_1, std::placeholders::_2), 32);
	//async.wait();

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	clock_t time = 0, time1 = clock();

	while (!glfwWindowShouldClose(window))
	{
		glClear(GL_COLOR_BUFFER_BIT);

		glUseProgram(program);

		clock_t time2 = clock();
		time += time2 - time1;
		time1 = time2;

		glUniform1f(fragShaderUniforms.time, time / 1000.0f);
		glUniform1f(fragShaderUniforms.aspectRatio, aspectRatio);
		glUniform1f(fragShaderUniforms.fovFactor, fovFactor);

		glm::vec3 position = glm::vec3(sin(time / 2000.0f) * 7.0f, 4.0f, cos(time / 2000.0f) * -7.0f);

		glUniform3fv(fragShaderUniforms.cameraPosition, 1, glm::value_ptr(position));

		glm::mat4 matrix(1.0f);
		matrix = glm::rotate(matrix, -time / 2000.0f, glm::vec3(0.0f, 1.0f, 0.0f));
		matrix = glm::rotate(matrix, 0.4f, glm::vec3(1.0f, 0.0f, 0.0f));

		glUniformMatrix3fv(fragShaderUniforms.cameraMatrix, 1, false, glm::value_ptr(
			glm::mat3(matrix)

		));

		//glBindTexture(GL_TEXTURE_2D, image);
		/*if (changed)
		{
			mutex.lock();

			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, IMAGE_SIZE_X, IMAGE_SIZE_Y, GL_RGB, GL_FLOAT, pixels2);

			changed = false;
			mutex.unlock();
		}*/

		glBindVertexArray(vao);
		glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

		glfwSwapBuffers(window);

		glfwPollEvents();
	}

	glfwTerminate();

	return 0;
}