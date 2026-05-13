#
header: 
(uint16_t) area_id, (uint32_t) start_offset, end_offset (to the triangles in the level data)
end with a single 0x0

triangle information:
(all xyz float, all uv int16_t) (x, y, z), (u, v), (x, y, z), (u, v) (x, y, z), (u, v), (uint16_t) texid, (uint32_t) lightmap_offset

have uvs be based on position to create contiguous textures
ignore any existing uvs unless it is (1, 1), then DONT make the textures contiguous and make the vertices (0, 0), (1, 0), (0, 1)


>[!TODO]
> replace redundant vertices by adding an index buffer

> [!NOTE] Note
> stored in little endian when reading but still should work when reading

#
lightmap format (currently supports up to 256x256):

pixels are this format:
```c 
struct lightmap_pixel {
        unsigned char flag : 1;
        unsigned char pixel : 7;
};
``` 

the flag is used to skip empty space, the amount to skip is determined by the next line's pixel (and flag if need to support 256)
in all other cases, the other 7 bits determine change of the color shade of that pixel, 64 is no change

be sure to use this to make sure it wont flush the instructions:
```c 
#define likely(x) __builtin_expect(!!(x), 1) // makes the compiler make this branch more likely
#define unlikely(x) __builtin_expect(!!(x), 0) // makes the compiler make this branch less likely
``` 

each triangle lightmap ends with 0xFF
