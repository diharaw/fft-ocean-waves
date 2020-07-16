#define LOCAL_SIZE 32
#define M_PI 3.1415926535897932384626433832795

// ------------------------------------------------------------------
// INPUTS -----------------------------------------------------------
// ------------------------------------------------------------------

layout(local_size_x = LOCAL_SIZE, local_size_y = LOCAL_SIZE, local_size_z = 1) in;

// ------------------------------------------------------------------
// UNIFORMS ---------------------------------------------------------
// ------------------------------------------------------------------

layout(binding = 0, rgba32f) writeonly uniform image2D normal_map;

uniform sampler2D s_HeightMap; 

uniform int u_N;

// ------------------------------------------------------------------
// MAIN -------------------------------------------------------------
// ------------------------------------------------------------------

void main()
{
    // z0 -- z1 -- z2
	// |	 |     |
	// z3 -- h  -- z4
	// |     |     |
	// z5 -- z6 -- z7
	
	ivec2 x = ivec2(gl_GlobalInvocationID.xy);
	vec2 tex_coord = gl_GlobalInvocationID.xy/float(u_N);
	
	float texelSize = 1.0/u_N;
	
	float z0 = texture(s_HeightMap, tex_coord + vec2(-texelSize, -texelSize)).r;
	float z1 = texture(s_HeightMap, tex_coord + vec2(0, -texelSize)).r;
	float z2 = texture(s_HeightMap, tex_coord + vec2(texelSize, -texelSize)).r;
	float z3 = texture(s_HeightMap, tex_coord + vec2(-texelSize, 0)).r;
	float z4 = texture(s_HeightMap, tex_coord + vec2(texelSize, 0)).r;
	float z5 = texture(s_HeightMap, tex_coord + vec2(-texelSize, texelSize)).r;
	float z6 = texture(s_HeightMap, tex_coord + vec2(0, texelSize)).r;
	float z7 = texture(s_HeightMap, tex_coord + vec2(texelSize, texelSize)).r;
	 
	vec3 normal;
	
	// Sobel Filter
	normal.z = 1.0;
	normal.x = z0 + 2 * z3 + z5 - z2 - 2 * z4 - z7;
	normal.y = z0 + 2 * z1 + z2 -z5 - 2 * z6 - z7;
	
	imageStore(normal_map, x, vec4(normalize(normal), 1));
}

// ------------------------------------------------------------------