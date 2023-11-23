#version 450 core
#pragma optionNV (unroll all)

in vec3 fPos;
in vec3 fNormal;
in vec2 fUV;

/*matrix*/
uniform mat4 modelMat;
uniform mat4 viewMat;
uniform mat4 projectMat;

/*ambient*/
uniform vec3 ambientLight;

/*lights*/
uniform int activeLightNum;
const int maxLightNum = 5; /*TODO:don't forget to modify its corresponding variable in SceneManager*/
struct Light
{
	int type;
	vec3 color, pos, dir; /*color, position, direction*/
	float intensity;
	vec2 attenuation;

	/*shadow relevant*/
	int renderShadow;
};
uniform Light lights[maxLightNum];

uniform int useAlbedoTex;

/*material*/ 
struct Material
{
	vec3 ka, kd, ks, color; /*coefficient for ambient, diffuse, specular and color*/
	float shiness;
	sampler2D albedoTex;
};
uniform Material material; 

// import sub shader from other file
/*light info*/
struct LightCamInfo
{
	mat4 lightMat;
	vec3 lightViewDir;
};
uniform LightCamInfo lightCamInfos[maxLightNum]; // "maxLightNum" is defined in "phong_sm.main_fs"

/*shadow map*/
uniform sampler2D shadowMaps[maxLightNum]; // "maxLightNum" is defined in "phong_sm.main_fs"
uniform float bias;

/*PCF-percentage closer filtering*/
uniform int usePCF = 0; /*indicate whether use PCF to do filtering*/
uniform	int pcfHalfKernelSize;

float ComputeLightRatio(LightCamInfo lightCamInfo, sampler2D shadowMap)
{
	/*ComputeLightRatio: lightRatio is inside [0, 1]*/
	vec4 clipCoord = lightCamInfo.lightMat*modelMat*vec4(fPos,1); /*clip space*/
	vec3 ndcCoord = clipCoord.xyz / clipCoord.w; /*ndc space: [-1,1]^3*/ 
	vec3 shadowCoord = (ndcCoord+1)/2; /*map [-1,1]^3 to [0,1]^3*/
	float fragDepth = shadowCoord.z;

	vec2 texSize = textureSize(shadowMap, 0);
	vec2 texelSize = 1.0/texSize;

	float result = 0.0; // indicate how much lighting on this fragment

	if(usePCF==1)
	{
		/*produce soft-edge shadow by using PCF*/
		/*refer: "Rendering Antialiased Shadows with Depth Maps"(PCF) [Reeves et al. 1987]*/
		int totalNum = 0;
		for(int i = -pcfHalfKernelSize; i <= pcfHalfKernelSize; i++)
		{
			for(int j = -pcfHalfKernelSize; j <= pcfHalfKernelSize; j++)
			{
				float refDepth = texture(shadowMap, shadowCoord.xy+vec2(i,j)*texelSize).r;
				if(fragDepth<(refDepth+bias))
					result += 1.0;
				totalNum += 1;
			}
		}
		result /= totalNum;
	}
	else
	{
		// produce hard shadow
		float refDepth = texture(shadowMap, shadowCoord.xy).r; 
		if(fragDepth<(refDepth+bias))
			result = 1.0;
	}

	return result;
}

layout (location = 0) out vec4 resultTex;


void main()
{
	vec3 albedo;
	if(useAlbedoTex==1)
		albedo = texture(material.albedoTex, fUV).rgb;
	else
		albedo = material.color;

	vec3 ePos = (viewMat*modelMat*vec4(fPos, 1)).xyz; /*vertex position in eye space*/

	vec3 lRes = vec3(0); /*lighting response*/

	vec3 ambient = ambientLight*material.ka;

	for(int i=0;i<activeLightNum;i++)
	{
		/*lighting computation is in "eye space"*/
		Light light = lights[i];
		float lI = light.intensity;
		vec3 lPos = (viewMat*vec4(light.pos, 1)).xyz;
		vec3 lDir;
		if(light.type == 0)
		{
			/*point light*/ 
			lDir = lPos - ePos;
			float d = length(lDir);
			lI *= 1.0 / (1 + d*light.attenuation.x + pow(d,2)*light.attenuation.y);
		}
		else if(light.type == 1)
		{
			lDir = (viewMat*vec4(light.dir, 0)).xyz;
		}
		lDir = normalize(lDir);
		vec3 lC = light.color*lI; /*light color(or color intensity) at this vertex.*/
		
		/*refer: https://en.wikipedia.org/wiki/Phong_reflection_model, but I have my own modification*/

		vec3 eN = normalize((transpose(inverse(viewMat*modelMat))*vec4(fNormal, 0)).xyz);

		/*diffuse*/ 
		/*use 0 for normal's forth component in homogenous coordinate, cause translation should not be applied to normal vector*/
		vec3 diffuse = vec3(0);		
		float lDirDotN = dot(lDir,eN);
		if(lDirDotN > 0)
			/*albedo and incoming light control the color response at this vertex point*/
			diffuse = material.kd*lDirDotN*lC;

		/*specular*/
		vec3 specular = vec3(0);
		vec3 lRef = normalize(2*lDirDotN*eN-lDir); /*light reflection direction*/
		vec3 eV = normalize(-ePos); /*eye view direction at this vertex. In eye space, camera is at origin*/
		float lRefDotN = dot(lRef, eV);
		if(lRefDotN > 0)
			specular = material.ks*pow(lRefDotN, material.shiness)*lC;

		/*shadow is the result of light source(direct light), not ambient(indirect light)*/
		vec3 sum = diffuse + specular;
		float lightRatio = 1.0;
		if(light.renderShadow == 1)
		{
			// below code line("ComputeLightRatio") is defined in sub shader "ShadowMap/lightRatioPCF.sub_fs"
			lightRatio = ComputeLightRatio(lightCamInfos[i], shadowMaps[i]);
		}

		lRes += (sum*lightRatio);
	}

	lRes += ambient;
	lRes *= albedo; /*albedo determines how much lighting reflect from the surface*/
	resultTex = vec4(lRes, 1);
}
