#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <float.h>
#include <GL/gl.h>
#include <GL/glext.h> // for mingw

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

typedef float		f32;
typedef s32		fx32;

#define FX32_SHIFT		12
#define FX_FX32_TO_F32(x)	((f32)((x) / (f32)(1 << FX32_SHIFT)))

enum CULL_MODE {
	DOUBLE_SIDED = 0,
	BACK_SIDE = 1,
	FRONT_SIDE = 2
};

enum GXPolygonMode {
	GX_POLYGONMODE_MODULATE = 0,
	GX_POLYGONMODE_DECAL = 1,
	GX_POLYGONMODE_TOON = 2,
	GX_POLYGONMODE_SHADOW = 3
};

enum REPEAT_MODE {
	CLAMP = 0,
	REPEAT = 1,
	MIRROR = 2
};

enum RENDER_MODE {
	NORMAL = 0,
	DECAL = 1,
	TRANSLUCENT = 2,
	// viewer only, not stored in a file
	ALPHA_TEST = 3
};

typedef struct {
	fx32		x;
	fx32		y;
	fx32		z;
} VecFx32;

typedef struct {
	fx32		m[4][3];
} MtxFx43;

typedef struct {
	u8		r;
	u8		g;
	u8		b;
} Color3;

typedef struct {
	u8		name[64];
	u8		light;
	u8		culling;
	u8		alpha;
	u8		wireframe;
	u16		palid;
	u16		texid;
	u8		x_repeat;
	u8		y_repeat;
	Color3		diffuse;
	Color3		ambient;
	Color3		specular;
	u8		field_53;
	u32		polygon_mode;
	u8		render_mode;
	u8		anim_flags;
	u16		field_5A;
	u32		texcoord_transform_mode;
	u16		texcoord_animation_id;
	u16		field_62;
	u32		matrix_id;
	u32		scale_s;
	u32		scale_t;
	u16		rot_z;
	u16		field_72;
	u32		translate_s;
	u32		translate_t;
	u16		material_animation_id;
	u16		field_7E;
	u8		packed_repeat_mode;
	u8		field_81;
	u16		field_82;
} Material;

typedef struct {
	u32		start_ofs;
	u32		size;
	s32		bounds[3][2];
} Dlist;

typedef struct {
	u8		name[64];
	// u32		dunno[104];
	u16		parent;
	u16		child;
	u16		next;
	u16		field_46;
	u32		enabled;
	u16		mesh_count;
	u16		mesh_id;
	u32		field_50;
	u32		field_54;
	u32		field_58;
	u16		field_5C;
	u16		field_5E;
	u16		field_60;
	u16		field_62;
	u32		field_64;
	u32		field_68;
	u32		field_6C;
	u32		field_70;
	VecFx32		vec1;
	VecFx32		vec2;
	u8		type;
	u8		field_8D;
	u16		field_8E;
	MtxFx43		a_matrix;
	u32		field_C0;
	u32		field_C4;
	u32		field_C8;
	u32		field_CC;
	u32		field_D0;
	u32		field_D4;
	u32		field_D8;
	u32		field_DC;
	u32		field_E0;
	u32		field_E4;
	u32		field_E8;
	u32		field_EC;
} Node;

typedef struct {
	u16		matid;
	u16		dlistid;
} Mesh;

typedef struct {
	u16		format;
	u16		width;
	u16		height;
	u16		pad;
	u32		image_ofs;
	u32		imagesize;
	u32		dunno1;
	u32		dunno2;
	u32		vram_offset;
	u32		opaque;
	u32		some_value;
	u8		packed_size;
	u8		native_texture_format;
	u16		texture_obj_ref;
} Texture;

typedef struct {
	u32		entries_ofs;
	u32		count;
	u32		dunno1;
	u32		some_reference;
} Palette;

