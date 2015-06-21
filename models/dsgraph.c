#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <GL/glut.h>

#include <math.h>

#include "types.h"
#include "endianess.h"
#include "scene.h"

#define M_PI		3.14159265358979323846





#include "scene.h"

SCENE* scene = 0;
long time = 0;

bool texturing = true;
bool colouring = true;
bool wireframe = false;
bool culling = true;
bool tex_filtering = true;
bool alpha = true;


float sin_deg(float deg) {
	return sin(deg * M_PI / 180);
}

float cos_deg(float deg) {
	return cos(deg * M_PI / 180);
}

bool mouse_r = false;
bool mouse_l = false;
int mouse_x = 0;
int mouse_y = 0;

float heading = 0.0f;
float heading_y = 0.0f;

float xrot = 0.0f;
float yrot = 0.0f;

float pos_x = 0.0f;
float pos_y = 0.0f;
float pos_z = 0.0f;

bool key_down_forward = false;
bool key_down_backward = false;
bool key_down_speed = false;

void move_forward(float distance)
{
	pos_x -= distance * (float)sin_deg(heading) * cos_deg(heading_y) * 0.05f;
	pos_y -= distance * (float)sin_deg(heading_y) * 0.05f;
	pos_z -= distance * (float)cos_deg(heading) * cos_deg(heading_y) * 0.05f;
}

void move_backward(float distance)
{
	pos_x += distance * (float)sin_deg(heading) * cos_deg(heading_y) * 0.05f;
	pos_y += distance * (float)sin_deg(heading_y) * 0.05f;
	pos_z += distance * (float)cos_deg(heading) * cos_deg(heading_y) * 0.05f;
}

void mouse_func(int button, int state, int x, int y)
{
	mouse_x = x;
	mouse_y = y;
	if(button == GLUT_RIGHT_BUTTON)
		mouse_r = state == GLUT_DOWN;
	if(button == GLUT_LEFT_BUTTON)
		mouse_l = state == GLUT_DOWN;
}

void motion_func(int x, int y)
{
	int dx = x - mouse_x;
	int dy = y - mouse_y;

	if( mouse_l && (dx || dy)) {
		if(dx || dy) {
			heading -= dx / 5.0f;
			if(heading > 360)	heading -= 360;
			if(heading < 0)		heading += 360;
			heading_y += dy / 5.0f;
			if(heading_y > 360)	heading_y -= 360;
			if(heading_y < 0)	heading_y += 360;
			xrot = heading_y;
			yrot = heading;
			glutPostRedisplay();
		}
	}

	mouse_x = x;
	mouse_y = y;
}

void special_func(int key, int x, int y)
{
	switch(key) {
		case GLUT_KEY_UP:
			key_down_forward = true;
			break;

		case GLUT_KEY_DOWN:
			key_down_backward = true;
			break;
	}
}

void special_up_func(int key, int x, int y)
{
	switch(key) {
		case GLUT_KEY_UP:
			key_down_forward = false;
			break;

		case GLUT_KEY_DOWN:
			key_down_backward = false;
			break;
	}
}

void kb_func(unsigned char key, int x, int y)
{
	switch(key) {
		case 't':	case 'T': {
			texturing = !texturing;
			glutPostRedisplay();
		}
		break;

		case 'c':	case 'C': {
			colouring = !colouring;
			glutPostRedisplay();
		}
		break;

		case 'w':	case 'W': {
			wireframe = !wireframe;
			glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
			glutPostRedisplay();
		}
		break;

		case 'b':	case 'B': {
			culling = !culling;
			(culling ? glEnable : glDisable)(GL_CULL_FACE);
			glutPostRedisplay();
		}
		break;

		case 'f':	case 'F': {
			tex_filtering = !tex_filtering;
			u32 m;
			for(m = 0; m < scene->num_materials; m++) {
				glBindTexture(GL_TEXTURE_2D, m + 1);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, tex_filtering ? GL_LINEAR : GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, tex_filtering ? GL_LINEAR : GL_NEAREST);
			}
			glutPostRedisplay();
		}
		break;

		case 'a':	case 'A': {
			alpha = !alpha;
			glutPostRedisplay();
		}
		break;

		case ' ':
			key_down_speed = true;
			break;
	}
}

void kb_up_func(unsigned char key, int x, int y) {
	switch(key) {
		case ' ':
			key_down_speed = false;
			break;
	}
}


void process()
{
	long now = glutGet(GLUT_ELAPSED_TIME);
	float dt = (now - time) / 1000.0f;
	float distance = dt;
	if(key_down_speed)
		distance *= 10.0f;
	if(key_down_forward)
		move_forward(distance);
	if(key_down_backward)
		move_backward(distance);
	time = now;
}

void display_func(void)
{
	process();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	GLfloat vp[4];
	glGetFloatv(GL_VIEWPORT, vp);
	float aspect = (vp[2] - vp[0]) / (vp[3] - vp[1]);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(90.0f, aspect, 0.01f, 32.0f);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glRotatef(xrot, 1, 0, 0);
	glRotatef(360.0f - yrot, 0, 1, 0);
	glTranslatef(-pos_x, -pos_y, -pos_z);

	SCENE_render(scene);

	glutSwapBuffers();
}






int main(int argc, char **argv)
{
	unsigned int i;
	if((argc != 2) && (argc != 3)) {
		printf("Metroid Prime Hunters model viewer\n");
		printf("Usage: dsgraph <filename> [textures]\n");
		exit(0);
	}

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE);
	glutInitWindowSize(512, 512);
	glutInitWindowPosition(100, 100);
	glutCreateWindow("dsgraph");
	glutDisplayFunc(display_func);
	glutIdleFunc(display_func);
	glutMouseFunc(mouse_func);
	glutMotionFunc(motion_func);
	glutKeyboardFunc(kb_func);
	glutKeyboardUpFunc(kb_up_func);
	glutSpecialFunc(special_func);
	glutSpecialUpFunc(special_up_func);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_CULL_FACE);

	glDepthFunc(GL_LEQUAL);

	const char* model = argv[1];
	const char* textures = argc == 3 ? argv[2] : NULL;
	scene = SCENE_load_file(model, textures);

	printf(" - Hold left mouse button to look around\n");
	printf(" - Up/Down moves around\n");
	printf(" - T toggles texturing\n");
	printf(" - C toggles vertex colours\n");
	printf(" - W toggles wireframe\n");
	printf(" - B toggles backface culling\n");
	printf(" - F toggles texture filtering\n");

	glutMainLoop();

	return 0;
}
