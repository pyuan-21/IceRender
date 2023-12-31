#version 450 core

layout (location = 0) in vec3 vPos;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vUV;

uniform mat4 modelMat;
uniform mat4 viewMat;
uniform mat4 projectMat;

out vec3 fPos;

void main()
{
	fPos = vPos;
	gl_Position = projectMat*viewMat*modelMat*vec4(vPos, 1);
}