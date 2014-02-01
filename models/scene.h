#ifndef __SCENE_H__
#define __SCENE_H__

typedef struct {
	char				name[64];
	unsigned int			flags;
	unsigned int			texid;
	unsigned int			tex;
} MATERIAL;

typedef struct {
	char				name[64];
} NODE;

typedef struct {
	unsigned int			matid;
	unsigned int			dlistid;
	float				bounds[3][2];
} MESH;

typedef struct {
	unsigned int			width;
	unsigned int			height;
	bool				opaque;
} TEXTURE;

typedef struct {
	MATERIAL*			materials;
	NODE*				nodes;
	MESH*				meshes;
	unsigned int			num_meshes;
	TEXTURE*			textures;
	unsigned int			num_textures;
	unsigned int			num_materials;
} SCENE;

SCENE*	SCENE_load(u8* scenedata, unsigned int scenesize, u8* texturedata, unsigned int texturesize);
SCENE*	SCENE_load_file(const char* model, const char* textures);
void	SCENE_free(SCENE* scene);
void	SCENE_render(SCENE* scene);

#endif
