// Слънчева система: слънце = сфера, планета = кубче, 2 луни = 2 пирамиди

//#define GLEW_STATIC
#include <GL\glew.h>

#include <SDL.h>
#include <SDL_opengl.h>
#include <cmath>
#include <stdio.h>
#include <vector>
#include <gl\GLU.h>
#include "Shader.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

bool init();
bool initGL();
void render();
GLuint CreateCube(GLfloat width, GLuint& VBO, GLuint& EBO);
GLuint CreatePyramid(GLfloat width, GLuint& VBO, GLuint& EBO);
GLuint CreateSphere(float, GLuint&, GLuint&, GLsizei&);
Shader CreateShaderProg();
void DrawCube(GLuint id);
void DrawPyramid(GLuint id);
void DrawSphere(GLuint id);
void close();

//The window we'll be rendering to
SDL_Window* gWindow = NULL;

//OpenGL context
SDL_GLContext gContext;

Shader gShaderProg;

GLuint gVAO, gVBO, gEBO;                      // куб
GLuint gPyrVAO, gPyrVBO, gPyrEBO;             // пирамида
GLuint gSphereVAO, gSphereVBO, gSphereEBO;  // сфера
GLsizei gSphereIndexCount = 0;

using namespace glm;
vec3 movement;

void HandleKeyUp(const SDL_KeyboardEvent& key);


int main(int argc, char* args[])
{
	init();

	SDL_Event e;
	bool quit = false;
	while (!quit)
	{
		while (SDL_PollEvent(&e) != 0)
		{
			switch (e.type)
			{
			case SDL_QUIT:
				quit = true;
				break;
			case SDL_KEYDOWN:
				if (e.key.keysym.sym == SDLK_ESCAPE)
				{
					quit = true;
				}
				else {
					HandleKeyUp(e.key);
				}
				break;
			}
		}

		render();

		SDL_GL_SwapWindow(gWindow);
	}

	close();

	return 0;
}

void HandleKeyUp(const SDL_KeyboardEvent& key)
{
	switch (key.keysym.sym) {
	case SDLK_UP:
		movement.y += 0.1f;
		break;
	case SDLK_DOWN:
		movement.y -= 0.1f;
		break;
	case SDLK_LEFT:
		movement.x -= 0.1f;
		break;
	case SDLK_RIGHT:
		movement.x += 0.1f;
		break;
	}
}

bool init()
{
	bool success = true;

	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		printf("SDL could not initialize! SDL Error: %s\n", SDL_GetError());
		success = false;
	}
	else
	{
		//Use OpenGL 3.3
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);   // depth буфер

		gWindow = SDL_CreateWindow("Solar System", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480,
			SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
		if (gWindow == NULL)
		{
			printf("Window could not be created! SDL Error: %s\n", SDL_GetError());
			success = false;
		}
		else
		{
			gContext = SDL_GL_CreateContext(gWindow);
			if (gContext == NULL)
			{
				printf("OpenGL context could not be created! SDL Error: %s\n", SDL_GetError());
				success = false;
			}
			else
			{
				if (SDL_GL_SetSwapInterval(1) < 0)
				{
					printf("Warning: Unable to set VSync! SDL Error: %s\n", SDL_GetError());
				}

				if (!initGL())
				{
					printf("Unable to initialize OpenGL!\n");
					success = false;
				}
			}
		}
	}

	return success;
}

bool initGL()
{
	bool success = true;
	GLenum error = GL_NO_ERROR;

	glewInit();

	error = glGetError();
	if (error != GL_NO_ERROR)
	{
		success = false;
		printf("Error initializing OpenGL! %s\n", gluErrorString(error));
	}

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_DEPTH_TEST);

	gShaderProg = CreateShaderProg();
	gVAO = CreateCube(1.0f, gVBO, gEBO);
	gPyrVAO = CreatePyramid(1.0f, gPyrVBO, gPyrEBO);
	gSphereVAO = CreateSphere(2.0f, gSphereVBO, gSphereEBO, gSphereIndexCount);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	return success;
}

