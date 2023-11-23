#version 450 core

in vec3 fPos; // local space

uniform mat4 modelMat;
uniform mat4 viewMat;
uniform mat4 projectMat;

layout (location = 0) out vec2 opaqueInfo;

void main()
{
	vec4 projectedPos = projectMat*viewMat*modelMat*vec4(fPos, 1);
	vec3 ndcPos = (projectedPos / projectedPos.w).xyz; // map to [-1, 1] -> NDC pos
	vec3 sampleTexCoord = (ndcPos + vec3(1)) / 2.0; // map to [0, 1] -> texCoord(screen space)
	opaqueInfo = vec2(sampleTexCoord.z, 1);
}