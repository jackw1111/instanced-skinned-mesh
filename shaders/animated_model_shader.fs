#version 330 core

in vec2 TexCoord;
in vec3 FragPos;
in vec3 Normal;

float ambient = 0.4;
in float diffuse;

out vec4 FragColor;                                                              
                                                                                                        
uniform sampler2D gColorMap;
uniform sampler2D normalMap;

void main()
{    
	vec4 tex = texture(gColorMap, TexCoord);
    FragColor =  tex * (diffuse + ambient);
}