void close()
{
	glDeleteProgram(gShaderProg.ID);

	glDeleteVertexArrays(1, &gVAO);
	glDeleteBuffers(1, &gVBO);
	glDeleteBuffers(1, &gEBO);

	glDeleteVertexArrays(1, &gPyrVAO);
	glDeleteBuffers(1, &gPyrVBO);
	glDeleteBuffers(1, &gPyrEBO);

	glDeleteVertexArrays(1, &gSphereVAO);
	glDeleteBuffers(1, &gSphereVBO);
	glDeleteBuffers(1, &gSphereEBO);

	SDL_GL_DeleteContext(gContext);
	SDL_DestroyWindow(gWindow);
	gWindow = NULL;

	SDL_Quit();
}

void render()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	static float ang = 0.0f;
	ang += 1.0f;

	const vec3 X(1.0f, 0.0f, 0.0f);   // planet orbit around X - for pyramid 1
	const vec3 Y(0.0f, 1.0f, 0.0f);   // planet orbit around Y - for cube
	const vec3 Z(0.0f, 0.0f, 1.0f);   // planet orbit around Z - for pyramid 2

	gShaderProg.use();
	gShaderProg.setMat4("projection", perspective(radians(45.0f), 640.0f / 480.0f, 0.1f, 100.0f));
	gShaderProg.setMat4("view", lookAt(vec3(0.0f, 5.0f, 15.0f), vec3(0.0f), Y));

	mat4 system = translate(mat4(1.0f), movement);

	// sun - sphere 
	gShaderProg.setMat4("model", system);
	DrawSphere(gSphereVAO);

	// cube - circle with radius 5 around Y of the sphere
	mat4 cube = translate(rotate(system, radians(ang), Y), vec3(5.0f, 0.0f, 0.0f));
	gShaderProg.setMat4("model", cube);
	DrawCube(gVAO);

	// 1 pyramid - radius 2 around X of the cube
	mat4 pyr1 = translate(rotate(cube, radians(ang * 2.0f), X), vec3(0.0f, 2.0f, 0.0f));
	gShaderProg.setMat4("model", scale(pyr1, vec3(0.5f)));
	DrawPyramid(gPyrVAO);

	// 2 pyramid - radius 2 around Z of the cube
	mat4 pyr2 = translate(rotate(cube, radians(ang * 2.0f), Z), vec3(2.0f, 0.0f, 0.0f));
	gShaderProg.setMat4("model", scale(pyr2, vec3(0.5f)));
	DrawPyramid(gPyrVAO);
}


Shader CreateShaderProg()
{
	gShaderProg.Load(".\\shaders\\vertex.vs", ".\\shaders\\fragment.fs");
	return gShaderProg;
}

//  cube with 8 vertices: x, y, z, r, g, b
GLuint CreateCube(GLfloat width, GLuint& VBO, GLuint& EBO)
{
	GLfloat vertices[] = {
		-0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 0.0f, // 0
		 0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 1.0f, // 1
		 0.5f,  0.5f,  0.5f,  0.0f, 0.0f, 1.0f, // 2
		-0.5f,  0.5f,  0.5f,  0.0f, 1.0f, 0.0f, // 3
		-0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f, // 4
		 0.5f, -0.5f, -0.5f,  0.0f, 1.0f, 0.0f, // 5
		 0.5f,  0.5f, -0.5f,  0.0f, 1.0f, 1.0f, // 6
		-0.5f,  0.5f, -0.5f,  0.5f, 0.5f, 0.5f  // 7
	};

	GLuint indices[] = {
		0, 1, 2,  2, 3, 0, // front
		1, 5, 6,  6, 2, 1, // right
		5, 4, 7,  7, 6, 5, // back
		4, 0, 3,  3, 7, 4, // left
		3, 2, 6,  6, 7, 3, // up
		4, 5, 1,  1, 0, 4  // bottom
	};

	GLuint VAO;
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);
	glGenVertexArrays(1, &VAO);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	return VAO;
}

