#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>

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
	for(int i = 0; i < 4; i++){
		fgets(line, 100, fmap); // skip the spawn (fake brush)
	}
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
				int16_t u[] = {0, 0    , 32767};
				int16_t v[] = {0, 32767, 0    };
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
				index += 3;
				char str[20] = {0};
				for(int i = 0; line[index] != ' '; i++){
					str[i] = line[index];
					index++;
				}
				if(str[3] == 'r'){ // remove this when updating this to actually do the thing
					str[3] = 'r';
				}
				index++;
				fwrite(coords, sizeof(float), 3, flvl);
				fwrite(u, sizeof(int16_t), 1, flvl);
				fwrite(v, sizeof(int16_t), 1, flvl);
				fwrite(&coords[3], sizeof(float), 3, flvl);
				fwrite(&u[1], sizeof(int16_t), 1, flvl);
				fwrite(&v[1], sizeof(int16_t), 1, flvl);
				fwrite(&coords[6], sizeof(float), 3, flvl);
				fwrite(&u[2], sizeof(int16_t), 1, flvl);
				fwrite(&v[2], sizeof(int16_t), 1, flvl);
				fwrite(&id, sizeof(uint16_t), 1, flvl);
			}
		}
		memset(line, '0', 100);
	}
	fclose(fmap);
	fclose(flvl);
}
