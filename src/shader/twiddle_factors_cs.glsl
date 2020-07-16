#define LOCAL_SIZE 32
#define M_PI 3.1415926535897932384626433832795

// ------------------------------------------------------------------
// INPUTS -----------------------------------------------------------
// ------------------------------------------------------------------

layout(local_size_x = 1, local_size_y = LOCAL_SIZE, local_size_z = 1) in;

// ------------------------------------------------------------------
// UNIFORMS ---------------------------------------------------------
// ------------------------------------------------------------------

layout(binding = 0, rgba32f) writeonly uniform image2D twiddle_factors;

layout(std430, binding = 0) buffer indices
{
    int j[];
}
bit_reversed;

struct complex
{
    float real;
    float im;
};

uniform int u_N;

// ------------------------------------------------------------------
// MAIN -------------------------------------------------------------
// ------------------------------------------------------------------

void main()
{
    vec2    x       = gl_GlobalInvocationID.xy;
    float   k       = mod(x.y * (float(u_N) / pow(2, x.x + 1)), u_N);
    complex twiddle = complex(cos(2.0 * M_PI * k / float(u_N)), sin(2.0 * M_PI * k / float(u_N)));

    int butterflyspan = int(pow(2, x.x));

    int butterflywing;

    if (mod(x.y, pow(2, x.x + 1)) < pow(2, x.x))
        butterflywing = 1;
    else
        butterflywing = 0;

    // first stage, bit reversed indices
    if (x.x == 0)
    {
        // top butterfly wing
        if (butterflywing == 1)
            imageStore(twiddle_factors, ivec2(x), vec4(twiddle.real, twiddle.im, bit_reversed.j[int(x.y)], bit_reversed.j[int(x.y + 1)]));
        // bot butterfly wing
        else
            imageStore(twiddle_factors, ivec2(x), vec4(twiddle.real, twiddle.im, bit_reversed.j[int(x.y - 1)], bit_reversed.j[int(x.y)]));
    }
    // second to log2(N) stage
    else
    {
        // top butterfly wing
        if (butterflywing == 1)
            imageStore(twiddle_factors, ivec2(x), vec4(twiddle.real, twiddle.im, x.y, x.y + butterflyspan));
        // bot butterfly wing
        else
            imageStore(twiddle_factors, ivec2(x), vec4(twiddle.real, twiddle.im, x.y - butterflyspan, x.y));
    }
}

// ------------------------------------------------------------------