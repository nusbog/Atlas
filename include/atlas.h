#ifndef ATLAS_H
#define ATLAS_H

#include <stdbool.h>

typedef struct Texture Texture;

typedef struct {
	unsigned int w, h;
	unsigned char* data;

	int x_current_offset;
	int y_current_offset;

	int xspacing;
	int yspacing;

	bool minimize_texture_size;
	bool minimize_atlas_dimensions;
} Atlas;


struct Texture {
	unsigned char* data;

	int left, right, top, bottom;
	int size_bytes;

	unsigned int w_unminimized, h_unminimized;
	unsigned int w_minimized, h_minimized;

	// Location in atlas
	int x_atlas_loc;
	int y_atlas_loc;

	// Location in atlas mapped from 0.0 to 1.0
	float x_tex_coord;
	float y_tex_coord;
};



#endif /* ATLAS_H */