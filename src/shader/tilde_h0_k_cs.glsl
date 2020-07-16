#define LOCAL_SIZE 32
#define M_PI 3.1415926535897932384626433832795

// ------------------------------------------------------------------
// INPUTS -----------------------------------------------------------
// ------------------------------------------------------------------

layout(local_size_x = LOCAL_SIZE, local_size_y = LOCAL_SIZE, local_size_z = 1) in;

// ------------------------------------------------------------------
// INPUT ------------------------------------------------------------
// ------------------------------------------------------------------

uniform sampler2D noise0;
uniform sampler2D noise1;
uniform sampler2D noise2;
uniform sampler2D noise3;

uniform float u_Amplitude;
uniform float u_WindSpeed;
uniform float u_SuppressFactor;
uniform vec2  u_WindDirection;
uniform int   u_N;
uniform int   u_L;

const float g = 9.81;

layout(binding = 0, rg32f) writeonly uniform image2D tilde_h0k;
layout(binding = 1, rg32f) writeonly uniform image2D tilde_h0minusk;

// ------------------------------------------------------------------
// FUNCTIONS --------------------------------------------------------
// ------------------------------------------------------------------

float suppression_factor(float k_mag_sqr)
{
    return exp(-k_mag_sqr * u_SuppressFactor * u_SuppressFactor);
}

// ------------------------------------------------------------------

float philips_power_spectrum(vec2 k, float k_mag, float k_mag_sqr, float L_philips, float suppression)
{
    return (u_Amplitude * (exp(-1.0 / (k_mag_sqr * L_philips * L_philips))) * pow(dot(normalize(k), u_WindDirection), 2.0) * suppression) / (k_mag_sqr * k_mag_sqr);
}

// ------------------------------------------------------------------

// Box-Muller-Method

vec4 gauss_rnd()
{
    vec2 texCoord = vec2(gl_GlobalInvocationID.xy) / float(u_N);

    float noise00 = clamp(texture(noise0, texCoord).r, 0.001, 1.0);
    float noise01 = clamp(texture(noise1, texCoord).r, 0.001, 1.0);
    float noise02 = clamp(texture(noise2, texCoord).r, 0.001, 1.0);
    float noise03 = clamp(texture(noise3, texCoord).r, 0.001, 1.0);

    float u0 = 2.0 * M_PI * noise00;
    float v0 = sqrt(-2.0 * log(noise01));
    float u1 = 2.0 * M_PI * noise02;
    float v1 = sqrt(-2.0 * log(noise03));

    vec4 rnd = vec4(v0 * cos(u0), v0 * sin(u0), v1 * cos(u1), v1 * sin(u1));

    return rnd;
}

// ------------------------------------------------------------------
// MAIN -------------------------------------------------------------
// ------------------------------------------------------------------

void main()
{
    vec2  x         = vec2(gl_GlobalInvocationID.xy) - float(u_N) / 2.0;
    vec2  k         = vec2((2.0 * M_PI * float(x.x)) / float(u_L), (2.0 * M_PI * float(x.y)) / float(u_L));
    float L_philips = (u_WindSpeed * u_WindSpeed) / g;
    float k_mag     = length(k);

    if (k_mag < 0.00001)
        k_mag = 0.00001;

    float k_mag_sqr   = k_mag * k_mag;
    float suppression = suppression_factor(k_mag_sqr);

    float h0k      = clamp(sqrt(philips_power_spectrum(k, k_mag, k_mag_sqr, L_philips, suppression)) / sqrt(2.0), -4000.0, 4000.0);
    float h0minusk = clamp(sqrt(philips_power_spectrum(-k, k_mag, k_mag_sqr, L_philips, suppression)) / sqrt(2.0), -4000.0, 4000.0);

    vec4 rnd = gauss_rnd();

    imageStore(tilde_h0k, ivec2(gl_GlobalInvocationID.xy), vec4(rnd.xy * h0k, 0.0, 1.0));
    imageStore(tilde_h0minusk, ivec2(gl_GlobalInvocationID.xy), vec4(rnd.zw * h0minusk, 0.0, 1.0));
}

// ------------------------------------------------------------------