typedef struct {
	u32		modelview_mtx_shamt;
	s32		scale;
	u32		unk3;
	u32		unk4;
	u32		materials;
	u32		dlists;
	u32		nodes;
	u16		unk_anim_count;
	u8		flags;
	u8		field_1F;
	u32		some_node_id;
	u32		meshes;
	u16		num_textures;
	u16		field_2A;
	u32		textures;
	u16		palette_count;
	u16		field_32;
	u32		palettes;
	u32		some_anim_counts;
	u32		unk8;
	u32		node_initial_pos;
	u32		node_pos;
	u16		num_materials;
	u16		num_nodes;
	u32		texture_matrices;
	u32		node_animation;
	u32		texcoord_animations;
	u32		material_animations;
	u32		texture_animations;
	u16		num_meshes;
	u16		num_matrices;
} __attribute__((__packed__)) HEADER;

static void update_bounds(SCENE* scene, float vtx_state[3])
{
	if(vtx_state[0] < scene->min_x) {
		scene->min_x = vtx_state[0];
	} else if(vtx_state[0] > scene->max_x) {
		scene->max_x = vtx_state[0];
	}
	if(vtx_state[1] < scene->min_y) {
		scene->min_y = vtx_state[1];
	} else if(vtx_state[1] > scene->max_y) {
		scene->max_y = vtx_state[1];
	}
	if(vtx_state[2] < scene->min_z) {
		scene->min_z = vtx_state[0];
	} else if(vtx_state[2] > scene->max_z) {
		scene->max_z = vtx_state[2];
	}
}

void do_reg(u32 reg, u32** data_pp, float vtx_state[3], SCENE* scene)
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

			update_bounds(scene, vtx_state);
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

			update_bounds(scene, vtx_state);
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

			update_bounds(scene, vtx_state);
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

			update_bounds(scene, vtx_state);
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

			update_bounds(scene, vtx_state);
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

			update_bounds(scene, vtx_state);
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
			printf("unhandled write to register 0x%x\n", 0x4000000 + reg);
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

