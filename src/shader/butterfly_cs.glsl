#define LOCAL_SIZE 32
#define M_PI 3.1415926535897932384626433832795

// ------------------------------------------------------------------
// INPUTS -----------------------------------------------------------
// ------------------------------------------------------------------

layout (local_size_x = LOCAL_SIZE, local_size_y = LOCAL_SIZE, local_size_z = 1) in;

// ------------------------------------------------------------------
// UNIFORMS ---------------------------------------------------------
// ------------------------------------------------------------------

layout (binding = 0, rgba32f) readonly uniform image2D twiddle_factors;
layout (binding = 1, rgba32f) uniform image2D pingpong0;
layout (binding = 2, rgba32f) uniform image2D pingpong1;

uniform int u_PingPong;
uniform int u_Direction;
uniform int u_Stage;

// ------------------------------------------------------------------
// FUNCTION ---------------------------------------------------------
// ------------------------------------------------------------------

struct complex
{	
	float real;
	float im;
};

// ------------------------------------------------------------------

complex mul(complex c0, complex c1)
{
	complex c;
	c.real = c0.real * c1.real - c0.im * c1.im;
	c.im   = c0.real * c1.im + c0.im * c1.real;
	return c;
}

// ------------------------------------------------------------------

complex add(complex c0, complex c1)
{
	complex c;
	c.real = c0.real + c1.real;
	c.im   = c0.im   + c1.im;
	return c;
}

// ------------------------------------------------------------------

void horizontal_butterflies()
{
	complex H;
	ivec2 x = ivec2(gl_GlobalInvocationID.xy);
	
	if(u_PingPong == 0)
	{
		vec4 data = imageLoad(twiddle_factors, ivec2(u_Stage, x.x)).rgba;
		vec2 p_ = imageLoad(pingpong0, ivec2(data.z, x.y)).rg;
		vec2 q_ = imageLoad(pingpong0, ivec2(data.w, x.y)).rg;
		vec2 w_ = vec2(data.x, data.y);
		
		complex p = complex(p_.x,p_.y);
		complex q = complex(q_.x,q_.y);
		complex w = complex(w_.x,w_.y);
		
		//Butterfly operation
		H = add(p,mul(w,q));
		
		imageStore(pingpong1, x, vec4(H.real, H.im, 0, 1));
	}
	else if(u_PingPong == 1)
	{
		vec4 data = imageLoad(twiddle_factors, ivec2(u_Stage, x.x)).rgba;
		vec2 p_ = imageLoad(pingpong1, ivec2(data.z, x.y)).rg;
		vec2 q_ = imageLoad(pingpong1, ivec2(data.w, x.y)).rg;
		vec2 w_ = vec2(data.x, data.y);
		
		complex p = complex(p_.x,p_.y);
		complex q = complex(q_.x,q_.y);
		complex w = complex(w_.x,w_.y);
		
		//Butterfly operation
		H = add(p,mul(w,q));
		
		imageStore(pingpong0, x, vec4(H.real, H.im, 0, 1));
	}
}

// ------------------------------------------------------------------

void vertical_butterflies()
{
	complex H;
	ivec2 x = ivec2(gl_GlobalInvocationID.xy);
	
	if(u_PingPong == 0)
	{
		vec4 data = imageLoad(twiddle_factors, ivec2(u_Stage, x.y)).rgba;
		vec2 p_ = imageLoad(pingpong0, ivec2(x.x, data.z)).rg;
		vec2 q_ = imageLoad(pingpong0, ivec2(x.x, data.w)).rg;
		vec2 w_ = vec2(data.x, data.y);
		
		complex p = complex(p_.x,p_.y);
		complex q = complex(q_.x,q_.y);
		complex w = complex(w_.x,w_.y);
		
		//Butterfly operation
		H = add(p,mul(w,q));
		
		imageStore(pingpong1, x, vec4(H.real, H.im, 0, 1));
	}
	else if(u_PingPong == 1)
	{
		vec4 data = imageLoad(twiddle_factors, ivec2(u_Stage, x.y)).rgba;
		vec2 p_ = imageLoad(pingpong1, ivec2(x.x, data.z)).rg;
		vec2 q_ = imageLoad(pingpong1, ivec2(x.x, data.w)).rg;
		vec2 w_ = vec2(data.x, data.y);
		
		complex p = complex(p_.x,p_.y);
		complex q = complex(q_.x,q_.y);
		complex w = complex(w_.x,w_.y);
		
		//Butterfly operation
		H = add(p,mul(w,q));
		
		imageStore(pingpong0, x, vec4(H.real, H.im, 0, 1));
	}
}

// ------------------------------------------------------------------
// MAIN -------------------------------------------------------------
// ------------------------------------------------------------------

void main()
{
	if(u_Direction == 0)
		horizontal_butterflies();
	else if(u_Direction == 1)
		vertical_butterflies();
}

// ------------------------------------------------------------------