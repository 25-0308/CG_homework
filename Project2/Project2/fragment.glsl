#version 330 core

in vec3 FragPos;
in vec3 Normal;
in vec3 lightPos;

out vec4 FragColor;  

uniform vec3 objectColor;
uniform vec3 lightColor;
uniform bool lightOn;
  
void main()
{
    vec3 ambientLight = vec3(0.3f);
    vec3 ambient = ambientLight * lightColor;

    vec3 normalVector = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diffuseLight = max(dot(normalVector, lightDir), 0.0);
    vec3 diffuse = diffuseLight * lightColor;
    
    vec3 result = ambient * objectColor; 
    result += diffuse * objectColor; 

    FragColor = vec4(result, 1.0f);
}
