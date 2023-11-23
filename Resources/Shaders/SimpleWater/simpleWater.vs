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

out vec3 fPos; // for converting to clip space then use xy as uv to sample screen space.
out vec3 waterPos; // for computing the real water depth

void main()
{
	/*
	* Let's focus on one dimension first: X axis.(u direction). 
	* We can use a noise texture as a Perlin Noise function F(u, v, t), where u,v are the texture coordinate, t is time.
	* Sampling it will return a value(Perlin Noise function return value) which can be used for water height offset.
	* We know that, when sampling noise texture, we need uv which means Tex(u, v) only takes two paramters.
	* We can consider it as Tex(ut, vt) is the value of F(ut, vt, t) at time=t. 
	* Therefore, we can use ut=u-L*t where L is wave speed, same as v. (minus it propagates along with +u direction)
	* Focus on u=0,v=0, as the t changes, F(ut, vt, t) will follow the shape of perlin noise which look like a wave.
	*/

	fPos = vPos;
	waterPos = vPos;
	float temp = time*waveSpeed;
	float u = (sin(waveFrequence*vUV.x-temp) + 1) * 0.5; // map to [0, 1]
	float v = (cos(waveFrequence*vUV.y-temp) + 1) * 0.5;
	float noise = texture(noiseTex, vec2(u, v)).r; // value in range [0, 1]
	waterPos.y += mix(-waveAmplitude, +waveAmplitude, noise);
	
	gl_Position = projectMat*viewMat*modelMat*vec4(waterPos, 1);
}