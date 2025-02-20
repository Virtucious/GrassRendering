#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <random>
#include "shader.h"
#include "camera.h"
#include "FastNoiseLite/Cpp/FastNoiseLite.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

const GLint width = 1920, height = 1080;

//Camera
Camera camera(glm::vec3(-7.78218f, 6.11892f, -7.79f));
float lastX = (float)width / 2.0;
float lastY = (float)height / 2.0;
bool firstMouse = true;

//timing
float deltaTime = 0.0f;	
float lastFrame = 0.0f;
float fps = 0.0f;		

//plane
int planeWidth = 20;			//size along x-axis
int depth = 20;					//size along z-axis
int rows = 50;					//Number of subdivisions along X
int cols = 50;					//Number of subdivisions along Z
std::vector<float> vertices;
std::vector<unsigned int> indices;

FastNoiseLite noise;

glm::vec3 lightPos(1.2f, 10.0f, 2.0f);

float getTerrainHeight(float x, float z, float amplitude, float frequency) {
	float noiseValue = noise.GetNoise(x * frequency, z * frequency);
	return noiseValue * amplitude;
}

int main()
{
#pragma region GLAD AND GLFW SETUP
	//Initialize GLAD and GLFW
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	//glfw window creation 
	GLFWwindow* window = glfwCreateWindow(width, height, "GrassRendering", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);

	//Tell GLFW to capture our mouse
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	//glad: load all OpenGL function pointers
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}
#pragma endregion

	//Enable depth test
	glEnable(GL_DEPTH_TEST);

	//Shader
	Shader planeShader("planeShader.vert", "planeShader.frag");
	Shader grassShader("grassShader.vert", "grassShader.frag");

	//Setup noise
	noise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);

#pragma region Setting up terrain
	//Creating indices to define triangles for plane
	for (int i = 0; i < rows; i++)
	{
		for (int j = 0; j < cols; j++)
		{
			int topLeft = i * (cols + 1) + j;
			int topRight = topLeft + 1;
			int bottomLeft = (i + 1) * (cols + 1) + j;
			int bottomRight = bottomLeft + 1;

			//First triangle
			indices.push_back(topLeft);
			indices.push_back(bottomLeft);
			indices.push_back(topRight);

			//Second triangle
			indices.push_back(topRight);
			indices.push_back(bottomLeft);
			indices.push_back(bottomRight);
		}
	}

	//Setting up the plane
	for (int i = 0; i <= rows; i++)
	{
		for (int j = 0; j <= cols; j++)
		{
			//Calculate vertex position
			float x = (float)j / cols * planeWidth - planeWidth / 2.0f; //Centering the plane
			float z = (float)i / rows * depth - depth / 2.0f; //Centering the plane
			float y = getTerrainHeight(x, z, 2.0f, 2.0f);

			vertices.push_back(x);
			vertices.push_back(y);
			vertices.push_back(z);

		}
	}

	//Creating normals
	std::vector<glm::vec3> normals(vertices.size() / 3, glm::vec3(0.0f));

	for (size_t i = 0; i < indices.size(); i += 3)
	{
		//Get the three vertices of the triangle
		unsigned int i0 = indices[i];
		unsigned int i1 = indices[i + 1];
		unsigned int i2 = indices[i + 2];

		glm::vec3 v0(vertices[i0 * 3], vertices[i0 * 3 + 1], vertices[i0 * 3 + 2]);
		glm::vec3 v1(vertices[i1 * 3], vertices[i1 * 3 + 1], vertices[i1 * 3 + 2]);
		glm::vec3 v2(vertices[i2 * 3], vertices[i2 * 3 + 1], vertices[i2 * 3 + 2]);

		//Calculate triangle's normal
		glm::vec3 edge1 = v1 - v0;
		glm::vec3 edge2 = v2 - v0;
		glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));

		//Accumulate the normal of each vertex
		normals[i0] += normal;
		normals[i1] += normal;
		normals[i2] += normal;
	}

	//Normalize the normals
	for (auto& normal : normals)
	{
		normal = glm::normalize(normal);
	}


	//Add normals with vertex positions to vertex buffer
	std::vector<float> vertexData;
	for (size_t i = 0; i < vertices.size() / 3; i++)
	{
		//Position
		vertexData.push_back(vertices[i * 3]);
		vertexData.push_back(vertices[i * 3 + 1]);
		vertexData.push_back(vertices[i * 3 + 2]);

		//Normals
		vertexData.push_back(normals[i].x);
		vertexData.push_back(normals[i].y);
		vertexData.push_back(normals[i].z);
	}



	unsigned int VAO, VBO, EBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), vertexData.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

	//Set up vertex attribute pointers
	//Position attribute 
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	//Normal attribute
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glBindVertexArray(0);
#pragma endregion

