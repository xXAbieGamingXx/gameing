#
header: 
(uint16_t) area_id, (uint32_t) start_offset, end_offset (to the triangles in the level data)
end with a single 0x0

triangle information:
(all xyz float, all uv uint16_t) (x, y, z), (u, v), (x, y, z), (u, v) (x, y, z), (u, v), (uint16_t) texid, (uint32_t) lightmap_offset

uvs come from the .map face's texture params (offset, rotation, scale), divided by texture size
supports quake standard and valve 220 faces


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

#
current layout:
```
uint32_t vertex_count, tri_count, node_count
uint32_t indices[tri_count * 3]   (grouped by octree node)
struct vertex vertices[vertex_count]
struct octree_node nodes[node_count]   (see octree.h, node 0 is root)
```
each tri lives in the deepest node that fully holds it, so a node's tris are one contiguous index range

entities follow the world (all but worldspawn and func_group, which are static and go in the world):
```
uint32_t entity_count
per entity:
    uint16_t type                   (enum entity_type, see entities.h)
    int32_t params[entity_descs[type].params]   (int key values in .map order, classname and _ keys skipped)
    uint32_t vertex_count, tri_count
    uint32_t indices[tri_count * 3]
    struct vertex vertices[vertex_count]
```
unknown classnames fail the conversion
entity brushes skip planar culling since they move and can expose hidden faces
