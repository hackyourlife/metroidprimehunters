#ifndef __SCENE_H__
#define __SCENE_H__

typedef struct {
	char				name[64];
	unsigned char			light;
	unsigned char			culling;
	unsigned char			x_repeat;
	unsigned char			y_repeat;
	unsigned int			polygon_mode;
	float				scale_s;
	float				scale_t;
	unsigned int			alpha;
	unsigned int			texid;
	unsigned int			tex;
} MATERIAL;

typedef struct {
	char				name[64];
	unsigned int			parent;
	unsigned int			child;
	unsigned int			next;
	char*				some_name;
	unsigned int			mesh_count;
	unsigned int			mesh_id;
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
	int*				dlists;
	unsigned int			num_meshes;
	TEXTURE*			textures;
	unsigned int			num_textures;
	unsigned int			num_materials;
	unsigned int			num_dlists;
	unsigned int			num_nodes;
	float				scale;

	float				min_x;
	float				max_x;
	float				min_y;
	float				max_y;
	float				min_z;
	float				max_z;
	char*				room_node_name;
} SCENE;

SCENE*	SCENE_load(u8* scenedata, unsigned int scenesize, u8* texturedata, unsigned int texturesize);
SCENE*	SCENE_load_file(const char* model, const char* textures);
void	SCENE_free(SCENE* scene);
void	SCENE_render(SCENE* scene);

#endif
