#version 450 core

layout (location = 0) out vec2 waterDepthInfoTex;

uniform mat4 modelMat;
uniform mat4 viewMat;
uniform mat4 projectMat;

in vec3 waterPos;

void main()
{
	vec4 projectedPos = projectMat*viewMat*modelMat*vec4(waterPos, 1);
	vec3 ndcPos = (projectedPos / projectedPos.w).xyz; // map to [-1, 1] -> NDC pos
	vec3 sampleTexCoord = (ndcPos + vec3(1)) / 2.0; // map to [0, 1] -> texCoord(screen space)
	waterDepthInfoTex = vec2(sampleTexCoord.z, 1);

	gl_FragDepth = sampleTexCoord.z;
}