void do_dlist(u32* data, u32 len, SCENE* scene)
{
	u32* end = data + len / 4;

	float vtx_state[3] = { 0.0f, 0.0f, 0.0f };

	while(data < end) {
		u32 regs = *(data++);

		u32 c;
		for(c = 0; c < 4; c++,regs >>= 8) {
			u32 reg = ((regs & 0xFF) << 2) + 0x400;

			do_reg(reg, &data, vtx_state, scene);
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

	scene->min_x = FLT_MAX;
	scene->min_y = FLT_MAX;
	scene->min_z = FLT_MAX;
	scene->max_x = -FLT_MAX;
	scene->max_y = -FLT_MAX;
	scene->max_z = -FLT_MAX;

	for(mesh = meshes, m = scene->meshes; mesh < end; mesh++, m++) {
		Dlist* dlist = &dlists[mesh->dlistid];
		u32* data = (u32*) (scenedata + dlist->start_ofs);
		m->matid = mesh->matid;
		m->dlistid = mesh->dlistid;
		if(scene->dlists[m->dlistid] != -1)
			continue;
		scene->dlists[m->dlistid] = glGenLists(1);
		glNewList(scene->dlists[m->dlistid], GL_COMPILE);
		do_dlist(data, dlist->size, scene);
		glEndList();
	}
}

/*

	Texture formats:

		0 = 2bit palettised
		1 = 4bit palettised
		2 = 8bit palettised
		4 = 8bit A5I3
		5 = 16bit RGBA
		6 = 8bit A3I5

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

		float alpha = mat->alpha / 31.0;

		if(tex->format == 0) {				// 2bit palettised
			u32 p;
			for(p = 0; p < num_pixels; p++) {
				u32 index = texels[p / 4];
				index = (index >> ((p % 4) * 2)) & 0x3;
				u16 col = get16bit_LE((u8*)&paxels[index]);
				u32 r = ((col >>  0) & 0x1F) << 3;
				u32 g = ((col >>  5) & 0x1F) << 3;
				u32 b = ((col >> 10) & 0x1F) << 3;
				u32 a = (tex->opaque ? 0xFF : (index == 0 ? 0x00 : 0xFF)) * alpha;
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
				u32 a = (tex->opaque ? 0xFF : (index == 0 ? 0x00 : 0xFF)) * alpha;
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
				u32 a = (tex->opaque ? 0xFF : (index == 0 ? 0x00 : 0xFF)) * alpha;
				image[p] = (r << 0) | (g << 8) | (b << 16) | (a << 24);
			}
		} else if(tex->format == 4) {			// A5I3
			u32 p;
			for(p = 0; p < num_pixels; p++) {
				u8 entry = texels[p];
				u8 i = (entry & 0x07);
				u16 col = get16bit_LE((u8*)&paxels[i]);
				u32 r = ((col >>  0) & 0x1F) << 3;
				u32 g = ((col >>  5) & 0x1F) << 3;
				u32 b = ((col >> 10) & 0x1F) << 3;
				u32 a = ((entry >> 3) / 31.0 * 255.0) * alpha;
				image[p] = (r << 0) | (g << 8) | (b << 16) | (a << 24);
			}
		} else if(tex->format == 5) {			// 16it RGB
			u32 p;
			for(p = 0; p < num_pixels; p++) {
				u16 col = (u16)texels[p * 2 + 0] | (((u16)texels[p * 2 + 1]) << 8);
				u32 r = ((col >>  0) & 0x1F) << 3;
				u32 g = ((col >>  5) & 0x1F) << 3;
				u32 b = ((col >> 10) & 0x1F) << 3;
				u32 a = ((col & 0x8000) ? 0x00 : 0xFF) * alpha;
				image[p] = (r << 0) | (g << 8) | (b << 16) | (a << 24);
			}
		} else if(tex->format == 6) {			// A3I5
			u32 p;
			for(p = 0; p < num_pixels; p++) {
				u8 entry = texels[p];
				u8 i = (entry & 0x1F);
				u16 col = get16bit_LE((u8*)&paxels[i]);
				u32 r = ((col >>  0) & 0x1F) << 3;
				u32 g = ((col >>  5) & 0x1F) << 3;
				u32 b = ((col >> 10) & 0x1F) << 3;
				u32 a = ((entry >> 5) / 7.0 * 255.0) * alpha;
				image[p] = (r << 0) | (g << 8) | (b << 16) | (a << 24);
			}
		} else {
			BREAKPOINT();
			print_once("Unhandled texture-format\n");
			memset(image, 0x7F, num_pixels * 4);
		}

		u32 p;
		switch(mat->polygon_mode) {
			case GX_POLYGONMODE_MODULATE:
				break;
			case GX_POLYGONMODE_DECAL:
				for(p = 0; p < num_pixels; p++) {
					u32 col = image[p];
					u32 r = (col >>  0) & 0xFF;
					u32 g = (col >>  8) & 0xFF;
					u32 b = (col >> 16) & 0xFF;
					u32 a = 0xFF * alpha;
					image[p] = (r << 0) | (g << 8) | (b << 16) | (a << 24);
				}
				break;
			case GX_POLYGONMODE_TOON:
				break;
			case GX_POLYGONMODE_SHADOW:
				break;
			default:
				printf("unknown alpha mode %d\n", mat->alpha);
				break;
		}

		int translucent = 0;
		for(p = 0; p < num_pixels; p++) {
			u32 col = image[p];
			u32 a = (col >> 24) & 0xFF;
			if(a != 0xFF) {
				translucent = 1;
				break;
			}
		}
		scn->materials[m].render_mode = mat->render_mode;
		if(mat->alpha < 31) {
			scn->materials[m].render_mode = TRANSLUCENT;
		}
		if(scn->materials[m].render_mode) {
			if(!translucent) {
				printf("%d [%s]: strange, this should be opaque (alpha: %d, fmt: %d, opaque: %d)\n", m, mat->name, mat->alpha, tex->format, tex->opaque);
				scn->materials[m].render_mode = NORMAL;
			}
		} else if(translucent) {
			printf("%d [%s]: strange, this should be translucent (alpha: %d, fmt: %d, opaque: %d)\n", m, mat->name, mat->alpha, tex->format, tex->opaque);
			scn->materials[m].render_mode = TRANSLUCENT;
		}

		if(scn->materials[m].render_mode == TRANSLUCENT) {
			if(alpha == 1.0f) {
				switch(tex->format) {
					case 0:
					case 1:
					case 2:
					case 5:
						printf("%d [%s]: using alpha test\n", m, mat->name);
						scn->materials[m].render_mode = ALPHA_TEST;
				}
			}
		}

		glGenTextures(1, &scn->materials[m].tex);
		glBindTexture(GL_TEXTURE_2D, scn->materials[m].tex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex->width, tex->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, (GLvoid*)image);
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		switch(mat->x_repeat) {
			case CLAMP:
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				break;
			case REPEAT:
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
				break;
			case MIRROR:
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
				break;
			default:
				printf("unknown repeat mode %d\n", mat->x_repeat);
		}
		switch(mat->y_repeat) {
			case CLAMP:
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				break;
			case REPEAT:
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
				break;
			case MIRROR:
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
				break;
			default:
				printf("unknown repeat mode %d\n", mat->x_repeat);
		}
		glBindTexture(GL_TEXTURE_2D, 0);
		free(image);
	}
}

static char* get_room_node_name(NODE* nodes, unsigned int node_cnt)
{
	char* name = "rmMain";
	if(node_cnt > 0) {
		NODE* node = nodes;
		int i = 0;
		while(node->name[0] != 'r' || node->name[1] != 'm') {
			node++;
			i++;
			if(i >= node_cnt) {
				return name;
			}
		}
		name = node->name;
	}
	return name;
}

static int get_node_child(const char* name, SCENE* scene)
{
	unsigned int i;
	if(scene->num_nodes <= 0)
		return -1;
	for(i = 0; i < scene->num_nodes; i++) {
		NODE* node = &scene->nodes[i];
		if(!strcmp(node->name, name))
			return node->child;
	}
	return -1;
}

void SCENE_filter_nodes(SCENE* scene, int layer_mask)
{
	unsigned int i;
	for(i = 0; i < scene->num_nodes; i++) {
		NODE* node = &scene->nodes[i];
		int flags = 0;
		if(strlen(node->name)) {
			unsigned int p;
			int keep = 0;
			for(p = 0; p < strlen(node->name); p += 4) {
				char* ch1 = &node->name[p];
				if(*ch1 != '_')
					break;
				if(*(u16*)ch1 == *(u16*)"_s") {
					int nr = node->name[p + 3] - '0' + 10 * (node->name[p + 2] - '0');
					if(nr)
						flags = flags & 0xC03F | ((((u32)flags << 18 >> 24) | (1 << nr)) << 6);
				}
				u32 tag = *(u32*) ch1;
				if(tag == *(u32*)"_ml0")
					flags |= LAYER_ML0;
				if(tag == *(u32*)"_ml1")
					flags |= LAYER_ML1;
				if(tag == *(u32*)"_mpu")
					flags |= LAYER_MPU;
				if(tag == *(u32*)"_ctf")
					flags |= LAYER_CTF;
			}

			if(!p || flags & layer_mask)
				keep = 1;
			if(!keep) {
				printf("filtering node '%s'\n", node->name);
				node->enabled = 0;
			}
		}
	}
}

SCENE* SCENE_load(u8* scenedata, unsigned int scenesize, u8* texturedata, unsigned int texturesize, int layer_mask)
{
	unsigned int i;

	SCENE* scene = (SCENE*) malloc(sizeof(SCENE));
	if(!scene)
		fatal("not enough memory");

	HEADER* rawheader = (HEADER*) scenedata;

	scene->scale		= get32bit_LE((u8*)&rawheader->scale) / 4096.0f;
	scene->scale		*= 1 << get32bit_LE((u8*)&rawheader->modelview_mtx_shamt);

	Material* materials	= (Material*)	((uintptr_t)scenedata + (uintptr_t)get32bit_LE((u8*)&rawheader->materials));
	Dlist* dlists		= (Dlist*)	((uintptr_t)scenedata + (uintptr_t)get32bit_LE((u8*)&rawheader->dlists));
	Node* nodes		= (Node*)	((uintptr_t)scenedata + (uintptr_t)get32bit_LE((u8*)&rawheader->nodes));
	Mesh* meshes		= (Mesh*)	((uintptr_t)scenedata + (uintptr_t)get32bit_LE((u8*)&rawheader->meshes));
	Texture* textures	= (Texture*)	((uintptr_t)scenedata + (uintptr_t)get32bit_LE((u8*)&rawheader->textures));
	Palette* palettes	= (Palette*)	((uintptr_t)scenedata + (uintptr_t)get32bit_LE((u8*)&rawheader->palettes));

	scene->num_textures	= get16bit_LE((u8*)&rawheader->num_textures);
	scene->num_materials	= get16bit_LE((u8*)&rawheader->num_materials);
	scene->num_nodes	= get16bit_LE((u8*)&rawheader->num_nodes);

	scene->materials = (MATERIAL*) malloc(scene->num_materials * sizeof(MATERIAL));
	scene->textures = (TEXTURE*) malloc(scene->num_textures * sizeof(TEXTURE));
	scene->nodes = (NODE*) malloc(scene->num_nodes * sizeof(NODE));

	for(i = 0; i < scene->num_nodes; i++) {
		NODE* node = &scene->nodes[i];
		Node* raw = &nodes[i];

		strncpy(node->name, raw->name, 64);
		node->parent = (s16) get16bit_LE((u8*)&raw->parent);
		node->child = (s16) get16bit_LE((u8*)&raw->child);
		node->next = (s16) get16bit_LE((u8*)&raw->next);
		node->mesh_count = get16bit_LE((u8*)&raw->mesh_count);
		node->mesh_id = (s16) get16bit_LE((u8*)&raw->mesh_id);
		node->enabled = get32bit_LE((u8*)&raw->enabled);
	}

	scene->room_node_name = get_room_node_name(scene->nodes, scene->num_nodes);
	scene->room_node_id = get_node_child(scene->room_node_name, scene);

	SCENE_filter_nodes(scene, layer_mask);

	printf("scale: %f\n", scene->scale);
	printf("room node: '%s' (%d)\n", scene->room_node_name, scene->room_node_id);

	if(!scene->materials || !scene->textures)
		fatal("not enough memory");

	for(i = 0; i < scene->num_materials; i++) {
		Material* m = &materials[i];
		m->palid = get16bit_LE((u8*)&m->palid);
		m->texid = get16bit_LE((u8*)&m->texid);

		MATERIAL* mat = &scene->materials[i];
		strcpy(mat->name, m->name);
		mat->texid = m->texid;
		mat->light = m->light;
		mat->culling = m->culling;
		mat->alpha = m->alpha;
		mat->x_repeat = m->x_repeat;
		mat->y_repeat = m->y_repeat;
		mat->polygon_mode = get32bit_LE((u8*)&m->polygon_mode);
		mat->scale_s = FX_FX32_TO_F32(get32bit_LE((u8*)&m->scale_s));
		mat->scale_t = FX_FX32_TO_F32(get32bit_LE((u8*)&m->scale_t));
		mat->translate_s = FX_FX32_TO_F32(get32bit_LE((u8*)&m->translate_s));
		mat->translate_t = FX_FX32_TO_F32(get32bit_LE((u8*)&m->translate_t));

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
	scene->num_dlists = 0;
	for(mesh = meshes; mesh < end; mesh++) {
		meshcount++;
		mesh->matid = get16bit_LE((u8*)&mesh->matid);
		mesh->dlistid = get16bit_LE((u8*)&mesh->dlistid);
		if(mesh->dlistid >= scene->num_dlists)
			scene->num_dlists = mesh->dlistid + 1;
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

	scene->dlists = (int*) malloc(scene->num_dlists * sizeof(int));
	for(i = 0; i < scene->num_dlists; i++)
		scene->dlists[i] = -1;

	make_textures(scene, materials, scene->num_materials, textures, scene->num_textures, palettes, texturedata, texturesize);

	i = 0;
	scene->meshes = (MESH*) malloc(sizeof(MESH));
	if(!scene->meshes)
		fatal("not enough memory");
	build_meshes(scene, meshes, dlists, scenedata, scenesize);

	return scene;
}

SCENE* SCENE_load_file(const char* model, const char* textures, int layer_mask)
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

	SCENE* scene = SCENE_load(content, scenesize, texturedata, texturesize, layer_mask);
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
	for(i = 0; i < scene->num_dlists; i++) {
		glDeleteLists(1, scene->dlists[i]);
	}
	free(scene->textures);
	free(scene->materials);
	free(scene->meshes);
	free(scene->dlists);
	free(scene->nodes);
	free(scene);
}

void SCENE_render_mesh(SCENE* scene, int mesh_id, int render_notex)
{
	if(mesh_id >= scene->num_meshes) {
		printf("trying to render mesh %d, but scene only has %d meshes\n", mesh_id, scene->num_meshes);
		return;
	}

	MESH* mesh = &scene->meshes[mesh_id];
	MATERIAL* material = &scene->materials[mesh->matid];
	TEXTURE* texture = &scene->textures[material->texid];

	if(material->texid != 0xFFFF) {
		glBindTexture(GL_TEXTURE_2D, material->tex);

		//Convert pixel coords to normalised STs
		glMatrixMode(GL_TEXTURE);
		glLoadIdentity();
		glTranslatef(material->translate_s, material->translate_t, 0.0f);
		glScalef(1.0f / texture->width, 1.0f / texture->height, 1.0f);
		glScalef(material->scale_s, material->scale_t, 1.0f);
	} else {
		glBindTexture(GL_TEXTURE_2D, 0);
		if(!render_notex)
			return;
	}

	switch(material->culling) {
		case DOUBLE_SIDED:
			glDisable(GL_CULL_FACE);
			break;
		case BACK_SIDE:
			glEnable(GL_CULL_FACE);
			glCullFace(GL_FRONT);
			break;
		case FRONT_SIDE:
			glEnable(GL_CULL_FACE);
			glCullFace(GL_BACK);
			break;
	}

	glCallList(scene->dlists[mesh->dlistid]);
}

void SCENE_render_all(SCENE* scene)
{
	// GLboolean cull;
	unsigned int i;
	// pass 1: opaque
	glDepthMask(GL_TRUE);
	for(i = 0; i < scene->num_meshes; i++) {
		MESH* mesh = &scene->meshes[i];
		MATERIAL* material = &scene->materials[mesh->matid];

		if(material->render_mode != NORMAL)
			continue;

		SCENE_render_mesh(scene, i, 1);
	}


	// pass 2: translucent
	// glGetBooleanv(GL_CULL_FACE, &cull);
	// glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);
	glDepthMask(GL_FALSE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	for(i = 0; i < scene->num_meshes; i++) {
		MESH* mesh = &scene->meshes[i];
		MATERIAL* material = &scene->materials[mesh->matid];

		if(material->render_mode == NORMAL)
			continue;

		SCENE_render_mesh(scene, i, 1);
	}
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
	// if(cull) {
	// 	glEnable(GL_CULL_FACE);
	// }
}

void SCENE_render_node_tree(SCENE* scene, int node_idx)
{
	unsigned int i;
	int start_node = node_idx;
	while(node_idx != -1) {
		NODE* node = &scene->nodes[node_idx];
		if(node->mesh_count > 0 && node->enabled == 1) {
			int mesh_id = node->mesh_id / 2;
			for(i = 0; i < node->mesh_count; i++)
				SCENE_render_mesh(scene, mesh_id + i, 0);
		}
		if(node->child != -1)
			SCENE_render_node_tree(scene, node->child);
		node_idx = node->next;
	}
}

void SCENE_render_all_nodes(SCENE* scene)
{
	unsigned int i, j;

	// pass 1: opaque
	glDepthMask(GL_TRUE);
	for(i = 0; i < scene->num_nodes; i++) {
		NODE* node = &scene->nodes[i];
		if(node->mesh_count > 0 && node->enabled == 1) {
			int mesh_id = node->mesh_id / 2;
			for(j = 0; j < node->mesh_count; j++) {
				int id = mesh_id + j;
				MESH* mesh = &scene->meshes[id];
				MATERIAL* material = &scene->materials[mesh->matid];

				if(material->render_mode != NORMAL)
					continue;

				SCENE_render_mesh(scene, id, 0);
			}
		}
	}

	// pass 2: decal
	glEnable(GL_POLYGON_OFFSET_FILL);
	glPolygonOffset(-1, -1);
	glDepthMask(GL_FALSE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	for(i = 0; i < scene->num_nodes; i++) {
		NODE* node = &scene->nodes[i];
		if(node->mesh_count > 0 && node->enabled == 1) {
			int mesh_id = node->mesh_id / 2;
			for(j = 0; j < node->mesh_count; j++) {
				int id = mesh_id + j;
				MESH* mesh = &scene->meshes[id];
				MATERIAL* material = &scene->materials[mesh->matid];

				if(material->render_mode != DECAL)
					continue;

				SCENE_render_mesh(scene, id, 0);
			}
		}
	}

	// pass 3: translucent with alpha test
	glEnable(GL_ALPHA_TEST);
	glDepthMask(GL_TRUE);
	glAlphaFunc(GL_GEQUAL, 0.5f);
	for(i = 0; i < scene->num_nodes; i++) {
		NODE* node = &scene->nodes[i];
		if(node->mesh_count > 0 && node->enabled == 1) {
			int mesh_id = node->mesh_id / 2;
			for(j = 0; j < node->mesh_count; j++) {
				int id = mesh_id + j;
				MESH* mesh = &scene->meshes[id];
				MATERIAL* material = &scene->materials[mesh->matid];
				TEXTURE* texture = &scene->textures[material->texid];

				if(material->render_mode != ALPHA_TEST)
					continue;

				SCENE_render_mesh(scene, id, 0);
			}
		}
	}
	glDisable(GL_ALPHA_TEST);

	// pass 4: translucent
	glDepthMask(GL_FALSE);
	for(i = 0; i < scene->num_nodes; i++) {
		NODE* node = &scene->nodes[i];
		if(node->mesh_count > 0 && node->enabled == 1) {
			int mesh_id = node->mesh_id / 2;
			for(j = 0; j < node->mesh_count; j++) {
				int id = mesh_id + j;
				MESH* mesh = &scene->meshes[id];
				MATERIAL* material = &scene->materials[mesh->matid];
				TEXTURE* texture = &scene->textures[material->texid];

				if(material->render_mode != TRANSLUCENT)
					continue;

				SCENE_render_mesh(scene, id, 0);
			}
		}
	}

	glPolygonOffset(0, 0);
	glDisable(GL_POLYGON_OFFSET_FILL);

	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
}

void SCENE_render(SCENE* scene)
{
	if(scene->room_node_id == -1)
		SCENE_render_all(scene);
	else
		SCENE_render_all_nodes(scene);
}
