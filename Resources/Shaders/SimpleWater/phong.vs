#version 450 core

layout (location = 0) in vec3 vPos;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vUV;

uniform mat4 modelMat;
uniform mat4 viewMat;
uniform mat4 projectMat;

uniform sampler2D waterDepthInfoTex;
uniform float time;
uniform sampler2D bumpTex;
const float distortLength = 0.03;
const float waveSpeed = 0.005;
const float waveFrequence = 0.08; // the samller, the flatter.

out vec3 fPos;
out vec3 fNormal;
out vec2 fUV;

void main()
{
	vec4 projectedPos = projectMat*viewMat*modelMat*vec4(vPos, 1);
	fPos = vPos;
	fNormal = vNormal;
	fUV = vUV;

	// Distortion:
	// before generating the fragments, do the distortion based on the vertices under water
	// (only those part which we can see from water surface, which means use water_uv to get the pos)

	vec3 ndcPos = (projectedPos / projectedPos.w).xyz; // map to [-1, 1] -> NDC pos
	vec3 sampleTexCoord = (ndcPos + vec3(1)) / 2.0; // map to [0, 1] -> texCoord(screen space)
	vec2 waterDepthInfo = texture(waterDepthInfoTex, sampleTexCoord.xy).rg;
	// waterDepthInfoTex: R is depth, G indicates whether we have vertex info at this fragment.
	if(waterDepthInfo.g == 1)
	{
		// within water plane
		float waterDepth = waterDepthInfo.r;
		float opaqueDepth = sampleTexCoord.z;
		if(waterDepth < opaqueDepth)
		{
			// behind water, then we can distort it
			float temp = time*waveSpeed;
			float u = (sin(waveFrequence*vUV.x-temp) + 1) * 0.5; // map to [0, 1]
			float v = (cos(waveFrequence*vUV.y-temp) + 1) * 0.5;
			vec3 noise = texture(bumpTex, vec2(u, v)).rgb; // value in range [0, 1]

			projectedPos.x += mix(-distortLength, +distortLength, noise.r);
			projectedPos.y += mix(-distortLength, +distortLength, noise.g);
		}
	}

	gl_Position = projectedPos;
}