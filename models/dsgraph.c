#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <GL/glut.h>

#include <math.h>

#include "types.h"
#include "endianess.h"

#define M_PI		3.14159265358979323846

typedef char bool;
#define true 1
#define false 0

#ifdef _DEBUG
#	ifdef WIN32
#		define BREAKPOINT()				{ __asm { int 3 }; }
#	else
#		define BREAKPOINT()
#	endif
#else
#	define BREAKPOINT()
#endif


void fatal(const char* message)
{
	BREAKPOINT()
	printf(message);
	exit(0);
}

#define print_once(message)			{ static bool done_it = false; if(!done_it) { printf(message); done_it = true; } }


typedef struct {
	u8				name[64];
	u32				dunno1;
	u16				palid;
	u16				texid;
	u32				dunno2[15];
} Material;

typedef struct {
	u32				start_ofs;
	u32				size;
	s32				bounds[3][2];
} Dlist;

typedef struct {
	u8				name[64];
	u32				dunno[104];
} Node;

typedef struct {
	u16				matid;
	u16				dlistid;
} Mesh;

typedef struct {
	u16				format;
	u16				width;
	u16				height;
	u16				pad;
	u32				image_ofs;
	u32				imagesize;
	u32				dunno1;
	u32				dunno2;
	u32				dunno3;
	u32				opaque;
	u32				dunno4;
	u32				dunno5;
} Texture;

typedef struct {
	u32				entries_ofs;
	u32				count;
	u32				dunno1;
	u32				dunno2;
} Palette;

typedef struct {
	Material*			materials;
	Dlist*				dlists;
	Node*				nodes;
	Mesh*				meshes;
	u32				num_meshes;
	Texture*			textures;
	u32				num_textures;
	Palette*			palettes;
	u16				num_materials;
	u32				size;
	u8*				data;
	u32				texturesize;
	u8*				texturedata;
} Scene;

typedef struct {
	u32				dunno1;
	u32				dunno2;
	u32				dunno3;
	u32				dunno4;
	u32				materials;
	u32				dlists;
	u32				nodes;
	u32				dunno5;
	u32				dunno6;
	u32				meshes;
	u32				num_meshes;		//maybe
	u32				textures;
	u32				num_textures;
	u32				palettes;
	u32				dunno7[4];
	u16				num_materials;
	u16				dunno8;
} __attribute__((__packed__)) HEADER_RAW;


Scene* scene = 0;
long time = 0;

bool texturing = true;
bool colouring = true;
bool wireframe = false;
bool culling = true;
bool tex_filtering = false;
bool alpha = true;