//pyramid with 5 vertices 
GLuint CreatePyramid(GLfloat width, GLuint& VBO, GLuint& EBO)
{
	GLfloat vertices[] = {
		 0.0f,  0.5f,  0.0f,  0.0f, 0.0f, 1.0f, // 0 apex
		-0.5f, -0.5f,  0.5f,  0.0f, 1.0f, 0.0f, // 1 front left base
		 0.5f, -0.5f,  0.5f,  0.0f, 0.0f, 1.0f, // 2 front right base
		 0.5f, -0.5f, -0.5f,  1.0f, 0.0f, 0.0f, // 3 back right base
		-0.5f, -0.5f, -0.5f,  1.0f, 1.0f, 1.0f  // 4 back left base
	};

	GLuint indices[] = {
		0, 1, 2, // front face
		0, 2, 3, // right face
		0, 3, 4, // back face
		0, 4, 1, // left face
		1, 4, 3, // base 1
		3, 2, 1  // base 2
	};

	GLuint VAO;
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);
	glGenVertexArrays(1, &VAO);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	return VAO;
}

GLuint CreateSphere(float radius, GLuint& VBO, GLuint& EBO, GLsizei& indexCount)
{
	const int sectors = 32;
	const int stacks = 24;
	const float pi = 3.14159265358979323846f;
	const GLfloat color[] = { 1.0f, 0.65f, 0.05f };
	std::vector<GLfloat> vertices;
	std::vector<GLuint> indices;

	for (int stack = 0; stack <= stacks; ++stack)
	{
		const float stackAngle = pi * static_cast<float>(stack) / stacks;
		const float ringRadius = radius * std::sin(stackAngle);
		const float y = radius * std::cos(stackAngle);

		for (int sector = 0; sector <= sectors; ++sector)
		{
			const float sectorAngle = 2.0f * pi * static_cast<float>(sector) / sectors;
			vertices.push_back(ringRadius * std::cos(sectorAngle));
			vertices.push_back(y);
			vertices.push_back(ringRadius * std::sin(sectorAngle));
			vertices.insert(vertices.end(), color, color + 3);
		}
	}

	for (int stack = 0; stack < stacks; ++stack)
	{
		for (int sector = 0; sector < sectors; ++sector)
		{
			const GLuint first = static_cast<GLuint>(stack * (sectors + 1) + sector);
			const GLuint second = first + sectors + 1;

			if (stack > 0)
			{
				indices.push_back(first);
				indices.push_back(second);
				indices.push_back(first + 1);
			}

			if (stack < stacks - 1)
			{
				indices.push_back(first + 1);
				indices.push_back(second);
				indices.push_back(second + 1);
			}
		}
	}

	indexCount = static_cast<GLsizei>(indices.size());

	GLuint VAO;
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);
	glGenVertexArrays(1, &VAO);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(GLfloat)), vertices.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(indices.size() * sizeof(GLuint)), indices.data(), GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	return VAO;
}

void DrawCube(GLuint vaoID)
{
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	gShaderProg.use();
	glBindVertexArray(vaoID);

	gShaderProg.setFloat("alpha", 1.0f);
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}

void DrawPyramid(GLuint vaoID)
{
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	gShaderProg.use();
	glBindVertexArray(vaoID);

	gShaderProg.setFloat("alpha", 1.0f);
	glDrawElements(GL_TRIANGLES, 18, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}

void DrawSphere(GLuint vaoID)
{
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	gShaderProg.use();
	glBindVertexArray(vaoID);

	gShaderProg.setFloat("alpha", 1.0f);
	glDrawElements(GL_TRIANGLES, gSphereIndexCount, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}