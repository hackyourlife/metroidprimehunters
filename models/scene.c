#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <GL/gl.h>

#include "types.h"
#include "endianess.h"
#include "scene.h"

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
	u32				flags;
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
} __attribute__((__packed__)) HEADER;


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

void build_meshes(SCENE* scene, Mesh* meshes, Dlist* dlists, u8* scenedata, unsigned int scenesize)
{
	scene->meshes = (MESH*) malloc(scene->num_meshes * sizeof(MESH));
	if(!scene->meshes)
		fatal("not enough memory");

	Mesh* end = (Mesh*) (scenedata + scenesize);
	Mesh* mesh;
	MESH* m;

	for(mesh = meshes, m = scene->meshes; mesh < end; mesh++, m++) {
		Dlist* dlist = &dlists[mesh->dlistid];
		u32* data = (u32*) (scenedata + dlist->start_ofs);
		m->matid = mesh->matid;
		m->dlistid = glGenLists(1);
		glNewList(m->dlistid, GL_COMPILE);
		do_dlist(data, dlist->size);
		glEndList();
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
void make_textures(SCENE* scn, Material* materials, unsigned int num_materials, Texture* textures, unsigned int num_textures, Palette* palettes, u8* data, u32 datasize)
{
	u32 m;
	for(m = 0; m < num_materials; m++) {
		Material* mat = &materials[m];
		if(mat->texid == 0xFFFF)
			continue;
		if(mat->texid >= num_textures) {
			printf("invalid texture id %04X for material %d\n", mat->texid, m);
			continue;
		}

		Texture* tex = &textures[mat->texid];
		Palette* pal = &palettes[mat->palid];

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
		if(!image)
			fatal("not enough memory");

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

		if(mat->flags == 0x60200) {
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
			scn->textures[mat->texid].opaque = 0;
		}

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

		glGenTextures(1, &scn->materials[m].tex);
		glBindTexture(GL_TEXTURE_2D, scn->materials[m].tex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex->width, tex->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)image);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glBindTexture(GL_TEXTURE_2D, 0);
		free(image);
	}
}

SCENE* SCENE_load(u8* scenedata, unsigned int scenesize, u8* texturedata, unsigned int texturesize)
{
	unsigned int i;

	SCENE* scene = (SCENE*) malloc(sizeof(SCENE));
	if(!scene)
		fatal("not enough memory");

	HEADER* rawheader = (HEADER*) scenedata;

	Material* materials	= (Material*)	((uintptr_t)scenedata + (uintptr_t)get32bit_LE((u8*)&rawheader->materials));
	Dlist* dlists		= (Dlist*)	((uintptr_t)scenedata + (uintptr_t)get32bit_LE((u8*)&rawheader->dlists));
	Node* nodes		= (Node*)	((uintptr_t)scenedata + (uintptr_t)get32bit_LE((u8*)&rawheader->nodes));
	Mesh* meshes		= (Mesh*)	((uintptr_t)scenedata + (uintptr_t)get32bit_LE((u8*)&rawheader->meshes));
	Texture* textures	= (Texture*)	((uintptr_t)scenedata + (uintptr_t)get32bit_LE((u8*)&rawheader->textures));
	Palette* palettes	= (Palette*)	((uintptr_t)scenedata + (uintptr_t)get32bit_LE((u8*)&rawheader->palettes));

	scene->num_textures	= get32bit_LE((u8*)&rawheader->num_textures);
	scene->num_materials	= get16bit_LE((u8*)&rawheader->num_materials);

	scene->materials = (MATERIAL*) malloc(scene->num_materials * sizeof(MATERIAL));
	scene->textures = (TEXTURE*) malloc(scene->num_textures * sizeof(TEXTURE));
	if(!scene->materials || !scene->textures)
		fatal("not enough memory");

	for(i = 0; i < scene->num_materials; i++) {
		Material* m = &materials[i];
		m->palid = get16bit_LE((u8*)&m->palid);
		m->texid = get16bit_LE((u8*)&m->texid);

		MATERIAL* mat = &scene->materials[i];
		strcpy(mat->name, m->name);
		mat->texid = m->texid;
		mat->flags = m->flags;

		unsigned int p = m->palid;
		if(p == 0xFFFF)
			continue;
		palettes[p].count = get32bit_LE((u8*)&palettes[p].count);
		palettes[p].entries_ofs = get32bit_LE((u8*)&palettes[p].entries_ofs);
	}
	for(i = 0; i < scene->num_textures; i++) {
		TEXTURE* tex = &scene->textures[i];
		Texture* t = &textures[i];
		t->format = get16bit_LE((u8*)&t->format);
		t->width = get16bit_LE((u8*)&t->width);
		t->height = get16bit_LE((u8*)&t->height);
		t->image_ofs = get32bit_LE((u8*)&t->image_ofs);
		t->imagesize = get32bit_LE((u8*)&t->imagesize);
		tex->width = t->width;
		tex->height = t->height;
		tex->opaque = t->opaque;
	}

	unsigned int meshcount = 0;
	Mesh* end = (Mesh*) (scenedata + scenesize);
	Mesh* mesh;
	for(mesh = meshes; mesh < end; mesh++) {
		meshcount++;
		mesh->matid = get16bit_LE((u8*)&mesh->matid);
		mesh->dlistid = get16bit_LE((u8*)&mesh->dlistid);
		Dlist* dlist = &dlists[mesh->dlistid];
		dlist->start_ofs = get32bit_LE((u8*)&dlist->start_ofs);
		dlist->size = get32bit_LE((u8*)&dlist->size);
		u32* data = (u32*) (scenedata + dlist->start_ofs);
		u32* end = data + dlist->size / 4;
		while(data < end) {
			*data = get32bit_LE((u8*)data);
			data++;
		}
	}
	scene->num_meshes = meshcount;

	make_textures(scene, materials, scene->num_materials, textures, scene->num_textures, palettes, texturedata, texturesize);

	i = 0;
	scene->meshes = (MESH*) malloc(sizeof(MESH));
	if(!scene->meshes)
		fatal("not enough memory");
	build_meshes(scene, meshes, dlists, scenedata, scenesize);

	return scene;
}

SCENE* SCENE_load_file(const char* model, const char* textures)
{
	FILE* file = fopen(model, "rb");
	if(!file) {
		printf("%s not found\n", model);
		exit(1);
	}

	fseek(file, 0, SEEK_END);
	u32 scenesize = ftell(file);
	fseek(file, 0, SEEK_SET);
	u8* content = (u8*) malloc(scenesize);
	fread(content, 1, scenesize, file);
	fclose(file);

	u32 texturesize = scenesize;
	u8* texturedata = content;

	if(textures != NULL) {
		file = fopen(textures, "rb");
		if(!file) {
			printf("%s not found\n", textures);
			exit(1);
		}
		fseek(file, 0, SEEK_END);
		texturesize = ftell(file);
		fseek(file, 0, SEEK_SET);
		texturedata = (u8*) malloc(texturesize);
		fread(texturedata, 1, texturesize, file);
		fclose(file);
	}

	SCENE* scene = SCENE_load(content, scenesize, texturedata, texturesize);
	if(texturedata != content)
		free(texturedata);
	free(content);
	return scene;
}

void SCENE_free(SCENE* scene)
{
	unsigned int i;
	for(i = 0; i < scene->num_materials; i++) {
		glDeleteTextures(1, &scene->materials[i].tex);
	}
	for(i = 0; i < scene->num_meshes; i++) {
		glDeleteLists(1, scene->meshes[i].dlistid);
	}
	free(scene->textures);
	free(scene->materials);
	free(scene->meshes);
	free(scene);
}

void SCENE_render(SCENE* scene)
{
	unsigned int i;
	// pass 1: opaque
	for(i = 0; i < scene->num_meshes; i++) {
		MESH* mesh = &scene->meshes[i];
		MATERIAL* material = &scene->materials[mesh->matid];
		TEXTURE* texture = &scene->textures[material->texid];

		if(material->texid != 0xFFFF) {
			if(!texture->opaque) {
				continue;
			}

			glBindTexture(GL_TEXTURE_2D, material->tex);

			//Convert pixel coords to normalised STs
			glMatrixMode(GL_TEXTURE);
			glLoadIdentity();
			glScalef(1.0f / texture->width, 1.0f / texture->height, 1.0f);
		} else {
			glBindTexture(GL_TEXTURE_2D, 0);
		}

		glCallList(mesh->dlistid);
	}


	// pass 2: translucent
	glEnable(GL_BLEND);
	glDepthMask(GL_FALSE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	for(i = 0; i < scene->num_meshes; i++) {
		MESH* mesh = &scene->meshes[i];
		MATERIAL* material = &scene->materials[mesh->matid];
		TEXTURE* texture = &scene->textures[material->texid];

		if(material->texid != 0xFFFF) {
			if(texture->opaque) {
				continue;
			}

			glBindTexture(GL_TEXTURE_2D, material->tex);

			//Convert pixel coords to normalised STs
			glMatrixMode(GL_TEXTURE);
			glLoadIdentity();
			glScalef(1.0f / texture->width, 1.0f / texture->height, 1.0f);
		} else {
			glBindTexture(GL_TEXTURE_2D, 0);
		}

		glCallList(mesh->dlistid);
	}
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
}