void do_reg(u32 reg, u32** data_pp, float vtx_state[3])
{
	u32* data = *data_pp;

	switch(reg) {
		//NOP
		case 0x400: {
		}
		break;

		//MTX_RESTORE
		case 0x450: {
			u32 idx = *(data++);
		}
		break;

		//COLOR
		case 0x480: {
			u32 rgb = *(data++);
			u32 r = (rgb >>  0) & 0x1F;
			u32 g = (rgb >>  5) & 0x1F;
			u32 b = (rgb >> 10) & 0x1F;
			if(colouring)
				glColor3f(((float)r) / 31.0f, ((float)g) / 31.0f, ((float)b) / 31.0f);
		}
		break;

		//NORMAL
		case 0x484: {
			u32 xyz = *(data++);
			s32 x = (xyz >>  0) & 0x3FF;			if(x & 0x200)			x |= 0xFFFFFC00;
			s32 y = (xyz >> 10) & 0x3FF;			if(y & 0x200)			y |= 0xFFFFFC00;
			s32 z = (xyz >> 20) & 0x3FF;			if(z & 0x200)			z |= 0xFFFFFC00;
			glNormal3f(((float)x) / 512.0f, ((float)y) / 512.0f, ((float)z) / 512.0f);
		}
		break;

		//TEXCOORD
		case 0x488: {
			u32 st = *(data++);
			s32 s = (st >>  0) & 0xFFFF;			if(s & 0x8000)		s |= 0xFFFF0000;
			s32 t = (st >> 16) & 0xFFFF;			if(t & 0x8000)		t |= 0xFFFF0000;
			glTexCoord2f(((float)s) / 16.0f, ((float)t) / 16.0f);
		}
		break;

		//VTX_16
		case 0x48C: {
			u32 xy = *(data++);
			s32 x = (xy >>  0) & 0xFFFF;			if(x & 0x8000)		x |= 0xFFFF0000;
			s32 y = (xy >> 16) & 0xFFFF;			if(y & 0x8000)		y |= 0xFFFF0000;
			s32 z = (*(data++)) & 0xFFFF;			if(z & 0x8000)		z |= 0xFFFF0000;

			vtx_state[0] = ((float)x) / 4096.0f;
			vtx_state[1] = ((float)y) / 4096.0f;
			vtx_state[2] = ((float)z) / 4096.0f;
			glVertex3fv(vtx_state);
		}
		break;

		//VTX_10
		case 0x490: {
			u32 xyz = *(data++);
			s32 x = (xyz >>  0) & 0x3FF;			if(x & 0x200)		x |= 0xFFFFFC00;
			s32 y = (xyz >> 10) & 0x3FF;			if(y & 0x200)		y |= 0xFFFFFC00;
			s32 z = (xyz >> 20) & 0x3FF;			if(z & 0x200)		z |= 0xFFFFFC00;
			vtx_state[0] = ((float)x) / 64.0f;
			vtx_state[1] = ((float)y) / 64.0f;
			vtx_state[2] = ((float)z) / 64.0f;
			glVertex3fv(vtx_state);
		}
		break;

		//VTX_XY
		case 0x494: {
			u32 xy = *(data++);
			s32 x = (xy >>  0) & 0xFFFF;			if(x & 0x8000)		x |= 0xFFFF0000;
			s32 y = (xy >> 16) & 0xFFFF;			if(y & 0x8000)		y |= 0xFFFF0000;
			vtx_state[0] = ((float)x) / 4096.0f;
			vtx_state[1] = ((float)y) / 4096.0f;
			glVertex3fv(vtx_state);
		}
		break;

		//VTX_XZ
		case 0x498: {
			u32 xz = *(data++);
			s32 x = (xz >>  0) & 0xFFFF;			if(x & 0x8000)		x |= 0xFFFF0000;
			s32 z = (xz >> 16) & 0xFFFF;			if(z & 0x8000)		z |= 0xFFFF0000;
			vtx_state[0] = ((float)x) / 4096.0f;
			vtx_state[2] = ((float)z) / 4096.0f;
			glVertex3fv(vtx_state);
		}
		break;

		//VTX_YZ
		case 0x49C: {
			u32 yz = *(data++);
			s32 y = (yz >>  0) & 0xFFFF;			if(y & 0x8000)		y |= 0xFFFF0000;
			s32 z = (yz >> 16) & 0xFFFF;			if(z & 0x8000)		z |= 0xFFFF0000;
			vtx_state[1] = ((float)y) / 4096.0f;
			vtx_state[2] = ((float)z) / 4096.0f;
			glVertex3fv(vtx_state);
		}
		break;

		//VTX_DIFF
		case 0x4A0: {
			u32 xyz = *(data++);
			s32 x = (xyz >>  0) & 0x3FF;			if(x & 0x200)		x |= 0xFFFFFC00;
			s32 y = (xyz >> 10) & 0x3FF;			if(y & 0x200)		y |= 0xFFFFFC00;
			s32 z = (xyz >> 20) & 0x3FF;			if(z & 0x200)		z |= 0xFFFFFC00;
			vtx_state[0] += ((float)x) / 4096.0f;
			vtx_state[1] += ((float)y) / 4096.0f;
			vtx_state[2] += ((float)z) / 4096.0f;
			glVertex3fv(vtx_state);
		}
		break;

		//DIF_AMB
		case 0x4C0: {
			u32 val = *(data++);
		}
		break;

		//BEGIN_VTXS
		case 0x500: {
			u32 type = *(data++);
			switch( type )
			{
				case 0:		glBegin(GL_TRIANGLES);			break;
				case 1:		glBegin(GL_QUADS);			break;
				case 2:		glBegin(GL_TRIANGLE_STRIP);		break;
				case 3:		glBegin(GL_QUAD_STRIP);			break;
				default:	fatal("Bogus geom type\n");		break;
			}
		}
		break;

		//END_VTXS
		case 0x504: {
			glEnd();
		}
		break;

		//Hmm?
		default: {
			fatal("Unhandled reg write\n");
		}
		break;
	}

	*data_pp = data;
}


