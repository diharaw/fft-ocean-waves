out vec4 FS_OUT_Color;

in vec3 FS_IN_FragPos;
in vec3 FS_IN_Normal;

uniform samplerCube s_Sky;
uniform vec3        u_CameraPos;
uniform vec3        u_SunDirection;
uniform vec3        u_SeaColor;

vec3 fresnel_schlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

void main()
{
    // Input lighting data
    vec3 N = FS_IN_Normal;
    vec3 V = normalize(u_CameraPos - FS_IN_FragPos);
    vec3 R = reflect(-V, N);
    vec3 L = -u_SunDirection;

    vec3 F0 = vec3(0.04);
    vec3 F  = fresnel_schlick(max(dot(N, V), 0.0), F0);

    vec3 albedo = mix(u_SeaColor, texture(s_Sky, R).rgb, F);

    float NdotL = max(dot(N, L), 0.0);
    vec3  color = NdotL * albedo;

    // HDR tonemapping
    color = color / (color + vec3(1.0));
    // gamma correct
    color = pow(color, vec3(1.0 / 2.2));

    FS_OUT_Color = vec4(color, 1.0);
}
