#
header: 
(uint16_t) area_id, (uint32_t) start_offset, end_offset (to the triangles in the level data)
end with a single 0x0

vertex: float x, y, z, u, v, atlas, rect[4], lm[2]   (lm is lightmap uv over the whole sheet)

uvs come from the .map face's texture params (offset, rotation, scale), divided by texture size
supports quake standard and valve 220 faces


> [!NOTE] Note
> stored in little endian when reading but still should work when reading

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
    float origin[3]                 ("origin" key, 0 0 0 if unset)
    int32_t params[entity_descs[type].params]   (int key values in .map order, classname, origin and _ keys skipped)
    uint32_t vertex_count, tri_count
    uint32_t indices[tri_count * 3]
    struct vertex vertices[vertex_count]
```
unknown classnames fail the conversion
`light` entities are baked and dropped, never written

lightmap follows the entities:
```
uint16_t lm_w, lm_h
uint8_t rgb[lm_w * lm_h * 3]
```
light is stored at half, so 255 is 2x albedo (overbright). game pads to rgba8 on load
one rect per brush face, 16 units per luxel (coarser for faces past 128 luxels), 1 luxel border
direct light only: worldspawn `_ambient`, sun (`_sun_angle`, `_sun_color`, `_sun_light`), and `light` entities (`light` radius, `_color`)
shadow rays hit world tris only; brush entities are baked where they spawn
entity brushes skip planar culling since they move and can expose hidden faces