/*

	First word is 4 byte-sized register indexes (where index is quadrupled and added to 0x04000400 to yield an address)
	These are processed least significant byte first
	Register index 0 means NOP
	Following this is the data to be written to each register, which is variable depending on the register being written

*/

void do_dlist(u32* data, u32 len)
{
	u32* end = data + len / 4;

	float vtx_state[3] = { 0.0f, 0.0f, 0.0f };

	while(data < end) {
		u32 regs = *(data++);

		u32 c;
		for(c = 0; c < 4; c++,regs >>= 8) {
			u32 reg = ((regs & 0xFF) << 2) + 0x400;

			do_reg(reg, &data, vtx_state);
		}
	}
}

void render_scene(Scene* scn)
{
	//Blech...can't spot mesh count anywhere so this will have to do
	Mesh* end = (Mesh*) (scn->data + scn->size);
	Mesh* mesh;

	if(alpha) {
		// pass 1: opaque
		unsigned int meshid = 0;
		for(mesh = scn->meshes; mesh < end; mesh++) {
			Dlist* dlist = &scn->dlists[mesh->dlistid];

			u32* data = (u32*) (scn->data + dlist->start_ofs);

			Material* material = &scn->materials[mesh->matid];
			Texture* texture = &scn->textures[material->texid];

			if(material->texid != 0xFFFF) {
				if(!texture->opaque) {
					continue;
				}

				if(texturing && !wireframe) {
					glBindTexture(GL_TEXTURE_2D, mesh->matid + 1);

					//Convert pixel coords to normalised STs
					glMatrixMode(GL_TEXTURE);
					glLoadIdentity();
					glScalef(1.0f / texture->width, 1.0f / texture->height, 1.0f);
				} else {
					glBindTexture(GL_TEXTURE_2D, 0);
				}
			} else {
				glBindTexture(GL_TEXTURE_2D, 0);
			}

			if(!colouring)
				glColor3f(1, 1, 1);

			do_dlist(data, dlist->size);
		}


		// pass 2: translucent
		glEnable(GL_BLEND);
		glDepthMask(GL_FALSE);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		for(mesh = scn->meshes; mesh < end; mesh++) {
			Dlist* dlist = &scn->dlists[mesh->dlistid];

			u32* data = (u32*) (scn->data + dlist->start_ofs);

			Material* material = &scn->materials[mesh->matid];
			Texture* texture = &scn->textures[material->texid];

			if(material->texid != 0xFFFF) {
				if(texture->opaque) {
					continue;
				}

				if(texturing && !wireframe) {
					glBindTexture(GL_TEXTURE_2D, mesh->matid + 1);

					//Convert pixel coords to normalised STs
					glMatrixMode(GL_TEXTURE);
					glLoadIdentity();
					glScalef(1.0f / texture->width, 1.0f / texture->height, 1.0f);
				} else {
					glBindTexture(GL_TEXTURE_2D, 0);
				}
			} else {
				glBindTexture(GL_TEXTURE_2D, 0);
			}

			if(!colouring)
				glColor3f(1, 1, 1);

			do_dlist(data, dlist->size);
		}
		glDepthMask(GL_TRUE);
		glDisable(GL_BLEND);
	} else {
		for(mesh = scn->meshes; mesh < end; mesh++) {
			Dlist* dlist = &scn->dlists[mesh->dlistid];

			u32* data = (u32*) (scn->data + dlist->start_ofs);

			Material* material = &scn->materials[mesh->matid];
			Texture* texture = &scn->textures[material->texid];

			if(material->texid != 0xFFFF) {
				if(texturing && !wireframe) {
					glBindTexture(GL_TEXTURE_2D, mesh->matid + 1);

					//Convert pixel coords to normalised STs
					glMatrixMode(GL_TEXTURE);
					glLoadIdentity();
					glScalef(1.0f / texture->width, 1.0f / texture->height, 1.0f);
				} else {
					glBindTexture(GL_TEXTURE_2D, 0);
				}
			} else {
				glBindTexture(GL_TEXTURE_2D, 0);
			}

			if(!colouring)
				glColor3f(1, 1, 1);

			do_dlist(data, dlist->size);
		}
	}
}



