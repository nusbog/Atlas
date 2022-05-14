#include <stdlib.h>
#include <stdio.h>
#include <lodepng.h>
#include <atlas.h>
#include <stdbool.h>

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

// Contains pointers to all textures loaded
Texture **texture_ptrs;
// Contains pointers to all atlases loaded
Atlas **atlas_ptrs;

int texture_count = 0;
int atlas_count = 0;

// ATLAS
void highlightBackground() {

}

void createAtlas(Atlas *atlas, int width, int height, int xspacing, int yspacing, bool minimize_texture_size) {
    atlas->w = width;
    atlas->h = height;
    atlas->x_current_offset = 0;
    atlas->y_current_offset = 0;
    atlas->xspacing = xspacing;
    atlas->yspacing = yspacing;

    atlas->data = calloc(width * height * 4, 1);
    atlas->minimize_texture_size = minimize_texture_size;

    for(int i = 0; i < width * height * 4; i += 4) {
        atlas->data[i+0] = 255;
        atlas->data[i+1] = 000;
        atlas->data[i+2] = 000;
        atlas->data[i+3] = 255;
    }

    // Add this atlas to atlas_ptrs
    atlas_count++;
    atlas_ptrs = realloc(atlas_ptrs, atlas_count * sizeof(Atlas*));
    atlas_ptrs[atlas_count-1] = atlas;
}

void appendTextureToAtlas(Atlas *atlas, Texture *src) {
    // Where the writing should start
    int y_start_write = atlas->minimize_texture_size ? src->top  : 0;
    int x_start_write = atlas->minimize_texture_size ? src->left : 0;

    int src_minimized_width = (src->right - src->left) * 4;

    // If the source texture's width would be out of bounds in the atlas
    if(atlas->x_current_offset + src_minimized_width > atlas->w * 4) {
        atlas->x_current_offset = 0;
        // TODO: Change from texture_ptrs[0]->minimized to the first texture in the previous rows height (minimized)
        atlas->y_current_offset += (atlas->w * (texture_ptrs[0]->h_minimized + atlas->yspacing) * 4);
    }

    if((atlas->y_current_offset + src_minimized_width * 64 * 4) > atlas->w * atlas->h * 4) {
        printf("New atlas required!\n");
        return;
    }

    for(int i = y_start_write; i < src->bottom; i++) {
        // Start of the current row in the new texture
        int dest_write_offset = i * atlas->w * 4;
        dest_write_offset -= y_start_write * atlas->w * 4;
        dest_write_offset += atlas->x_current_offset;
        dest_write_offset += atlas->y_current_offset;

        // Start of the current row in the source texture
        int src_read_offset = i * src->w_unminimized * 4;
        int src_read_amount = src_minimized_width;

        src_read_offset += x_start_write * 4;

        memcpy(atlas->data + dest_write_offset, src->data + src_read_offset, src_read_amount);
    }

    atlas->x_current_offset += src_minimized_width + atlas->xspacing * 4;
}

void saveAtlasToFile(char *filename, Atlas *atlas) {
    lodepng_encode32_file(filename, atlas->data, atlas->w, atlas->h);
}

void clearAtlasImageData() {
    for(int i = 0; i < atlas_count; i++) {
        free(atlas_ptrs[i]->data);
    }
}

void clearAtlasData() {
    free(atlas_ptrs);
}

// TEXTURES
// Creates multiple textures from one texture sheet
Texture *splitTexture(Texture *src, int slices) {
    Texture *desttexture = malloc(slices * sizeof(Texture));

    texture_count += slices;
    texture_ptrs = realloc(texture_ptrs, texture_count * sizeof(Texture*));

    // Each new texture's width
    int splitwidth = src->w_unminimized / slices; // TODO: Check if decimal

    // For each destination texture
    for(int i = 0; i < slices; i++) {
        Texture *dest = &desttexture[i];
        dest->data = malloc(splitwidth * src->h_unminimized * 4);
        dest->w_unminimized = splitwidth;
        dest->h_unminimized = src->h_unminimized;

        // For each row in the destination texture
        for(int j = 0; j < src->h_unminimized; j++) {
            // Start of the current row in the new texture
            int destwriteoffset = j * splitwidth * 4;

            // Start of the current row in the source texture
            int srcreadoffset = j * src->w_unminimized * 4 + splitwidth * 4 * i;
            int srcreadamount = splitwidth * 4;

            // Copy the current row from the source texture into the current row of the destination texture
            memcpy(dest->data + destwriteoffset, src->data + srcreadoffset, srcreadamount);
        }

        texture_ptrs[texture_count - slices + i] = dest;
    }

    // Free the source memory as we've assigned all the data to new textures
    free(src->data);
    return desttexture;
}

void minimizeTextureSize(Texture *texture) {
    texture->left = texture->w_unminimized;
    texture->right = 0;
    texture->top  = texture->h_unminimized;
    texture->bottom  = 0;

    // For each pixel in texture
    for(int y = 0; y < texture->h_unminimized; y++) {
        for(int x = 0; x < texture->w_unminimized; x++) {
            int index = x * 4 + (y * texture->w_unminimized) * 4;
            unsigned char pixel[4] = { 
                (unsigned char)texture->data[index+0], // R
                (unsigned char)texture->data[index+1], // G
                (unsigned char)texture->data[index+2], // B
                (unsigned char)texture->data[index+3], // A
            };
            // If pixel is not transparent
            if(pixel[3] != 0) {
                texture->left   = MIN(texture->left,   x);
                texture->right  = MAX(texture->right,  x);
                texture->top    = MIN(texture->top,    y);
                texture->bottom = MAX(texture->bottom, y);
            }
        }
    }

    texture->right++;
    texture->bottom++;

    texture->w_minimized = (texture->right - texture->left);
    texture->h_minimized = (texture->bottom - texture->top);
    texture->size_bytes = 4 * texture->w_minimized * texture->h_minimized;
}

int compareTextureSizes(const void *t1, const void *t2)
{
    Texture *e1 = *(Texture **)t1;
    Texture *e2 = *(Texture **)t2;
    return e2->size_bytes - e1->size_bytes;
}

void sortTextures() {
    qsort(texture_ptrs, texture_count, sizeof(Texture*), compareTextureSizes);
}

Texture *loadTexture(char *filename, int slices) {
    Texture src_texture;
    int success = lodepng_decode32_file(&src_texture.data, &src_texture.w_unminimized, &src_texture.h_unminimized, filename);
    if(success != 0) {
        printf("%d\n", success);
        return NULL;
    }

    Texture *textures;
    textures = splitTexture(&src_texture, slices);
    for(int i = 0; i < slices; i++) {
        minimizeTextureSize(&textures[i]);
    }

    return textures;
}