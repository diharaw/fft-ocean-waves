layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texcoord;

uniform mat4 u_Projection;
uniform mat4 u_View;

out vec3 PS_IN_WorldPos;

void main(void)
{
    PS_IN_WorldPos = position;
    gl_Position    = u_Projection * u_View * vec4(position, 1.0f);
}
