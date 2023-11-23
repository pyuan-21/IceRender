#version 450 core

uniform vec3 albedoColor;

uniform mat4 modelMat;
uniform mat4 viewMat;
uniform mat4 projectMat;


uniform sampler2D opaqueDepthInfoTex;
const vec3 foamLineColor = vec3(0.90,0.98,1.00);
const float foamLineMaxDepth = 0.005; // linear depth for controlling the foam line width

in vec3 fPos; // unchange position in local space
in vec3 waterPos;

out vec4 colorResponse;

vec3 LocalPosToTexCoord(vec3 localPos)
{   
    /* localPos should be in local space */
	vec4 projectedPos = projectMat*viewMat*modelMat*vec4(localPos, 1);
	vec3 ndcPos = (projectedPos / projectedPos.w).xyz; // map to [-1, 1] -> NDC pos
	vec3 sampleTexCoord = (ndcPos + vec3(1)) / 2.0; // map to [0, 1] -> texCoord
	return sampleTexCoord;
}

// map to [0, 1]
// float NormalizeLinearDepth(float linearDepth)
// {
// 	float A = projectMat[2][2];
// 	float B = projectMat[3][2];
// 	float f = B / (A+1);
// 	float n = B / (A-1);
// 	return (linearDepth - n) / (f - n);
// }

// project depth back to linear depth [n, f]
// float ProjectedDepthToLinearDepth(float projectedDepth)
// {
// 	float A = projectMat[2][2];
// 	float B = projectMat[3][2];
// 	return B / (A + projectedDepth); // inside [n, f]
// }

void main()
{	
	vec3 waterColor = albedoColor;

	vec3 sampleTexCoord = LocalPosToTexCoord(fPos);
	vec2 opaqueDepthInfo = texture(opaqueDepthInfoTex, sampleTexCoord.xy).rg;
	if(opaqueDepthInfo.g == 1)
	{
		// has opaque vertices info
		// ProjectedDepthToLinearDepth
		float opaqueDepth = opaqueDepthInfo.r;
		float waterDepth = LocalPosToTexCoord(waterPos).z;
		float depthDiff = opaqueDepth - waterDepth;
		
		if(depthDiff >= 0)
		{
			float ratio = clamp((foamLineMaxDepth-depthDiff)/foamLineMaxDepth, 0, 1); // the larger depthDiff, the smaller ratio
			waterColor += (foamLineColor*ratio);
		}
	}

	colorResponse = vec4(waterColor, 0.85);
	// colorResponse = vec4(waterColor, 0.01);
}