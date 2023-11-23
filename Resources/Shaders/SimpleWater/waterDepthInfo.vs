#version 450 core

layout (location = 0) in vec3 vPos;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vUV;

uniform mat4 modelMat;
uniform mat4 viewMat;
uniform mat4 projectMat;

uniform float time;
uniform sampler2D noiseTex;
const float waveAmplitude = 0.03;
const float waveSpeed = 0.007;
const float waveFrequence = 0.08; // the samller, the flatter.

out vec3 waterPos;

void main()
{
	waterPos = vPos;
	float temp = time*waveSpeed;
	float u = (sin(waveFrequence*vUV.x-temp) + 1) * 0.5; // map to [0, 1]
	float v = (cos(waveFrequence*vUV.y-temp) + 1) * 0.5;
	float noise = texture(noiseTex, vec2(u, v)).r; // value in range [0, 1]
	waterPos.y += mix(-waveAmplitude, +waveAmplitude, noise);
	
	gl_Position = projectMat*viewMat*modelMat*vec4(waterPos, 1);
}