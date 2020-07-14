#define LOCAL_SIZE 32
#define M_PI 3.1415926535897932384626433832795

// ------------------------------------------------------------------
// INPUTS -----------------------------------------------------------
// ------------------------------------------------------------------

layout (local_size_x = LOCAL_SIZE, local_size_y = LOCAL_SIZE, local_size_z = 1) in;

// ------------------------------------------------------------------
// STRUCTURES -------------------------------------------------------
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
	c.im = c0.real * c1.im + c0.im * c1.real;
	return c;
}

// ------------------------------------------------------------------

complex add(complex c0, complex c1)
{
	complex c;
	c.real = c0.real + c1.real;
	c.im = c0.im + c1.im;
	return c;
}

// ------------------------------------------------------------------

complex conjugate(complex c)
{
	complex c_conj;
	c_conj.real = c.real;
	c_conj.im = -c.im;
	return c;
}

// ------------------------------------------------------------------
// UNIFORMS ---------------------------------------------------------
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

	if (k_mag < 0.00001)
		k_mag = 0.00001;

	float w = sqrt(g * k_mag);

	vec2 h0k = imageLoad(tilde_h0k, ivec2(gl_GlobalInvocationID.xy)).xy;
	vec2 h0minusk = imageLoad(tilde_h0minusk, ivec2(gl_GlobalInvocationID.xy)).xy;

	complex fourier_amp;

	fourier_amp.real = h0k.x;
	fourier_amp.im = h0k.y;

	complex fourier_amp_conj;

	fourier_amp_conj.real = h0minusk.x;
	fourier_amp_conj.im = h0minusk.y;

	fourier_amp_conj = conjugate(fourier_amp_conj);

	float consinus = cos(w * u_Time);
	float sinus = sin(w * u_Time);

	complex exp_iwkt;

	exp_iwkt.real = consinus;
	exp_iwkt.im = sinus;

	complex exp_iwkt_minus;

	exp_iwkt_minus.real = consinus;
	exp_iwkt_minus.im = -sinus;

	// dy
	complex d_k_t_dy = add(mul(fourier_amp, exp_iwkt), mul(fourier_amp_conj, exp_iwkt_minus));

	// dx
	complex dx;

	dx.real = 0.0;
	dx.im = -k.x/k_mag;

	complex d_k_t_dx = mul(dx, d_k_t_dy);

	// dz
	complex dy;

	dy.real = 0.0;
	dy.im = -k.y/k_mag;

	complex d_k_t_dz = mul(dy, d_k_t_dy);

	imageStore(tilde_hkt_dx, ivec2(gl_GlobalInvocationID.xy), vec4(d_k_t_dx.real, d_k_t_dx.im, 0.0, 1.0));
	imageStore(tilde_hkt_dy, ivec2(gl_GlobalInvocationID.xy), vec4(d_k_t_dy.real, d_k_t_dy.im, 0.0, 1.0));
	imageStore(tilde_hkt_dz, ivec2(gl_GlobalInvocationID.xy), vec4(d_k_t_dz.real, d_k_t_dz.im, 0.0, 1.0));
}

// ------------------------------------------------------------------