out vec3 PS_OUT_Color;

in vec3 FS_IN_WorldPos;

uniform samplerCube s_Cubemap;

// ------------------------------------------------------------------
// MAIN -------------------------------------------------------------
// ------------------------------------------------------------------

void main()
{
    vec3 env_color = texture(s_Cubemap, FS_IN_WorldPos).rgb;

    // HDR tonemap and gamma correct
    env_color = env_color / (env_color + vec3(1.0));
    env_color = pow(env_color, vec3(1.0 / 2.2));

    PS_OUT_Color = env_color;
}

// ------------------------------------------------------------------