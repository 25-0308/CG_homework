#version 330 core

layout(location = 0) in vec3 vPosition;
layout(location = 1) in vec3 vNormal;

out vec3 FragPos;
out vec3 Normal;
out vec3 lightPos;

uniform vec3 lightpos;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
	vec4 worldPos = model * vec4(vPosition, 1.0);
	gl_Position = projection * view * worldPos;

	FragPos = vec3(view * worldPos);

	mat3 normalMatrix = transpose(inverse(mat3(model)));
	Normal = normalize(normalMatrix * vNormal);

	lightPos = vec3(view * vec4(lightpos, 1.0));
}  