#define LOCAL_SIZE 32
#define M_PI 3.1415926535897932384626433832795

// ------------------------------------------------------------------
// INPUTS -----------------------------------------------------------
// ------------------------------------------------------------------

layout (local_size_x = LOCAL_SIZE, local_size_y = LOCAL_SIZE, local_size_z = 1) in;

// ------------------------------------------------------------------
// INPUT ------------------------------------------------------------
// ------------------------------------------------------------------

uniform float u_Time;
uniform int u_N;
uniform int u_L;

const float g = 9.81;

layout (binding = 0, rg32f) readonly uniform image2D tilde_h0k;
layout (binding = 1, rg32f) readonly uniform image2D tilde_h0minusk;
layout (binding = 2, rg32f) writeonly uniform image2D tilde_hkt_dx;
layout (binding = 3, rg32f) writeonly uniform image2D tilde_hkt_dy;
layout (binding = 4, rg32f) writeonly uniform image2D tilde_hkt_dz;

// ------------------------------------------------------------------
// MAIN -------------------------------------------------------------
// ------------------------------------------------------------------

void main()
{
	vec2 x = vec2(gl_GlobalInvocationID.xy) - float(u_N)/2.0; 
	vec2 k = vec2((2.0 * M_PI * float(x.x)) / float(u_L), (2.0 * M_PI * float(x.y)) / float(u_L));
	float k_mag = length(k);



	imageStore(tilde_h0k, ivec2(gl_GlobalInvocationID.xy), vec4(rnd.xy * h0k, 0.0, 1.0));
	imageStore(tilde_h0minusk, ivec2(gl_GlobalInvocationID.xy), vec4(rnd.zw * h0minusk, 0.0, 1.0));
}

// ------------------------------------------------------------------