#pragma region Setting up grass

	//Generating texture
	unsigned int grassTexture;
	glGenTextures(1, &grassTexture);
	glBindTexture(GL_TEXTURE_2D, grassTexture);

	//Set texture parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	//Load image data
	stbi_set_flip_vertically_on_load(true);
	int width, height, nrChannels;
	unsigned char* data = stbi_load("resources/grass.png", &width, &height, &nrChannels, 0);
	if (data)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cerr << "Failed to load texture" << std::endl;
	}
	stbi_image_free(data);

	//Generating grass positions
	std::vector<glm::vec3> grassPositions;
	int grassCount = 50000;

	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_real_distribution<float> disX(0.0f, planeWidth);  // Random X within terrain width
	std::uniform_real_distribution<float> disZ(0.0f, depth);        // Random Z within terrain depth


	for (int i = 0; i < grassCount; i++)
	{
		float x = disX(gen) - planeWidth / 2.0f;
		float z = disZ(gen) - depth / 2.0f; 
		float y = getTerrainHeight(x, z, 2.0f, 2.0f);

		grassPositions.push_back(glm::vec3(x, y, z));
	}

	float grassQuad[] =
	{
		//Positions		//Texture coords
		-0.5f,  1.0f,	0.0f,	1.0f,
		-0.5f,  0.0f,	0.0f,	0.0f,
		 0.5f,  0.0f,	1.0f,	0.0f,

		-0.5f,  1.0f,	0.0f,	1.0f,
		 0.5f,  0.0f,	1.0f,	0.0f,
		 0.5f,  1.0f,	1.0f,	1.0f
	};

	unsigned int grassVAO, grassVBO;
	glGenVertexArrays(1, &grassVAO);
	glGenBuffers(1, &grassVBO);

	glBindVertexArray(grassVAO);
	glBindBuffer(GL_ARRAY_BUFFER, grassVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(grassQuad), grassQuad, GL_STATIC_DRAW);

	//Position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	//Texture Coordinate attribute
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
	glEnableVertexAttribArray(1);

	unsigned int grassInstanceVBO;
	glGenBuffers(1, &grassInstanceVBO);
	glBindBuffer(GL_ARRAY_BUFFER, grassInstanceVBO);
	glBufferData(GL_ARRAY_BUFFER, grassPositions.size() * sizeof(glm::vec3), grassPositions.data(), GL_STATIC_DRAW);

	glBindVertexArray(grassVAO);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glVertexAttribDivisor(2, 1);


	glBindVertexArray(0);

#pragma endregion

	while (!glfwWindowShouldClose(window))
	{
		//per-time frame logic
		float currentFrame = static_cast<float>(glfwGetTime());
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
		fps = 1.0 / deltaTime;
		
		//input
		processInput(window);

		//render
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		planeShader.use();

		//Set up matrices
		glm::mat4 model = glm::mat4(1.0f); 
		glm::mat4 view = camera.GetViewMatrix();
		glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)width / (float)height, 0.1f, 100.0f);

		planeShader.setMat4("model", model);
		planeShader.setMat4("view", view);
		planeShader.setMat4("projection", projection);

		//Set uniforms
		planeShader.setVec3("lightPos", lightPos);
		planeShader.setVec3("viewPos", camera.Position);
		planeShader.setVec3("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));
		planeShader.setVec3("objectColor", glm::vec3(0.76f, 0.69f, 0.04f));

		

		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);

		//Render grass
		grassShader.use();
		grassShader.setMat4("projection", projection);
		grassShader.setMat4("view", view);
		grassShader.setMat4("model", model);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, grassTexture);
		grassShader.setInt("grassTexture", 0);

		glBindVertexArray(grassVAO);
		glDrawArraysInstanced(GL_TRIANGLES, 0, 6, grassPositions.size());
		glBindVertexArray(0);

		//Render fps text
		std::cout << "FPS: " << fps << std::endl;

		//glfw swap buffers and poll IO events
		//------------------------------------
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
}

void processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		camera.ProcessKeyboard(FORWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		camera.ProcessKeyboard(BACKWARD, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		camera.ProcessKeyboard(LEFT, deltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		camera.ProcessKeyboard(RIGHT, deltaTime);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
	float xpos = static_cast<float>(xposIn);
	float ypos = static_cast<float>(yposIn);

	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	float xoffset = xpos - lastX;
	float yoffset = lastY - ypos;

	lastX = xpos;
	lastY = ypos;

	camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera.ProcessMouseScroll(static_cast<float>(yoffset));
}