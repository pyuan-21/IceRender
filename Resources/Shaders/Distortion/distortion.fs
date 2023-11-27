#version 450 core

in vec2 fUV;

uniform sampler2D colorTex;
uniform sampler2D noiseTex; // must be a nice noise textre for distortion.

uniform float time;


const int distortLen = 15; // it indicates the max texels-offset
const float distortSpeed = 1.0;

out vec4 colorResponse;

void main()
{
	vec4 sceneColor = texture(colorTex, fUV);

	vec2 texelSize = vec2(1.0) / textureSize(colorTex, 0); // size of one texel in the texture.
	float texMaxOffset = distortLen * length(texelSize); // how far the uv offset is. (uv here means a 2D point)

	float noiseX = texture(noiseTex, fUV+vec2(sin(distortSpeed*time), 0)).r;
	float noiseY = texture(noiseTex, fUV+vec2(0, cos(distortSpeed*time))).r;
	vec2 texOffset = vec2(noiseX, noiseY) * texMaxOffset;

	colorResponse = texture(colorTex, fUV + texOffset);
	// colorResponse = vec4(vec3(noise), 1.0);
}