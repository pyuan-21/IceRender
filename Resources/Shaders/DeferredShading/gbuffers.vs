#version 450 core

layout (location = 0) in vec3 vPos;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vUV;

uniform mat4 viewMat;
uniform mat4 projectMat;
uniform mat4 modelMat;

out vec3 fPos;
out vec3 fNormal;
out vec2 fUV;

void main()
{
	gl_Position = projectMat*viewMat*modelMat*vec4(vPos, 1);
	fPos = (modelMat*vec4(vPos, 1)).xyz;

	// only ModelMatrix can perform a non-uniform scale on normal vector,
	// and only non-uniform scale (e.g. scaling (1,2,3)) can change the normal after applying model matrix
	// therefore storing the normal vector in world space is enough
	mat4 normalMat = transpose(inverse(modelMat));
	fNormal = (normalMat * vec4(normalize(vNormal), 0.0)).xyz;
	fNormal = normalize(fNormal);

	fUV = vUV;
}