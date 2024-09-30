#include <GL/glew.h>
#include <chrono>
#include <thread>
#include <exception>
#include <iostream>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <SDL2/SDL_events.h>
#include <json/json.h>
#include "MyWindow.h"
using namespace std;

using hrclock = chrono::high_resolution_clock;
using u8vec4 = glm::u8vec4;
using ivec2 = glm::ivec2;
using vec3 = glm::dvec3;
using mat4 = glm::dmat4;

static const ivec2 WINDOW_SIZE(512, 512);
static const unsigned int FPS = 60;
static const auto FRAME_DT = 1.0s / FPS;

class GraphicObject {
public:
	GraphicObject() {}
	~GraphicObject() {}

	const mat4& mat() const { return modelMatrix; }
	vec3& position() { return *(vec3*)&modelMatrix[3]; }
	const vec3& forward() const { return *(vec3*)&modelMatrix[2]; }
	const vec3& up() const { return *(vec3*)&modelMatrix[1]; }
	const vec3& left() const { return *(vec3*)&modelMatrix[0]; }

	void translate(const vec3& t) { modelMatrix = glm::translate(modelMatrix, t); }
	void rotate(double angle, const vec3& axis) { modelMatrix = glm::rotate(modelMatrix, angle, axis); }
	void scale(const vec3& s) { modelMatrix = glm::scale(modelMatrix, s); }

	void reset() { modelMatrix = glm::identity<mat4>(); }

	virtual void draw() = 0;

private:
	mat4 modelMatrix = glm::identity<mat4>();
};

class Triangle : public GraphicObject {
public:
	Triangle(const u8vec4& color, const vec3& center, double size) : color(color), center(center), size(size) {}

	void draw() override {
		glMultMatrixd(&mat()[0][0]);

		glColor4ub(color.r, color.g, color.b, color.a);
		glBegin(GL_TRIANGLES);
		glVertex3d(center.x, center.y + size, center.z);
		glVertex3d(center.x - size, center.y - size, center.z);
		glVertex3d(center.x + size, center.y - size, center.z);
		glEnd();
	}

private:
	u8vec4 color;
	vec3 center;
	double size;
};

struct Camera : public GraphicObject
{
	double fov = glm::radians(70.0);
	double zNear = 0.1;
	double zFar = 1000.0;

	double aspect() const { return static_cast<double>(WINDOW_SIZE.x / WINDOW_SIZE.y); }
	vec3 target() { return position() + forward(); }

	void draw() override 
	{
		glMatrixMode(GL_PROJECTION);
		mat4 projeccionMatrix = glm::perspective(fov, aspect(), zNear, zFar);
		glLoadMatrixd(&projeccionMatrix[0][0]);

		glMatrixMode(GL_MODELVIEW);
		mat4 viewMatrix = glm::lookAt(position(), target(), up());
		glLoadMatrixd(&viewMatrix[0][0]);
	}
};

Camera camera;
Triangle triangle(u8vec4(255, 0, 0, 255), vec3(0.0, 0.0, 0.0), 1.0);

static void init_openGL() {
	glewInit();
	if (!GLEW_VERSION_3_0) throw exception("OpenGL 3.0 API is not available.");
	glEnable(GL_DEPTH_TEST);
	glClearColor(0.5, 0.5, 0.5, 1.0);

	camera.translate(vec3(0.0, 0.0, -5.0));
	camera.rotate(glm::radians(0.0), vec3(1.0, 0.0, 0.0));
}

static void display_func(u8vec4 color, vec3 center, double size) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	camera.draw();
	triangle.draw();

	triangle.rotate(glm::radians(1.0), vec3(0.0, 1.0, 0.0));
}

static bool processEvents() {
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		switch (event.type) {
		case SDL_QUIT:
			return false;
		}
	}
	return true;
}

int main(int argc, char** argv) {
	MyWindow window("SDL2 Simple Example", WINDOW_SIZE.x, WINDOW_SIZE.y);

	init_openGL();

	Json::Value root;
	ifstream ifs("datos.json");
	ifs >> root;

	u8vec4 color;
	color.r = root["color"]["r"].asUInt();
	color.g = root["color"]["g"].asUInt();
	color.b = root["color"]["b"].asUInt();
	color.a = root["color"]["a"].asUInt();

	vec3 center;
	center.x = root["center"]["x"].asDouble();
	center.y = root["center"]["y"].asDouble();
	center.z = root["center"]["z"].asDouble();

	double size = root["size"].asDouble();

	while (processEvents()) {
		const auto t0 = hrclock::now();
		display_func(color, center, size);
		window.swapBuffers();
		const auto t1 = hrclock::now();
		const auto dt = t1 - t0;
		if(dt<FRAME_DT) this_thread::sleep_for(FRAME_DT - dt);
	}

	return 0;
}