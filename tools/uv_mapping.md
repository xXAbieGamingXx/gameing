triangle with vectors a, b, c

get direction vectors like this:

```c
vec3_t n = vec3_cross(vec3_sub(b, a), vec3_sub(c, a));

vec3_t v_vec = vec3_cross(n, vec3(1, 0, 0)); // actually the one with the u cross vec3(0, 0, 1) the closest to 0 
vec3_t u_vec;

if(vec3_dot(v_vec, vec3(0, 0, 1)) < 0){ // facing away from up
        u_vec = v_vec;
        v_vec = vec3_cross(n, u_vec);
}else{
        u_vec = vec3_cross(v, n_vec);
}

uint16_t u = vec3_dot(vertex, u_vec);
uint16_t v = vec3_dot(vertex, v_vec);
```

ensure that the u vector is going in a consistent direction so check dot with (1, 0, 0) and if it is 0 then check dot with (0, 1, 0) and negate them as needed
