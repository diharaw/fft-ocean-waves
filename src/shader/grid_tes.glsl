// ------------------------------------------------------------------
// INPUTS -----------------------------------------------------------
// ------------------------------------------------------------------

layout(triangles, equal_spacing, ccw) in;

in vec3 TES_IN_FragPos[];
in vec2 TES_IN_TexCoord[];

// ------------------------------------------------------------------
// OUTPUTS ----------------------------------------------------------
// ------------------------------------------------------------------

out vec3 FS_IN_FragPos;
out vec3 FS_IN_Normal;
out vec2 FS_IN_TexCoord;

// ------------------------------------------------------------------
// UNIFORMS ---------------------------------------------------------
// ------------------------------------------------------------------

layout(std140) uniform u_GlobalUBO
{
    mat4 view_proj;
};

uniform sampler2D s_Dy;
uniform sampler2D s_Dx;
uniform sampler2D s_Dz;
uniform sampler2D s_NormalMap;

uniform float u_DisplacementScale;
uniform float u_Choppiness;

// ------------------------------------------------------------------
// FUNCTIONS --------------------------------------------------------
// ------------------------------------------------------------------

vec2 interpolate_2d(vec2 v0, vec2 v1, vec2 v2)
{
    return vec2(gl_TessCoord.x) * v0 + vec2(gl_TessCoord.y) * v1 + vec2(gl_TessCoord.z) * v2;
}

// ------------------------------------------------------------------

vec3 interpolate_3d(vec3 v0, vec3 v1, vec3 v2)
{
    return vec3(gl_TessCoord.x) * v0 + vec3(gl_TessCoord.y) * v1 + vec3(gl_TessCoord.z) * v2;
}

// ------------------------------------------------------------------
// MAIN -------------------------------------------------------------
// ------------------------------------------------------------------

void main()
{
    FS_IN_FragPos  = interpolate_3d(TES_IN_FragPos[0], TES_IN_FragPos[1], TES_IN_FragPos[2]);
    FS_IN_TexCoord = interpolate_2d(TES_IN_TexCoord[0], TES_IN_TexCoord[1], TES_IN_TexCoord[2]);

    FS_IN_FragPos.y += texture(s_Dy, FS_IN_TexCoord).r * u_DisplacementScale;
    FS_IN_FragPos.x -= texture(s_Dx, FS_IN_TexCoord).r * u_Choppiness;
    FS_IN_FragPos.z -= texture(s_Dz, FS_IN_TexCoord).r * u_Choppiness;

    FS_IN_Normal = texture(s_NormalMap, FS_IN_TexCoord).xyz;

    gl_Position = view_proj * vec4(FS_IN_FragPos, 1.0);
}

// ------------------------------------------------------------------