#version 450 core

in vec3 fPos;
in vec3 fNormal;
in vec2 fUV;

/*material*/ 
struct Material
{
	vec3 ka, kd, ks, color; /*coefficient for ambient, diffuse, specular and color*/
	float shiness;
	sampler2D albedoTex;
};
uniform Material material; 

uniform int useAlbedoTex;

layout (location = 0) out vec3 posTex;
layout (location = 1) out vec4 normalTex;
layout (location = 2) out vec3 albedoTex;
layout (location = 3) out vec4 matTex;

void main()
{
	posTex = fPos;
	normalTex = vec4(fNormal, 1); // the w component stores fragment flag

	if(useAlbedoTex==1)
		albedoTex = texture(material.albedoTex, fUV).rgb;
	else
		albedoTex = material.color;

	vec3 invThree = vec3(1.0/3);
	matTex = vec4(dot(material.ka, invThree), dot(material.kd, invThree), dot(material.ks, invThree), material.shiness);
}