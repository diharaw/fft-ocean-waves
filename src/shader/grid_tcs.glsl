layout(vertices = 3) out;

// ------------------------------------------------------------------
// INPUTS -----------------------------------------------------------
// ------------------------------------------------------------------

in vec3 TCS_IN_FragPos[];
in vec2 TCS_IN_TexCoord[];

// ------------------------------------------------------------------
// OUTPUTS ----------------------------------------------------------
// ------------------------------------------------------------------

out vec3 TES_IN_FragPos[];
out vec2 TES_IN_TexCoord[];

// ------------------------------------------------------------------
// UNIFORMS ---------------------------------------------------------
// ------------------------------------------------------------------

uniform vec3 u_CameraPos;

// ------------------------------------------------------------------
// FUNCTIONS --------------------------------------------------------
// ------------------------------------------------------------------

float tessellation_level(float d0, float d1)
{
    float avg_d = (d0 + d1) / 2.0;

    if (avg_d <= 10.0)
        return 20.0;
    else if (avg_d <= 20.0)
        return 10.0;
    else if (avg_d <= 30.0)
        return 5.0;
    else
        return 1.0;
}

// ------------------------------------------------------------------
// MAIN -------------------------------------------------------------
// ------------------------------------------------------------------

void main()
{
    TES_IN_FragPos[gl_InvocationID]  = TCS_IN_FragPos[gl_InvocationID];
    TES_IN_TexCoord[gl_InvocationID] = TCS_IN_TexCoord[gl_InvocationID];

    // Calculate distances to each vertex
    float eye_to_v0_dist = distance(u_CameraPos, TES_IN_FragPos[0]);
    float eye_to_v1_dist = distance(u_CameraPos, TES_IN_FragPos[1]);
    float eye_to_v2_dist = distance(u_CameraPos, TES_IN_FragPos[2]);

    // Calculate tessellation levels
    gl_TessLevelOuter[0] = tessellation_level(eye_to_v1_dist, eye_to_v2_dist);
    gl_TessLevelOuter[1] = tessellation_level(eye_to_v2_dist, eye_to_v0_dist);
    gl_TessLevelOuter[2] = tessellation_level(eye_to_v0_dist, eye_to_v1_dist);
    gl_TessLevelInner[0] = gl_TessLevelOuter[2];
}

// ------------------------------------------------------------------