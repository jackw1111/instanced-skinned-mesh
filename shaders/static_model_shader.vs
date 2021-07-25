#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 5) in mat4 instanceMatrix;

out vec3 FragPos;
out vec2 TexCoord;
out vec3 Normal;

out float diffuse;

uniform bool invertedNormals;

uniform mat4 view;
uniform mat4 projection;

struct Light {
    vec3 position;
    vec3 color;
};

uniform int no_of_lights; 
const int MAX_LIGHTS = 32;
uniform Light lights[MAX_LIGHTS];

void main()
{
    FragPos = vec3(view * instanceMatrix * vec4(aPos, 1.0));
    TexCoord = aTexCoords;
    
    mat3 normalMatrix = transpose(inverse(mat3(instanceMatrix)));
    Normal = normalize(normalMatrix * (invertedNormals ? -aNormal : aNormal));
    vec3 lightDir = normalize(lights[0].position - FragPos);  
    diffuse =  max(dot(Normal, lightDir), 0.0);

    gl_Position = projection * view * instanceMatrix * vec4(aPos, 1.0);
}

