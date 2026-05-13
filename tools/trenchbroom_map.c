#define VECMATH_GENERICS
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include "util/vecmath.h"

int main(int argc, char** argv){
	if(argc != 3){
		printf(".map to .lvl converter takes two args");
		return 1;
	}
	char* map_path = argv[1];
	char* lvl_path = argv[2];
	FILE *fmap;
	fmap = fopen(map_path, "r");
	FILE *flvl;
	flvl = fopen(lvl_path, "w");
	char line[100] = {0};
	bool in_brush = false;
	while(fgets(line, 100, fmap) != NULL){
		if(line[0] == '/' && line[1] == '/'){
			if(line[3] == 'b'){
				in_brush = true;
			}else{
				in_brush = false;
			}
		}else if(in_brush){
			if(line[0] == '('){
				float coords[9];
				uint16_t id = 0;
				uint8_t index = 1;
				uint16_t u[3];
				uint16_t v[3];
				for(int i = 0; i < 9; i++){
					index++;
					if(line[index] == ')'){
						index += 4;
					}
					char number[20] = {0};
					for(int j = 0; line[index] != ' '; j++){
						number[j] = line[index];
						index++;
					}
					coords[i] = strtof((const char*) number, NULL);
				}
				// TODO: make the tex id be based on the files in /textures in alphabetical order then modify the header
				// include the scaling factor based on the size of the file (resolution)
				index += 3;
				char str[20] = {0};
				for(int i = 0; line[index] != ' '; i++){
					str[i] = line[index];
					index++;
				}
				// TODO: check for any existing uv value for things that dont want pos defined textures
				if(str[3] == 'r'){ // remove this when updating this to actually do the thing (here for warnings :)
					str[3] = 'r';
				}

				vec3_t a = vec3(coords[0], coords[1], coords[2]);
				vec3_t b = vec3(coords[3], coords[4], coords[5]);
				vec3_t c = vec3(coords[6], coords[7], coords[8]);
				
				vec3_t n = vec3_normalize(vec3_cross(vec3_sub(b, a), vec3_sub(c, a)));

				vec3_t tmp = vec3_cross(n, vec3(-1, 0, 0));

				vec3_t v_vec = vec3_cross(n, vec3(1, 0, 0)); // may be the one with the u cross vec3(0, 0, 1) the closest to 0 
				vec3_t u_vec;

				if(vec3_dot(tmp, vec3(0, 0, 1)) > vec3_dot(v_vec, vec3(0, 0, 1))){
					v_vec = tmp;
				}

				tmp = vec3_cross(n, vec3(0, 1, 0));

				if(fabs(vec3_dot(tmp, vec3(0, 0, 1))) > vec3_dot(v_vec, vec3(0, 0, 1))){
					if(vec3_dot(tmp, vec3(0, 0, 1)) < 0){
						v_vec = vec3_cross(n, vec3(0, -1, 0));
					}else{
						v_vec = tmp;
					}
				}else if(vec3_dot(v_vec, vec3(0, 0, 1)) == 0){
					v_vec = vec3(0, 1, 0);
				}
				
				u_vec = vec3_normalize(vec3_cross(v_vec, n));
				v_vec = vec3_normalize(v_vec);

				u[0] = vec3_dot(u_vec, a) * 10000; // scale to make them fit in uint16 scale
				v[0] = vec3_dot(v_vec, a) * 10000;
				u[1] = vec3_dot(u_vec, b) * 10000;
				v[1] = vec3_dot(v_vec, b) * 10000;
				u[2] = vec3_dot(u_vec, c) * 10000;
				v[2] = vec3_dot(v_vec, c) * 10000;

				printf("(%i, %i), (%i, %i), (%i, %i)\n", u[0], v[0], u[1], v[1], u[2], v[2]);

				index++;
				fwrite(coords, sizeof(float), 3, flvl);
				fwrite(u, sizeof(uint16_t), 1, flvl);
				fwrite(v, sizeof(uint16_t), 1, flvl);
				fwrite(&coords[3], sizeof(float), 3, flvl);
				fwrite(&u[1], sizeof(uint16_t), 1, flvl);
				fwrite(&v[1], sizeof(uint16_t), 1, flvl);
				fwrite(&coords[6], sizeof(float), 3, flvl);
				fwrite(&u[2], sizeof(uint16_t), 1, flvl);
				fwrite(&v[2], sizeof(uint16_t), 1, flvl);
				fwrite(&id, sizeof(uint16_t), 1, flvl);
			}
		}
		memset(line, '0', 100);
	}
	fclose(fmap);
	fclose(flvl);
}
