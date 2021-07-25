#version 330 core

in vec2 TexCoord;
in vec3 FragPos;
in vec3 Normal;



out vec4 FragColor;

uniform sampler2D texture_diffuse1;
uniform sampler2D shadowMap;

float ambient = 0.4;
in float diffuse;

void main()
{    
	vec4 tex = texture(texture_diffuse1, TexCoord);
    FragColor =  tex * (diffuse + vec4(ambient));
}