/*

	Texture formats:

		0 = 2bit palettised
		1 = 4bit palettised
		2 = 8bit palettised
		4 = 8bit greyscale
		5 = 16bit RGBA

	There may be more, but these are the only ones used by Metroid

	Palette entries are 16bit RGBA

*/

void make_textures(Scene* scn, u8* data, u32 datasize)
{
	u32 m;
	for(m = 0; m < scn->num_materials; m++) {
		Material* mat = &scn->materials[m];
		if(mat->texid == 0xFFFF)
			continue;
		if(mat->texid >= scn->num_textures) {
			printf("invalid texture id %04X for material %d\n", mat->texid, m);
			continue;
		}

		Texture* tex = &scn->textures[mat->texid];
		Palette* pal = &scn->palettes[mat->palid];

		u8* texels = (u8*) (data + tex->image_ofs);
		u16* paxels = (u16*) (data + pal->entries_ofs);

		if(tex->image_ofs >= datasize) {
			printf("invalid texel offset for texture %d in material %d\n", mat->texid, m);
			continue;
		}
		if(pal->entries_ofs >= datasize) {
			printf("invalid paxel offset for palette %d in material %d\n", mat->palid, m);
			continue;
		}

		u32 num_pixels = (u32)tex->width * (u32)tex->height;

		//printf("TEX(%s): %X %X %X %X %X %X\n", mat->name, tex->dunno1, tex->dunno2, tex->dunno3, tex->dunno4, tex->dunno5, mat->dunno1);

		u32 texsize = num_pixels;
		if(tex->format == 0)
			texsize /= 4;
		else if(tex->format == 1)
			texsize /= 2;
		else if(tex->format == 5)
			texsize *= 2;

		if((tex->image_ofs + texsize) > datasize) {
			printf("invalid texture size\n");
			continue;
		}
		//if((pal->entries_ofs + pal->count * 2) > datasize) {
		//	printf("invalid palette size (%d, %d)\n", pal->entries_ofs, pal->count);
		//	continue;
		//}

		u32* image = (u32*) malloc(sizeof(u32) * num_pixels);

		if(tex->format == 0) {				// 2bit palettised
			u32 p;
			for(p = 0; p < num_pixels; p++) {
				u32 index = texels[p / 4];
				index = (index >> ((p % 4) * 2)) & 0x3;
				u16 col = get16bit_LE((u8*)&paxels[index]);
				u32 r = ((col >>  0) & 0x1F) << 3;
				u32 g = ((col >>  5) & 0x1F) << 3;
				u32 b = ((col >> 10) & 0x1F) << 3;
				u32 a = (col & 0x8000) ? 0x00 : 0xFF;
				image[p] = (r << 0) | (g << 8) | (b << 16) | (a << 24);
			}
		} else if(tex->format == 1) {			// 4bit palettised
			u32 p;
			for(p = 0; p < num_pixels; p++) {
				u32 index = texels[p / 2];
				index = (index >> ((p % 2) * 4)) & 0xF;
				u16 col = get16bit_LE((u8*)&paxels[index]);
				u32 r = ((col >>  0) & 0x1F) << 3;
				u32 g = ((col >>  5) & 0x1F) << 3;
				u32 b = ((col >> 10) & 0x1F) << 3;
				u32 a = (col & 0x8000) ? 0x00 : 0xFF;
				image[p] = (r << 0) | (g << 8) | (b << 16) | (a << 24);
			}
		} else if(tex->format == 2) {			// 8bit palettised
			u32 p;
			for(p = 0; p < num_pixels; p++) {
				u32 index = texels[p];
				u16 col = get16bit_LE((u8*)&paxels[index]);
				u32 r = ((col >>  0) & 0x1F) << 3;
				u32 g = ((col >>  5) & 0x1F) << 3;
				u32 b = ((col >> 10) & 0x1F) << 3;
				u32 a = (col & 0x8000) ? 0x00 : 0xFF;
				image[p] = (r << 0) | (g << 8) | (b << 16) | (a << 24);
			}
		} else if(tex->format == 4) {			// 8bit greyscale
			u32 p;
			for(p = 0; p < num_pixels; p++) {
				u8 col = texels[p];
				image[p] = (col << 0) | (col << 8) | (col << 16) | (col << 24);
			}
		} else if(tex->format == 5) {			// 16it RGB
			u32 p;
			for(p = 0; p < num_pixels; p++) {
				u16 col = (u16)texels[p * 2 + 0] | (((u16)texels[p * 2 + 1]) << 8);
				u32 r = ((col >>  0) & 0x1F) << 3;
				u32 g = ((col >>  5) & 0x1F) << 3;
				u32 b = ((col >> 10) & 0x1F) << 3;
				u32 a = (col & 0x8000) ? 0x00 : 0xFF;
				image[p] = (r << 0) | (g << 8) | (b << 16) | (a << 24);
			}
		} else {
			BREAKPOINT();
			print_once("Unhandled texture-format\n");
			memset(image, 0x7F, num_pixels * 4);
		}

		if(mat->dunno1 == 0x60200) {
			u32 p;
			for(p = 0; p < num_pixels; p++) {
				u32 col = image[p];
				u32 r = (col >>  0) & 0xFF;
				u32 g = (col >>  8) & 0xFF;
				u32 b = (col >> 16) & 0xFF;
				u32 a = ((r + g + b) / 3) & 0xFF;
				image[p] = (r << 0) | (g << 8) | (b << 16) | (a << 24);
			}
			printf("material %d is set to transparent\n", m);
			tex->opaque = 0;
		}

		{
			u32 p;
			bool translucent = false;
			for(p = 0; p < num_pixels; p++) {
				if(((image[p] >> 24) & 0xFF) != 0xFF)
					translucent = true;
			}
			if(translucent && tex->opaque) {
				printf("texture %d marked as opaque but pixels are transparent\n", mat->texid);
			} else if(!translucent && !tex->opaque) {
				printf("texture %d marked as translucent but all pixels are opaque (%d)\n", mat->texid, tex->format);
			}
		}

		glBindTexture(GL_TEXTURE_2D, m + 1);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex->width, tex->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)image);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glBindTexture(GL_TEXTURE_2D, 0);
		free(image);
	}
}


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
	gluPerspective(90.0f, aspect, 0.02f, 32.0f);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glRotatef(xrot, 1, 0, 0);
	glRotatef(360.0f - yrot, 0, 1, 0);
	glTranslatef(-pos_x, -pos_y, -pos_z);

	render_scene(scene);

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
	const char* filename = argv[1];
	
	FILE* file = fopen(filename, "rb");
	if(!file) {
		printf("%s not found\n", filename);
		exit(1);
	}

	fseek(file, 0, SEEK_END);
	u32 scenesize = ftell(file);
	fseek(file, 0, SEEK_SET);
	u8* content = (u8*) malloc(scenesize);
	fread(content, 1, scenesize, file);
	fclose(file);

	scene = (Scene*) malloc(sizeof(Scene));
	if(scene == NULL) {
		printf("not enough memory!\n");
		exit(1);
	}

	if(argc == 3) {
		file = fopen(argv[2], "rb");
		if(!file) {
			printf("%s not found\n", filename);
			exit(1);
		}
		fseek(file, 0, SEEK_END);
		scene->texturesize = ftell(file);
		fseek(file, 0, SEEK_SET);
		scene->texturedata = (u8*) malloc(scene->texturesize);
		fread(scene->texturedata, 1, scene->texturesize, file);
		fclose(file);
	} else {
		scene->texturedata = content;
		scene->texturesize = scenesize;
	}

	HEADER_RAW* rawheader = (HEADER_RAW*) content;

	scene->data = content;
	scene->size = scenesize;
	scene->materials	= (Material*)	((uintptr_t)scene->data + (uintptr_t)get32bit_LE((u8*)&rawheader->materials));
	scene->dlists		= (Dlist*)	((uintptr_t)scene->data + (uintptr_t)get32bit_LE((u8*)&rawheader->dlists));
	scene->nodes		= (Node*)	((uintptr_t)scene->data + (uintptr_t)get32bit_LE((u8*)&rawheader->nodes));
	scene->meshes		= (Mesh*)	((uintptr_t)scene->data + (uintptr_t)get32bit_LE((u8*)&rawheader->meshes));
	scene->textures		= (Texture*)	((uintptr_t)scene->data + (uintptr_t)get32bit_LE((u8*)&rawheader->textures));
	scene->palettes		= (Palette*)	((uintptr_t)scene->data + (uintptr_t)get32bit_LE((u8*)&rawheader->palettes));
	scene->num_meshes	= get32bit_LE((u8*)&rawheader->num_meshes);
	scene->num_textures	= get32bit_LE((u8*)&rawheader->num_textures);
	scene->num_materials	= get16bit_LE((u8*)&rawheader->num_materials);

	for(i = 0; i < scene->num_materials; i++) {
		scene->materials[i].palid = get16bit_LE((u8*)&scene->materials[i].palid);
		scene->materials[i].texid = get16bit_LE((u8*)&scene->materials[i].texid);
		unsigned int p = scene->materials[i].palid;
		if(p == 0xFFFF)
			continue;
		scene->palettes[p].count = get32bit_LE((u8*)&scene->palettes[p].count);
		scene->palettes[p].entries_ofs = get32bit_LE((u8*)&scene->palettes[p].entries_ofs);
	}
	for(i = 0; i < scene->num_textures; i++) {
		scene->textures[i].format = get16bit_LE((u8*)&scene->textures[i].format);
		scene->textures[i].width = get16bit_LE((u8*)&scene->textures[i].width);
		scene->textures[i].height = get16bit_LE((u8*)&scene->textures[i].height);
		scene->textures[i].image_ofs = get32bit_LE((u8*)&scene->textures[i].image_ofs);
		scene->textures[i].imagesize = get32bit_LE((u8*)&scene->textures[i].imagesize);
	}

	unsigned int meshcount = 0;
	Mesh* end = (Mesh*) (scene->data + scene->size);
	Mesh* mesh;
	for(mesh = scene->meshes; mesh < end; mesh++) {
		meshcount++;
		mesh->matid = get16bit_LE((u8*)&mesh->matid);
		mesh->dlistid = get16bit_LE((u8*)&mesh->dlistid);
		Dlist* dlist = &scene->dlists[mesh->dlistid];
		dlist->start_ofs = get32bit_LE((u8*)&dlist->start_ofs);
		dlist->size = get32bit_LE((u8*)&dlist->size);
		u32* data = (u32*) (scene->data + dlist->start_ofs);
		u32* end = data + dlist->size / 4;
		while(data < end) {
			*data = get32bit_LE((u8*)data);
			data++;
		}
	}
	//printf("%d/%d\n", scene->num_meshes, meshcount);
	scene->num_meshes = meshcount;

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

	if(scene->texturedata != scene->data) {
		printf("using dedicated texture file (%d bytes)\n", scene->texturesize);
	}

	make_textures(scene, scene->texturedata, scene->texturesize);

	printf(" - Hold left mouse button to zoom\n");
	printf(" - Hold right mouse button to rotate\n");
	printf(" - T toggles texturing\n");
	printf(" - C toggles vertex colours\n");
	printf(" - W toggles wireframe\n");
	printf(" - B toggles backface culling\n");
	printf(" - F toggles texture filtering\n");

	glutMainLoop();

	return 0;
}
