#define LOCAL_SIZE 32
#define M_PI 3.1415926535897932384626433832795

// ------------------------------------------------------------------
// INPUTS -----------------------------------------------------------
// ------------------------------------------------------------------

layout (local_size_x = LOCAL_SIZE, local_size_y = LOCAL_SIZE, local_size_z = 1) in;

// ------------------------------------------------------------------
// UNIFORMS ---------------------------------------------------------
// ------------------------------------------------------------------

layout (binding = 0, r32f) writeonly uniform image2D displacement;
layout (binding = 1, rgba32f) readonly uniform image2D pingpong0;
layout (binding = 2, rgba32f) readonly uniform image2D pingpong1;

uniform int u_PingPong;
uniform int u_N;

// ------------------------------------------------------------------
// MAIN -------------------------------------------------------------
// ------------------------------------------------------------------

void main()
{
	ivec2 x = ivec2(gl_GlobalInvocationID.xy);
	
	float perms[] = {1.0,-1.0};
	int index = int(mod((int(x.x + x.y)),2));
	float perm = perms[index];
	
	if(u_PingPong == 0)
	{
		float h = imageLoad(pingpong0, x).r;
		imageStore(displacement, x, vec4(perm*(h/float(u_N*u_N)), 0, 0, 1));
	}
	else if(u_PingPong == 1)
	{
		float h = imageLoad(pingpong1, x).r;
		imageStore(displacement, x, vec4(perm*(h/float(u_N*u_N)), 0, 0, 1));
	}
}

// ------------------------------------------------------------------