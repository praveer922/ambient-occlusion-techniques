#pragma once
#include <cmath>
#include "SDL2/SDL.h"


namespace Util { 

    float degreesToRadians(float degrees) {
        return degrees * M_PI / 180.0;
    }

    cy::Matrix4f getOrthographicMatrix(float left, float right, float bottom, float top, float near, float far) {
            cy::Matrix4f proj = cy::Matrix4f(2.0f / (right - left), 0, 0, -(right + left) / (right - left),
            0, 2.0f / (top - bottom), 0, -(top + bottom) / (top - bottom),
            0, 0, -2.0f / (far - near),  -(far + near) / (far - near),
            0, 0, 0, 1);

            return proj;
    }

    static void load_tga(const char *filename, uint16_t *width, uint16_t *height, unsigned char **data) {
    
        SDL_RWops* file = SDL_RWFromFile(filename, "rb");
    
        if (file == NULL) {
            printf("Cannot open file %s", filename);
            exit(1);
        }
        
        char depth;
        SDL_RWseek(file, 12, SEEK_SET);
        SDL_RWread(file, width, sizeof(uint16_t), 1);
        SDL_RWseek(file, 14, SEEK_SET);
        SDL_RWread(file, height, sizeof(uint16_t), 1);
        SDL_RWseek(file, 16, SEEK_SET);
        SDL_RWread(file, &depth, sizeof(char), 1);
    
        if (depth != 32) {
            printf("tga file doesn't have four channels");
            exit(1);
        }
    
        *data = (unsigned char*)malloc(sizeof(unsigned char) * 4 * (*width) * (*height));
    
        SDL_RWseek(file, 18, SEEK_SET);
        SDL_RWread(file, *data, sizeof(unsigned char) * 4 * (*width) * (*height), 1);
        SDL_RWclose(file);
        
        for(int x = 0; x < *width; x++)
        for(int y = 0; y < *height; y++) {
            unsigned char t0, t1, t2, t3;
            t0 = (*data)[x * 4 + y * (*width) * 4 + 2]; t1 = (*data)[x * 4 + y * (*width) * 4 + 1];
            t2 = (*data)[x * 4 + y * (*width) * 4 + 0]; t3 = (*data)[x * 4 + y * (*width) * 4 + 3];
            (*data)[x * 4 + y * (*width) * 4 + 0] = t0; (*data)[x * 4 + y * (*width) * 4 + 1] = t1; 
            (*data)[x * 4 + y * (*width) * 4 + 2] = t2; (*data)[x * 4 + y * (*width) * 4 + 3] = t3;
        }
        
        for (int x = 0; x < *width;      x++)
        for (int y = 0; y < *height / 2; y++) {
            unsigned char t0, t1, t2, t3, b0, b1, b2, b3;
            t0 = (*data)[x * 4 + y * (*width) * 4 + 0]; t1 = (*data)[x * 4 + y * (*width) * 4 + 1];
            t2 = (*data)[x * 4 + y * (*width) * 4 + 2]; t3 = (*data)[x * 4 + y * (*width) * 4 + 3];
            b0 = (*data)[x * 4 + ((*height-1) - y) * (*width) * 4 + 0]; b1 = (*data)[x * 4 + ((*height-1) - y) * (*width) * 4 + 1];
            b2 = (*data)[x * 4 + ((*height-1) - y) * (*width) * 4 + 2]; b3 = (*data)[x * 4 + ((*height-1) - y) * (*width) * 4 + 3];
            (*data)[x * 4 + y * (*width) * 4 + 0] = b0; (*data)[x * 4 + y * (*width) * 4 + 1] = b1;
            (*data)[x * 4 + y * (*width) * 4 + 2] = b2; (*data)[x * 4 + y * (*width) * 4 + 3] = b3;
            (*data)[x * 4 + ((*height-1) - y) * (*width) * 4 + 0] = t0; (*data)[x * 4 + ((*height-1) - y) * (*width) * 4 + 1] = t1;
            (*data)[x * 4 + ((*height-1) - y) * (*width) * 4 + 2] = t2; (*data)[x * 4 + ((*height-1) - y) * (*width) * 4 + 3] = t3;
        }
    
    }

};