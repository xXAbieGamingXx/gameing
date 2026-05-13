# Vertex-based world-space UV mapping

Per-face algorithm that emits seamless tiling UVs for adjacent coplanar
triangles. Implemented in `tools/trenchbroom_map.c` as `compute_face_basis`
plus `emit_poly_projected`.

## Steps

1. Normal from the two edges: `n = normalize(cross(b - a, c - a))`.
2. Pick a stable reference axis: world Z, or world X if `|n.z| > 0.9`
   (avoids degenerate cross product when the triangle is nearly horizontal).
3. In-plane basis:
   ```
   u_basis = normalize(cross(ref, n))
   v_basis = normalize(cross(n, u_basis))
   ```
4. Canonicalize sign: if `u_basis.x + u_basis.y + u_basis.z < 0`, negate both
   basis vectors. This keeps UVs from flipping between triangles wound in
   opposite directions on the same plane.
5. For each vertex: `u = dot(vertex, u_basis) * UV_TILE_SCALE`, same for v.
   Truncate to uint16 so the sampler's REPEAT wrap tiles the texture every
   64 world units (`UV_TILE_SCALE = 1024`, since uint16 covers 65536 ≈ 64·1024).

## Wrap-boundary splitting

Step 5 truncates to uint16, which wraps every 65536 scaled units. If a face is
wider than one tile (64 world units), its vertices land in different wrap
periods — e.g. one at u=64000, another at u=70000 stored as 4464. The GPU
then interpolates 64000 → 4464 linearly, sweeping backwards across the entire
texture instead of crossing the repeat seam once.

Before triangulating a face we clip its polygon against every wrap-boundary
plane `dot(p, u_basis) = K * 64` and `dot(p, v_basis) = K * 64`. Each sub-polygon
sits in exactly one `(tile_u, tile_v)` cell; its UVs are emitted relative to
that cell's origin and clamped to `[0, 65535]`. Vertices that fall on a
boundary get stored as `65535` on the lower side and `0` on the upper side; both
sample the same texel under `GL_REPEAT`, so the seam is invisible. This is the
`emit_poly_projected` / `split_poly_plane` pair.

## Why it's seamless

Two coplanar triangles share a normal → same basis → `dot(p, basis)` for any
shared vertex `p` is identical. Truncation to uint16 is also identical, so
shared vertices get bit-identical UVs and the GPU samples the same texel at
the seam.

Non-coplanar triangles (corners) have different normals and *will* show a
tiling discontinuity. That's correct — a corner is where two distinct tiled
planes meet.

## Special case

If the source `.map` face has UV offset `(1, 1)`, skip projection and emit
`(0,0), (1,0), (0,1)` (encoded as `0` / `65535`). See `file_format.md`.
