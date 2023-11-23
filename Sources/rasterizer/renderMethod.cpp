#include "renderMethod.hpp"
#include "../globals.hpp"
#include "../light/pointLight.hpp"
#include "../light/directLight.hpp"
#include "../material/phongMaterial.hpp"
#include "../helpers/utility.hpp"
#include "../mesh/meshGenerator.hpp"


using namespace IceRender;

void RasterizerRender::NoRender() {/*do nothing*/ };

void RasterizerRender::RenderSimple()
{
	glClearColor(0.67f, 0.84f, 0.90f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Erase the color and z buffers.
	glm::mat4 viewMat = GLOBAL.camCtrller->GetActiveCamera()->GetViewMatrix();
	glm::mat4 projectMat = GLOBAL.camCtrller->GetActiveCamera()->GetProjectionMatrix();
	// try to get "Simple/simple" shader pro, if not exist, create/activate it and return it
	shared_ptr<ShaderProgram> shaderPro = GLOBAL.shaderMgr->TryActivateShaderProgram(GLOBAL.shaderPathPrefix + "Simple/simple");
	shaderPro->Set("viewMat", viewMat);
	shaderPro->Set("projectMat", projectMat);
	auto sceneObjs = GLOBAL.sceneMgr->GetAllSceneObject(); // not copy data, just return reference &
	for (auto iter = sceneObjs.begin(); iter != sceneObjs.end(); iter++)
	{
		auto sceneObj = *iter;
		glm::mat4 modelMat = sceneObj->GetTransform()->ComputeTransformationMatrix();
		shaderPro->Set("modelMat", modelMat);

		auto material = sceneObj->GetMaterial();
		if (material && material->GetUVDataSize() > 0 && material->GetAlbedo() != 0)
		{
			// OpenGL 4.5 way to use texture:
			GLuint texUnit = 0;
			glBindTextureUnit(texUnit, material->GetAlbedo()); // this function to bind texture object to sampler2D variable with "binding=0"
			//glActiveTexture(GL_TEXTURE0 + texUnit); // no need actually for now
			shaderPro->Set("useAlbedoTex", 1);
		}
		else if(material)
		{
			shaderPro->Set("useAlbedoTex", 0);
			shaderPro->Set("albedoColor", material->GetColor());
		}
		else
			shaderPro->Set("useAlbedoTex", 0);

		GLOBAL.render->Draw(sceneObj);
	}
}

void RasterizerRender::RenderPhong()
{
	glViewport(0, 0, GLOBAL.WIN_WIDTH, GLOBAL.WIN_HEIGHT); // set it back to normal
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClearColor(0.67f, 0.84f, 0.90f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Erase the color and z buffers.

	GLuint texUnit = 0; // each shader start with texture0

	glm::mat4 viewMat = GLOBAL.camCtrller->GetActiveCamera()->GetViewMatrix();
	glm::mat4 projectMat = GLOBAL.camCtrller->GetActiveCamera()->GetProjectionMatrix();
	shared_ptr<ShaderProgram> shaderPro = GLOBAL.shaderMgr->TryActivateShaderProgram(GLOBAL.shaderPathPrefix + "Phong/phong");
	shaderPro->Set("viewMat", viewMat);
	shaderPro->Set("projectMat", projectMat);

	bool needShadowRender = GLOBAL.shadowMgr->IsNeedShadowRender();
	shared_ptr<BasicShadowMapRender> basicShadowMapRender;
	if (needShadowRender)
		basicShadowMapRender = dynamic_pointer_cast<BasicShadowMapRender>(GLOBAL.shadowMgr->GetShadowRender());

	// pass light information to current shader
	shaderPro->Set("ambientLight", GLOBAL.sceneMgr->GetAmbient());
	auto lights = GLOBAL.sceneMgr->GetAllLight();
	shaderPro->Set("activeLightNum", static_cast<int>(lights.size()));
	for (size_t i = 0; i < lights.size(); i++)
	{
		shaderPro->Set("lights[" + std::to_string(i) + "].type", static_cast<int>(lights[i]->GetType()));
		shaderPro->Set("lights[" + std::to_string(i) + "].color", lights[i]->GetColor());
		shaderPro->Set("lights[" + std::to_string(i) + "].pos", lights[i]->GetTransform()->GetPosition());
		shaderPro->Set("lights[" + std::to_string(i) + "].intensity", lights[i]->GetIntensity());

		if (needShadowRender && lights[i]->IsRenderShadow())
			shaderPro->Set("lights[" + std::to_string(i) + "].renderShadow", 1);
		else
			shaderPro->Set("lights[" + std::to_string(i) + "].renderShadow", 0);

		if (lights[i]->GetType() == LightType::POINT)
		{
			shared_ptr<PointLight> pointLight = static_pointer_cast<PointLight>(lights[i]);
			shaderPro->Set("lights[" + std::to_string(i) + "].attenuation", pointLight->GetAttenuation());
		}
		else if (lights[i]->GetType() == LightType::DIRECT)
		{
			shared_ptr<DirectLight> directLight = static_pointer_cast<DirectLight>(lights[i]);
			shaderPro->Set("lights[" + std::to_string(i) + "].dir", -directLight->GetDirection()); // shader is using the direction from fragment to light source, then here we should pass -direction.
		}
	}

	// allow different shadow tecnique to use different light ratio sub shader.(pass parameters to final shader)
	// [Be careful] must pass texUnit into this function in order to bind the correct texture location in shader.
	if (needShadowRender)
		basicShadowMapRender->InitComputeLightRatioParameters(shaderPro, texUnit);

	auto sceneObjs = GLOBAL.sceneMgr->GetAllSceneObject(); // not copy data, just return reference &
	for (auto iter = sceneObjs.begin(); iter != sceneObjs.end(); iter++)
	{
		auto sceneObj = *iter;
		glm::mat4 modelMat = sceneObj->GetTransform()->ComputeTransformationMatrix();
		shaderPro->Set("modelMat", modelMat);
		// pass material information to shader
		auto material = static_pointer_cast<PhongMaterial>(sceneObj->GetMaterial());
		shaderPro->Set("material.ka", material->GetAmbientCoef());
		shaderPro->Set("material.kd", material->GetDiffuseCoef());
		shaderPro->Set("material.ks", material->GetSpecularCoef());
		shaderPro->Set("material.shiness", material->GetShiness());
		if (material && material->GetUVDataSize() > 0 && material->GetAlbedo() != 0)
		{
			// OpenGL 4.5 way to use texture:
			// refer: https://www.khronos.org/opengl/wiki/Example_Code
			// TODO: but not sure whether I am doing right or not. Finish reading OpenGL book later then back to here.
			shaderPro->Set("albedoTex", static_cast<int>(texUnit));
			glBindTextureUnit(texUnit++, material->GetAlbedo()); // this function to bind texture object to sampler2D variable with "binding=texUnit"
			shaderPro->Set("useAlbedoTex", 1);
		}
		else
		{
			shaderPro->Set("useAlbedoTex", 0);
			shaderPro->Set("material.color", material->GetColor());
		}
		GLOBAL.render->Draw(sceneObj);
	}
}

void RasterizerRender::RenderSceenQuad()
{
	glClearColor(0.0f, 0.0f, 0.0f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Erase the color and z buffers.
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	shared_ptr<ShaderProgram> shaderPro = GLOBAL.shaderMgr->TryActivateShaderProgram(GLOBAL.shaderPathPrefix + "ScreenQuad/screenQuad");
	auto screenQuadObj = GLOBAL.sceneMgr->GetSceneObj("screen_quad");
	auto material = screenQuadObj->GetMaterial();
	GLuint texID = material->GetAlbedo();
	if (material && material->GetUVDataSize() > 0 && glIsTexture(texID))
	{
		/*below code can render the original size texture*/
		//int width, height;
		//glGetTextureLevelParameteriv(texID, 0, GL_TEXTURE_WIDTH, &width);
		//glGetTextureLevelParameteriv(texID, 0, GL_TEXTURE_HEIGHT, &height);
		//glViewport(0, 0, width, height); // set it back to normal
		// OpenGL 4.5 way to use texture:
		shaderPro->Set("useAlbedoTex", 1);
		GLuint texUnit = 0;
		glBindTextureUnit(texUnit, texID);
		shaderPro->Set("albedoTex", static_cast<int>(texUnit));
	}
	else
	{
		shaderPro->Set("useAlbedoTex", 0);
		shaderPro->Set("albedoColor", material->GetColor());
	}
	GLOBAL.render->Draw(screenQuadObj);
	glViewport(0, 0, GLOBAL.WIN_WIDTH, GLOBAL.WIN_HEIGHT); // set it back to normal
}

void RasterizerRender::RenderSonarLight()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	//glClearColor(0.67f, 0.84f, 0.90f, 1.f);
	glClearColor(0.0f, 0.0f, 0.0f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Erase the color and z buffers.

	GLuint texUnit = 0; // each shader start with texture0

	glm::mat4 viewMat = GLOBAL.camCtrller->GetActiveCamera()->GetViewMatrix();
	glm::mat4 projectMat = GLOBAL.camCtrller->GetActiveCamera()->GetProjectionMatrix();
	shared_ptr<ShaderProgram> shaderPro = GLOBAL.shaderMgr->TryActivateShaderProgram(GLOBAL.shaderPathPrefix + "SonarLight/sonarLight");
	shaderPro->Set("viewMat", viewMat);
	shaderPro->Set("projectMat", projectMat);

	// Here is the idea, we use such an area, which is inside two spheres with different radius: r1 and r2, to represent the sonar wave
	// All vertices inside this wave get lit up(distance to light is inside range [r1,r2] is lit up)
	// These two radius can be represent by using r-width/2, r+width/2, where width is user-defined, r is the distance of this wave to the sonar source.
	// to simulate the propagation of wave, we just move the r from 0 to waveInterval and repeat it.
	// compute the sonar light radius
	static float waveMaxDepth;
	static float waveSpeed;
	static float waveWidth;
	static float waveInterval;
	static float waveMoveOffset = 0;
	// TODO: to delete below codes, for now they are just temporary codes
	// TODO: to support save as .gif?
	{
		// below codes are written for debug
		// below scene setting is an example for setting up the maxDepth, width, waveSpeed
		string configPath = "Resources/SceneConfigs/SonarLightScene/sonar_light.json";
		nlohmann::json sceneData = Utility::LoadJsonFromFile(configPath);

		/*
			"wave_width": 0.01,
			"max_depth": 2.0,
			"wave_speed": 0.5,
			"wave_interval": 0.5
		*/
		waveWidth = sceneData["sonar_light_config"]["wave_width"].get<float>();
		waveMaxDepth = sceneData["sonar_light_config"]["max_depth"].get<float>();
		waveSpeed = sceneData["sonar_light_config"]["wave_speed"].get<float>();
		waveInterval = sceneData["sonar_light_config"]["wave_interval"].get<float>();
	}

	shaderPro->Set("waveMaxDepth", waveMaxDepth);
	shaderPro->Set("waveWidth", waveWidth);
	shaderPro->Set("waveInterval", waveInterval);
	
	waveMoveOffset += waveSpeed * static_cast<float>(GLOBAL.timeMgr->GetDeltaTime());
	if (waveMoveOffset >= waveInterval)
		waveMoveOffset = 0;

	//Print("waveMoveOffset: " + std::to_string(waveMoveOffset));
	shaderPro->Set("waveMoveOffset", waveMoveOffset);

	// pass light information to current shader
	shaderPro->Set("ambientLight", GLOBAL.sceneMgr->GetAmbient());
	auto lights = GLOBAL.sceneMgr->GetAllLight();
	shaderPro->Set("activeLightNum", static_cast<int>(lights.size()));
	for (size_t i = 0; i < lights.size(); i++)
	{
		shaderPro->Set("lights[" + std::to_string(i) + "].type", static_cast<int>(lights[i]->GetType()));
		shaderPro->Set("lights[" + std::to_string(i) + "].color", lights[i]->GetColor());
		shaderPro->Set("lights[" + std::to_string(i) + "].pos", lights[i]->GetTransform()->GetPosition());
		shaderPro->Set("lights[" + std::to_string(i) + "].intensity", lights[i]->GetIntensity());

		if (lights[i]->GetType() == LightType::POINT)
		{
			shared_ptr<PointLight> pointLight = static_pointer_cast<PointLight>(lights[i]);
			shaderPro->Set("lights[" + std::to_string(i) + "].attenuation", pointLight->GetAttenuation());
		}
		else if (lights[i]->GetType() == LightType::DIRECT)
		{
			shared_ptr<DirectLight> directLight = static_pointer_cast<DirectLight>(lights[i]);
			shaderPro->Set("lights[" + std::to_string(i) + "].dir", -directLight->GetDirection()); // shader is using the direction from fragment to light source, then here we should pass -direction.
		}
	}

	auto sceneObjs = GLOBAL.sceneMgr->GetAllSceneObject(); // not copy data, just return reference &
	for (auto iter = sceneObjs.begin(); iter != sceneObjs.end(); iter++)
	{
		auto sceneObj = *iter;
		glm::mat4 modelMat = sceneObj->GetTransform()->ComputeTransformationMatrix();
		shaderPro->Set("modelMat", modelMat);
		// pass material information to shader
		auto material = static_pointer_cast<PhongMaterial>(sceneObj->GetMaterial());
		shaderPro->Set("material.ka", material->GetAmbientCoef());
		shaderPro->Set("material.kd", material->GetDiffuseCoef());
		shaderPro->Set("material.ks", material->GetSpecularCoef());
		shaderPro->Set("material.shiness", material->GetShiness());
		if (material && material->GetUVDataSize() > 0 && material->GetAlbedo() != 0)
		{
			// OpenGL 4.5 way to use texture:
			// refer: https://www.khronos.org/opengl/wiki/Example_Code
			// TODO: but not sure whether I am doing right or not. Finish reading OpenGL book later then back to here.
			shaderPro->Set("albedoTex", static_cast<int>(texUnit));
			glBindTextureUnit(texUnit++, material->GetAlbedo()); // this function to bind texture object to sampler2D variable with "binding=texUnit"
			shaderPro->Set("useAlbedoTex", 1);
		}
		else
		{
			shaderPro->Set("useAlbedoTex", 0);
			shaderPro->Set("material.color", material->GetColor());
		}
		GLOBAL.render->Draw(sceneObj);
	}
}

void RasterizerRender::RenderDissolve()
{
	glViewport(0, 0, GLOBAL.WIN_WIDTH, GLOBAL.WIN_HEIGHT); // set it back to normal
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glClearColor(0.67f, 0.84f, 0.90f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Erase the color and z buffers.

	GLuint texUnit = 0; // each shader start with texture0

	// TODO to refactor codes here
	static float dissolve = -0.000001f;
	static float dissolveSpeed = 0.001f;
	static GLuint noiseTex = 0;
	{
		static bool hasLoad = false;
		if (!hasLoad)
		{
			/*load noise texture*/
			std::string texPath = "/Noise/melt_noise.png";
			texPath = "/Noise/noise.jpg";
			//texPath = "/Noise/noise2.jpg";
			if (Utility::Load2DTexture(texPath, true, noiseTex))
				Print("Load Noise \"" + texPath + "\" successfully!");
			hasLoad = true;
		}
	}

	glm::mat4 viewMat = GLOBAL.camCtrller->GetActiveCamera()->GetViewMatrix();
	glm::mat4 projectMat = GLOBAL.camCtrller->GetActiveCamera()->GetProjectionMatrix();
	shared_ptr<ShaderProgram> shaderPro = GLOBAL.shaderMgr->TryActivateShaderProgram(GLOBAL.shaderPathPrefix + "Dissolve/dissolve");
	shaderPro->Set("viewMat", viewMat);
	shaderPro->Set("projectMat", projectMat);

	bool needShadowRender = GLOBAL.shadowMgr->IsNeedShadowRender();
	shared_ptr<BasicShadowMapRender> basicShadowMapRender;
	if (needShadowRender)
		basicShadowMapRender = dynamic_pointer_cast<BasicShadowMapRender>(GLOBAL.shadowMgr->GetShadowRender());

	// pass light information to current shader
	shaderPro->Set("ambientLight", GLOBAL.sceneMgr->GetAmbient());
	auto lights = GLOBAL.sceneMgr->GetAllLight();
	shaderPro->Set("activeLightNum", static_cast<int>(lights.size()));
	for (size_t i = 0; i < lights.size(); i++)
	{
		shaderPro->Set("lights[" + std::to_string(i) + "].type", static_cast<int>(lights[i]->GetType()));
		shaderPro->Set("lights[" + std::to_string(i) + "].color", lights[i]->GetColor());
		shaderPro->Set("lights[" + std::to_string(i) + "].pos", lights[i]->GetTransform()->GetPosition());
		shaderPro->Set("lights[" + std::to_string(i) + "].intensity", lights[i]->GetIntensity());

		if (needShadowRender && lights[i]->IsRenderShadow())
			shaderPro->Set("lights[" + std::to_string(i) + "].renderShadow", 1);
		else
			shaderPro->Set("lights[" + std::to_string(i) + "].renderShadow", 0);

		if (lights[i]->GetType() == LightType::POINT)
		{
			shared_ptr<PointLight> pointLight = static_pointer_cast<PointLight>(lights[i]);
			shaderPro->Set("lights[" + std::to_string(i) + "].attenuation", pointLight->GetAttenuation());
		}
		else if (lights[i]->GetType() == LightType::DIRECT)
		{
			shared_ptr<DirectLight> directLight = static_pointer_cast<DirectLight>(lights[i]);
			shaderPro->Set("lights[" + std::to_string(i) + "].dir", -directLight->GetDirection()); // shader is using the direction from fragment to light source, then here we should pass -direction.
		}
	}

	// allow different shadow tecnique to use different light ratio sub shader.(pass parameters to final shader)
	// [Be careful] must pass texUnit into this function in order to bind the correct texture location in shader.
	if (needShadowRender)
		basicShadowMapRender->InitComputeLightRatioParameters(shaderPro, texUnit);

	auto sceneObjs = GLOBAL.sceneMgr->GetAllSceneObject(); // not copy data, just return reference &
	for (auto iter = sceneObjs.begin(); iter != sceneObjs.end(); iter++)
	{
		auto sceneObj = *iter;
		glm::mat4 modelMat = sceneObj->GetTransform()->ComputeTransformationMatrix();
		shaderPro->Set("modelMat", modelMat);
		// pass material information to shader
		auto material = static_pointer_cast<PhongMaterial>(sceneObj->GetMaterial());
		shaderPro->Set("material.ka", material->GetAmbientCoef());
		shaderPro->Set("material.kd", material->GetDiffuseCoef());
		shaderPro->Set("material.ks", material->GetSpecularCoef());
		shaderPro->Set("material.shiness", material->GetShiness());
		if (material && material->GetUVDataSize() > 0 && material->GetAlbedo() != 0)
		{
			// OpenGL 4.5 way to use texture:
			// refer: https://www.khronos.org/opengl/wiki/Example_Code
			// TODO: but not sure whether I am doing right or not. Finish reading OpenGL book later then back to here.
			shaderPro->Set("albedoTex", static_cast<int>(texUnit));
			glBindTextureUnit(texUnit++, material->GetAlbedo()); // this function to bind texture object to sampler2D variable with "binding=texUnit"
			shaderPro->Set("useAlbedoTex", 1);
		}
		else
		{
			shaderPro->Set("useAlbedoTex", 0);
			shaderPro->Set("material.color", material->GetColor());
		}

		// TODO
		/*hard code here, to be factored later*/
		if (sceneObj->GetName() == "obj_2")
		{
			// set dissolve
			shaderPro->Set("noiseTex", static_cast<int>(texUnit));
			glBindTextureUnit(texUnit++, noiseTex); // this function to bind texture object to sampler2D variable with "binding=texUnit"
			shaderPro->Set("useNoiseTex", 1);

			// dissolve
			shaderPro->Set("dissolve", dissolve);
			dissolve += dissolveSpeed;
		}
		else
			shaderPro->Set("useNoiseTex", 0);

		GLOBAL.render->Draw(sceneObj);

		if (dissolve >= 1.0f)
			dissolve = -0.000001f;
	}
}

void RasterizerRender::RenderDeferred()
{
	// TODO: refactor the whole function to improve the perfomance.
	// Not handle the aliasing issue when using deferred shading. To implement it in DX11
	auto gBuffers = GLOBAL.render->GetGBuffers();
	if (gBuffers.size() <= 0)
	{
		GLOBAL.render->CreateGBuffers();
		gBuffers = GLOBAL.render->GetGBuffers();
	}
	
	glViewport(0, 0, GLOBAL.WIN_WIDTH, GLOBAL.WIN_HEIGHT); // set it back to normal
	// bind to GBuffer's framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, gBuffers[0]);
	glClearColor(0.0f, 0.0f, 0.0f, 0.f); // be careful here, the final alpha channel is going to be used as flag indicating whether the fragment has been shaded.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Erase the color and z buffers.

	glm::mat4 viewMat = GLOBAL.camCtrller->GetActiveCamera()->GetViewMatrix();
	glm::mat4 projectMat = GLOBAL.camCtrller->GetActiveCamera()->GetProjectionMatrix();
	shared_ptr<ShaderProgram> shaderPro = GLOBAL.shaderMgr->TryActivateShaderProgram(GLOBAL.shaderPathPrefix + "DeferredShading/gbuffers");
	shaderPro->Set("viewMat", viewMat);
	shaderPro->Set("projectMat", projectMat);

	auto sceneObjs = GLOBAL.sceneMgr->GetAllSceneObject(); // not copy data, just return reference &
	for (auto iter = sceneObjs.begin(); iter != sceneObjs.end(); iter++)
	{
		auto sceneObj = *iter;
		if (sceneObj->GetName() == "screen_quad")
			continue;

		glm::mat4 modelMat = sceneObj->GetTransform()->ComputeTransformationMatrix();
		shaderPro->Set("modelMat", modelMat);
		// pass material information to shader
		auto material = static_pointer_cast<PhongMaterial>(sceneObj->GetMaterial());
		shaderPro->Set("material.ka", material->GetAmbientCoef());
		shaderPro->Set("material.kd", material->GetDiffuseCoef());
		shaderPro->Set("material.ks", material->GetSpecularCoef());
		shaderPro->Set("material.shiness", material->GetShiness());
		if (material && material->GetUVDataSize() > 0 && material->GetAlbedo() != 0)
		{
			// each shader start with texture0
			shaderPro->Set("albedoTex", 0);
			glBindTextureUnit(0, material->GetAlbedo()); // this function to bind texture object to sampler2D variable with "binding=texUnit"
			shaderPro->Set("useAlbedoTex", 1);
		}
		else
		{
			shaderPro->Set("useAlbedoTex", 0);
			shaderPro->Set("material.color", material->GetColor());
		}
		GLOBAL.render->Draw(sceneObj);
	}
	// ------------------------------------------------------------------------------------------------

	// TODO: to refactor below.
	auto screenQuadObj = GLOBAL.sceneMgr->GetSceneObj("screen_quad");
	if (screenQuadObj == nullptr)
	{
		shared_ptr<Mesh> mesh = MeshGenerator::GenPlane(2, 2, glm::vec3(0, 0, -1), Utility::backV3);
		shared_ptr<Material> material = make_shared<Material>();
		material->SetUV(MeshGenerator::GenPlaneUV());
		screenQuadObj = make_shared<SceneObject>("screen_quad", mesh, material);
		GLOBAL.sceneMgr->AddSceneObj(screenQuadObj);
	}

	// TODO: refactor below codes
	// try to do something similar to UE5 RenderGraphicBuilder
	// sometimes we need to call several render passes and those passes could take some textures/paramters as input 
	// and ouput to some textures/parameters. I should try to do something simliar to what UE5 does to make it easy to use.
	// (minimize the drawcalls and the times for transmission of textures inputs/outputs between CPU/GPU)

	SSAOConfig ssaoConfig = GLOBAL.shadowMgr->GetSSAOConfig();
	if (ssaoConfig.useSSAO)
	{
		// bind to SSAO framebuffer
		glBindFramebuffer(GL_FRAMEBUFFER, ssaoConfig.aoData[2]); CheckGLError();
		glClearColor(0.0f, 0.0f, 0.0f, 0.f); 
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Erase the color and z buffers.
		// Two passes: one pass to create AO, one pass to blur(filter) it[optional]
		{
			// - one for calling ComputeSSAO shader to output to SSAOConfig.aoTex
			GLOBAL.shadowMgr->BindToAOTex();

			shaderPro = GLOBAL.shaderMgr->TryActivateShaderProgram(GLOBAL.shaderPathPrefix + ssaoConfig.ssaoShader);

			if (!ssaoConfig.isDO)
			{
				shaderPro->Set("ssaoOccluderRadius", ssaoConfig.ssaoOccluderRadius);
				shaderPro->Set("ssaoDistanceFar", ssaoConfig.ssaoDistanceFar);
			}
			else
			{
				shaderPro->Set("viewMat", viewMat);
				shaderPro->Set("projectMat", projectMat);
				shaderPro->Set("ssdoSampleRadius", ssaoConfig.ssdoSampleRadius);
			}

			GLuint texUnit = 0; // each shader start with texture0
			std::vector<std::string> texNames = { "posTex", "normalTex" };
			for (size_t n = 0; n < texNames.size(); n++)
			{
				// each shader start with texture0
				shaderPro->Set(texNames[n], static_cast<int>(texUnit));
				glBindTextureUnit(static_cast<GLuint>(texUnit), gBuffers[n + 1]); // offset is 1 becaue frambuffer is at index=1
				texUnit++;
			}
			// generate ao texture
			GLOBAL.render->Draw(screenQuadObj);
		}

		{
			// check whether it needs to use filter shader to blur
			if (!ssaoConfig.filterShader.empty())
			{
				// - one pass for blurring this aoTex and save the result into blurred ao texture.
				GLOBAL.shadowMgr->BindToBlurredAOTex();
				shaderPro = GLOBAL.shaderMgr->TryActivateShaderProgram(GLOBAL.shaderPathPrefix + ssaoConfig.filterShader);
				shaderPro->Set("inputTex", static_cast<int>(0));
				glBindTextureUnit(static_cast<GLuint>(0), ssaoConfig.aoData[0]);
				// render screenquad to call shader.
				GLOBAL.render->Draw(screenQuadObj);
			}
		}
		
	}

	// ------------------------------------------------------------------------------------------------
	// Final pass - use G-Buffers and other textures(e.g. AOTex) as input to draw screen quad
	glBindFramebuffer(GL_FRAMEBUFFER, 0); // bind default frame buffer
	glClearColor(0.67f, 0.84f, 0.90f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Erase the color and z buffers.
	shaderPro = GLOBAL.shaderMgr->TryActivateShaderProgram(GLOBAL.shaderPathPrefix + "DeferredShading/deferred_shading");

	// render shadow map
	bool needShadowRender = GLOBAL.shadowMgr->IsNeedShadowRender();
	shared_ptr<BasicShadowMapRender> basicShadowMapRender;
	if (needShadowRender)
		basicShadowMapRender = dynamic_pointer_cast<BasicShadowMapRender>(GLOBAL.shadowMgr->GetShadowRender());

	shaderPro->Set("viewMat", viewMat);


	// pass light information to current shader
	shaderPro->Set("ambientLight", GLOBAL.sceneMgr->GetAmbient());
	auto lights = GLOBAL.sceneMgr->GetAllLight();
	shaderPro->Set("activeLightNum", static_cast<int>(lights.size()));
	for (size_t i = 0; i < lights.size(); i++)
	{
		shaderPro->Set("lights[" + std::to_string(i) + "].type", static_cast<int>(lights[i]->GetType()));
		shaderPro->Set("lights[" + std::to_string(i) + "].color", lights[i]->GetColor());
		shaderPro->Set("lights[" + std::to_string(i) + "].pos", lights[i]->GetTransform()->GetPosition());
		shaderPro->Set("lights[" + std::to_string(i) + "].intensity", lights[i]->GetIntensity());

		if (needShadowRender && lights[i]->IsRenderShadow())
			shaderPro->Set("lights[" + std::to_string(i) + "].renderShadow", 1);
		else
			shaderPro->Set("lights[" + std::to_string(i) + "].renderShadow", 0);

		if (lights[i]->GetType() == LightType::POINT)
		{
			shared_ptr<PointLight> pointLight = static_pointer_cast<PointLight>(lights[i]);
			shaderPro->Set("lights[" + std::to_string(i) + "].attenuation", pointLight->GetAttenuation());
		}
		else if (lights[i]->GetType() == LightType::DIRECT)
		{
			shared_ptr<DirectLight> directLight = static_pointer_cast<DirectLight>(lights[i]);
			shaderPro->Set("lights[" + std::to_string(i) + "].dir", -directLight->GetDirection()); // shader is using the direction from fragment to light source, then here we should pass -direction.
		}
	}

	GLuint texUnit = 0;

	// allow different shadow tecnique to use different light ratio sub shader.(pass parameters to final shader)
	// [Be careful] must pass texUnit into this function in order to bind the correct texture location in shader.
	if (needShadowRender)
	{
		basicShadowMapRender->InitComputeLightRatioParameters(shaderPro, texUnit); // texUnit will be increased inside this function call.
	}

	auto material = screenQuadObj->GetMaterial();
	if (material && material->GetUVDataSize() > 0)
	{
		std::vector<std::string> texNames = {"posTex", "normalTex", "albedoTex", "matTex"};
		for (size_t n = 0; n < texNames.size(); n++)
		{
			// each shader start with texture0
			shaderPro->Set(texNames[n], static_cast<int>(texUnit));
			glBindTextureUnit(static_cast<GLuint>(texUnit), gBuffers[n + 1]);
			texUnit++;
		}
	}
	else
	{
		Print("Error in RasterizerRender::RenderDeferred: UV data are required for screen_quad object.");
		return;
	}

	// bind AO texture if enable
	if (ssaoConfig.useSSAO)
	{
		shaderPro->Set("aoTex", static_cast<int>(texUnit));
		// if enable filter shader, then use blurred aoTex
		if (!ssaoConfig.filterShader.empty())
			glBindTextureUnit(static_cast<GLuint>(texUnit), ssaoConfig.aoData[1]); // blurred aoTex
		else
			glBindTextureUnit(static_cast<GLuint>(texUnit), ssaoConfig.aoData[0]); // aoTex
		texUnit++;
	}

	GLOBAL.render->Draw(screenQuadObj);
}

void RasterizerRender::RenderSimpleWater()
{
	// prepare Perlin Noise texture
	static GLuint noiseTex = 0;
	{
		if (!noiseTex)
		{
			/*load noise texture*/
			std::string texPath;
			//texPath = "/Noise/melt_noise.png";
			//texPath = "/Noise/noise.jpg";
			texPath = "/Noise/noise2.jpg";
			if (Utility::Load2DTexture(texPath, true, noiseTex))
				Print("Load Noise \"" + texPath + "\" successfully!");
		}
	}

	static GLuint bumpTex = 0;
	{
		if (!bumpTex)
		{
			/*load noise texture*/
			std::string texPath = "/Noise/bump.jpg";
			if (Utility::Load2DTexture(texPath, true, bumpTex))
				Print("Load Noise \"" + texPath + "\" successfully!");
		}
	}

	std::vector<std::shared_ptr<IceRender::SceneObject>> opaqueObjs;
	std::vector<std::shared_ptr<IceRender::SceneObject>> transparentObjs;
	// TODO: I should refactor the code for supporting the alpha channel to indicate whether the object is opaque.
	// for the time being, I just simply check the name.(water)
	auto sceneObjs = GLOBAL.sceneMgr->GetAllSceneObject();
	for (auto iter = sceneObjs.begin(); iter != sceneObjs.end(); iter++)
	{
		auto sceneObj = *iter;
		if (sceneObj->GetName() == "screen_quad")
			continue;
		if (sceneObj->GetName().find("_transparent") != std::string::npos)
			transparentObjs.push_back(sceneObj);
		else
			opaqueObjs.push_back(sceneObj);
	}

	// TODO: below it's not neccessary because I got only one water plane(transparent)
	// sort the transparent objects before rendering them: project all center of aabb box onto the camera view direction, and sort them according to the projected distance.
	// farthest object first.
	/*glm::vec3 viewDir, right, up;
	GLOBAL.camCtrller->GetActiveCamera()->GetCameraDirection(viewDir, right, up);
	auto camPos = GLOBAL.camCtrller->GetActiveCamera()->GetTransform()->GetPosition();

	std::sort(transparentObjs.begin(),
		transparentObjs.end(),
		[&viewDir, &camPos](std::shared_ptr<IceRender::SceneObject>& a, std::shared_ptr<IceRender::SceneObject>& b)
		{
			auto camToSceneObjA = a->GetBoundingBox()->GetCenter() - camPos;
			float projectedDistanceA = glm::dot(camToSceneObjA, viewDir);

			auto camToSceneObjB = b->GetBoundingBox()->GetCenter() - camPos;
			float projectedDistanceB = glm::dot(camToSceneObjB, viewDir);

			return projectedDistanceA > projectedDistanceB;
		});*/

	shared_ptr<ShaderProgram> shaderPro;
	GLuint texUnit = 0; // each shader start with texture0
	glm::mat4 viewMat = GLOBAL.camCtrller->GetActiveCamera()->GetViewMatrix();
	glm::mat4 projectMat = GLOBAL.camCtrller->GetActiveCamera()->GetProjectionMatrix();

	// ----------------------------------------------------- //
	static GLuint waterDepthInfoTex = 0;
	static GLuint waterDepthInfoFbo = 0;
	{
		// Preparing the water depth tex
		// before drawing the screen, do the distortion based on the vertices under water(only those part which we can see from water surface, which means use water_uv to get the pos)
		// It has to be distorting the verices under water. Distort the uv only works for object fully behind the glass.(not suitable for water)

		// prepare the water depth texture, for water foam line and distortion purpose.
		if (!waterDepthInfoTex)
		{
			// waterDepthInfoTex: R is depth, G indicates whether we have vertex info at this fragment.
			glCreateTextures(GL_TEXTURE_2D, 1, &waterDepthInfoTex); if (CheckGLError()) { Print("Error in waterDepthInfoTex."); return; };
			glTextureStorage2D(waterDepthInfoTex, 1, GL_RG32F, GLOBAL.WIN_WIDTH, GLOBAL.WIN_HEIGHT); if (CheckGLError()) { Print("Error in waterDepthInfoTex."); return; };
			glTextureParameteri(waterDepthInfoTex, GL_TEXTURE_MIN_FILTER, GL_LINEAR); if (CheckGLError()) { Print("Error in waterDepthInfoTex."); return; };
			glTextureParameteri(waterDepthInfoTex, GL_TEXTURE_MAG_FILTER, GL_LINEAR); if (CheckGLError()) { Print("Error in waterDepthInfoTex."); return; };
			glTextureParameteri(waterDepthInfoTex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
			glTextureParameteri(waterDepthInfoTex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
			const float borderColor[] = { 0,0,0,0 }; // alpha is 0 mean no position information at the fragment
			glTextureParameterfv(waterDepthInfoTex, GL_TEXTURE_BORDER_COLOR, borderColor); if (CheckGLError()) { Print("Error in waterDepthInfoTex."); return; };
		}

		// TODO: actually below it only works for one transparent(like water plane)
		if (!waterDepthInfoFbo)
		{
			glCreateFramebuffers(1, &waterDepthInfoFbo);
			GLenum attachments[1] = { GL_COLOR_ATTACHMENT0 }; // bind to (location=0) in shader
			glNamedFramebufferTexture(waterDepthInfoFbo, attachments[0], waterDepthInfoTex, 0); if (CheckGLError()) { Print("Error in waterDepthInfoFbo::glNamedFramebufferTexture."); return; };

			// [Important] We must create and bind a depthbuffer! Otherwise there is no way to update the depth information!!!
			GLuint depthRB; // this render buffer has the same size as ShadowMap does.
			glCreateRenderbuffers(1, &depthRB);
			glNamedRenderbufferStorage(depthRB, GL_DEPTH_COMPONENT, GLOBAL.WIN_WIDTH, GLOBAL.WIN_HEIGHT); if (CheckGLError()) { Print("Error in waterDepthInfoFbo::glNamedRenderbufferStorage."); return; };
			glNamedFramebufferRenderbuffer(waterDepthInfoFbo, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRB); if (CheckGLError()) { Print("Error in waterDepthInfoFbo::glNamedFramebufferRenderbuffer."); return; };
			glNamedFramebufferDrawBuffers(waterDepthInfoFbo, 1, attachments); if (CheckGLError()) { Print("Error in waterDepthInfoFbo::glNamedFramebufferDrawBuffers."); return; };
		}

		glBindFramebuffer(GL_FRAMEBUFFER, waterDepthInfoFbo); // bind to the new framebuffer rather than the default one
		glClearColor(0, 0, 0, 0); // alpha is 0 mean no position information at the fragment
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Erase the color and z buffers.


		texUnit = 0; // because we change the shader
		shaderPro = GLOBAL.shaderMgr->TryActivateShaderProgram(GLOBAL.shaderPathPrefix + "SimpleWater/waterDepthInfo");
		shaderPro->Set("viewMat", viewMat);
		shaderPro->Set("projectMat", projectMat);
		shaderPro->Set("time", static_cast<float>(GLOBAL.timeMgr->GetCurrentTime()));
		
		shaderPro->Set("noiseTex", static_cast<int>(texUnit));
		glBindTextureUnit(texUnit++, noiseTex);

		for (auto iter = transparentObjs.begin(); iter != transparentObjs.end(); iter++)
		{
			auto sceneObj = *iter;
			glm::mat4 modelMat = sceneObj->GetTransform()->ComputeTransformationMatrix();
			shaderPro->Set("modelMat", modelMat);
			GLOBAL.render->Draw(sceneObj);
		}
	}
	

	glViewport(0, 0, GLOBAL.WIN_WIDTH, GLOBAL.WIN_HEIGHT); // set it back to normal
	glBindFramebuffer(GL_FRAMEBUFFER, 0); // bind to the new framebuffer rather than the default one
	glClearColor(0.67f, 0.84f, 0.90f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Erase the color and z buffers.

	texUnit = 0; // because we change the shader
	shaderPro = GLOBAL.shaderMgr->TryActivateShaderProgram(GLOBAL.shaderPathPrefix + "SimpleWater/phong");
	shaderPro->Set("viewMat", viewMat);
	shaderPro->Set("projectMat", projectMat);

	bool needShadowRender = GLOBAL.shadowMgr->IsNeedShadowRender();
	shared_ptr<BasicShadowMapRender> basicShadowMapRender;
	if (needShadowRender)
		basicShadowMapRender = dynamic_pointer_cast<BasicShadowMapRender>(GLOBAL.shadowMgr->GetShadowRender());

	// pass light information to current shader
	shaderPro->Set("ambientLight", GLOBAL.sceneMgr->GetAmbient());
	auto lights = GLOBAL.sceneMgr->GetAllLight();
	shaderPro->Set("activeLightNum", static_cast<int>(lights.size()));
	for (size_t i = 0; i < lights.size(); i++)
	{
		shaderPro->Set("lights[" + std::to_string(i) + "].type", static_cast<int>(lights[i]->GetType()));
		shaderPro->Set("lights[" + std::to_string(i) + "].color", lights[i]->GetColor());
		shaderPro->Set("lights[" + std::to_string(i) + "].pos", lights[i]->GetTransform()->GetPosition());
		shaderPro->Set("lights[" + std::to_string(i) + "].intensity", lights[i]->GetIntensity());

		if (needShadowRender && lights[i]->IsRenderShadow())
			shaderPro->Set("lights[" + std::to_string(i) + "].renderShadow", 1);
		else
			shaderPro->Set("lights[" + std::to_string(i) + "].renderShadow", 0);

		if (lights[i]->GetType() == LightType::POINT)
		{
			shared_ptr<PointLight> pointLight = static_pointer_cast<PointLight>(lights[i]);
			shaderPro->Set("lights[" + std::to_string(i) + "].attenuation", pointLight->GetAttenuation());
		}
		else if (lights[i]->GetType() == LightType::DIRECT)
		{
			shared_ptr<DirectLight> directLight = static_pointer_cast<DirectLight>(lights[i]);
			shaderPro->Set("lights[" + std::to_string(i) + "].dir", -directLight->GetDirection()); // shader is using the direction from fragment to light source, then here we should pass -direction.
		}
	}

	// allow different shadow tecnique to use different light ratio sub shader.(pass parameters to final shader)
	// [Be careful] must pass texUnit into this function in order to bind the correct texture location in shader.
	if (needShadowRender)
		basicShadowMapRender->InitComputeLightRatioParameters(shaderPro, texUnit);
	
	// opaque objects rendering, with "waterDepthInfoTex" we can distort mesh vertices which are under water.
	shaderPro->Set("waterDepthInfoTex", static_cast<int>(texUnit));
	glBindTextureUnit(texUnit++, waterDepthInfoTex);

	shaderPro->Set("bumpTex", static_cast<int>(texUnit));
	glBindTextureUnit(texUnit++, bumpTex);

	shaderPro->Set("time", static_cast<float>(GLOBAL.timeMgr->GetCurrentTime()));
	for (auto iter = opaqueObjs.begin(); iter != opaqueObjs.end(); iter++)
	{
		auto sceneObj = *iter;
		glm::mat4 modelMat = sceneObj->GetTransform()->ComputeTransformationMatrix();
		shaderPro->Set("modelMat", modelMat);
		// pass material information to shader
		auto material = static_pointer_cast<PhongMaterial>(sceneObj->GetMaterial());
		shaderPro->Set("material.ka", material->GetAmbientCoef());
		shaderPro->Set("material.kd", material->GetDiffuseCoef());
		shaderPro->Set("material.ks", material->GetSpecularCoef());
		shaderPro->Set("material.shiness", material->GetShiness());
		if (material && material->GetUVDataSize() > 0 && material->GetAlbedo() != 0)
		{
			shaderPro->Set("albedoTex", static_cast<int>(texUnit));
			glBindTextureUnit(texUnit++, material->GetAlbedo()); // this function to bind texture object to sampler2D variable with "binding=texUnit"
			shaderPro->Set("useAlbedoTex", 1);
		}
		else
		{
			shaderPro->Set("useAlbedoTex", 0);
			shaderPro->Set("material.color", material->GetColor());
		}
		GLOBAL.render->Draw(sceneObj);
	}
	
	
	// ----------------------------------------------------- //
	// render the opaque depth texture for foam line.
	static GLuint opaqueDepthInfoTex = 0;
	static GLuint opaqueDepthInfoFbo = 0;
	{
		if (!opaqueDepthInfoTex)
		{
			// opaqueDepthInfoTex: R is depth, G indicates whether we have vertex info at this fragment.
			glCreateTextures(GL_TEXTURE_2D, 1, &opaqueDepthInfoTex); if (CheckGLError()) { Print("Error in opaqueDepthInfoTex."); return; };
			glTextureStorage2D(opaqueDepthInfoTex, 1, GL_RG32F, GLOBAL.WIN_WIDTH, GLOBAL.WIN_HEIGHT); if (CheckGLError()) { Print("Error in opaqueDepthInfoTex."); return; };
			glTextureParameteri(opaqueDepthInfoTex, GL_TEXTURE_MIN_FILTER, GL_LINEAR); if (CheckGLError()) { Print("Error in opaqueDepthInfoTex."); return; };
			glTextureParameteri(opaqueDepthInfoTex, GL_TEXTURE_MAG_FILTER, GL_LINEAR); if (CheckGLError()) { Print("Error in opaqueDepthInfoTex."); return; };
			glTextureParameteri(opaqueDepthInfoTex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
			glTextureParameteri(opaqueDepthInfoTex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
			const float borderColor[] = { 0,0,0,0 }; // alpha is 0 mean no position information at the fragment
			glTextureParameterfv(opaqueDepthInfoTex, GL_TEXTURE_BORDER_COLOR, borderColor); if (CheckGLError()) { Print("Error in opaqueDepthInfoTex."); return; };
		}

		if (!opaqueDepthInfoFbo)
		{
			glCreateFramebuffers(1, &opaqueDepthInfoFbo);
			GLenum attachments[1] = { GL_COLOR_ATTACHMENT0 }; // bind to (location=0) in shader
			glNamedFramebufferTexture(opaqueDepthInfoFbo, attachments[0], opaqueDepthInfoTex, 0); if (CheckGLError()) { Print("Error in opaqueDepthInfoFbo::glNamedFramebufferTexture."); return; };

			// [Important] We must create and bind a depthbuffer! Otherwise there is no way to update the depth information!!!
			GLuint depthRB; // this render buffer has the same size as ShadowMap does.
			glCreateRenderbuffers(1, &depthRB);
			glNamedRenderbufferStorage(depthRB, GL_DEPTH_COMPONENT, GLOBAL.WIN_WIDTH, GLOBAL.WIN_HEIGHT); if (CheckGLError()) { Print("Error in opaqueDepthInfoFbo::glNamedRenderbufferStorage."); return; };
			glNamedFramebufferRenderbuffer(opaqueDepthInfoFbo, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRB); if (CheckGLError()) { Print("Error in opaqueDepthInfoFbo::glNamedFramebufferRenderbuffer."); return; };
			glNamedFramebufferDrawBuffers(opaqueDepthInfoFbo, 1, attachments); if (CheckGLError()) { Print("Error in opaqueDepthInfoFbo::glNamedFramebufferDrawBuffers."); return; };
		}

		glBindFramebuffer(GL_FRAMEBUFFER, opaqueDepthInfoFbo); // bind to the new framebuffer rather than the default one
		glClearColor(0, 0, 0, 0); // alpha is 0 mean no position information at the fragment
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Erase the color and z buffers.

		texUnit = 0; // reset when using new shader.
		shaderPro = GLOBAL.shaderMgr->TryActivateShaderProgram(GLOBAL.shaderPathPrefix + "SimpleWater/opaqueDepthInfo");
		shaderPro->Set("viewMat", viewMat);
		shaderPro->Set("projectMat", projectMat);
		for (auto iter = opaqueObjs.begin(); iter != opaqueObjs.end(); iter++)
		{
			auto sceneObj = *iter;
			glm::mat4 modelMat = sceneObj->GetTransform()->ComputeTransformationMatrix();
			shaderPro->Set("modelMat", modelMat);
			GLOBAL.render->Draw(sceneObj);
		}
	}
	// ----------------------------------------------------- //


	// render transparent objects
	glBindFramebuffer(GL_FRAMEBUFFER, 0); // bind back to the default one
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // the resultTex has been drawn into default frame buffer
	//glBlendFunci(resultTex, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	texUnit = 0; // because we change the shader
	shaderPro = GLOBAL.shaderMgr->TryActivateShaderProgram(GLOBAL.shaderPathPrefix + "SimpleWater/simpleWater");
	shaderPro->Set("viewMat", viewMat);
	shaderPro->Set("projectMat", projectMat);

	shaderPro->Set("time", static_cast<float>(GLOBAL.timeMgr->GetCurrentTime()));

	// set noise texture
	shaderPro->Set("noiseTex", static_cast<int>(texUnit));
	glBindTextureUnit(texUnit++, noiseTex);
	
	shaderPro->Set("opaqueDepthInfoTex", static_cast<int>(texUnit));
	glBindTextureUnit(texUnit++, opaqueDepthInfoTex);

	for (auto iter = transparentObjs.begin(); iter != transparentObjs.end(); iter++)
	{
		auto sceneObj = *iter;
		glm::mat4 modelMat = sceneObj->GetTransform()->ComputeTransformationMatrix();
		shaderPro->Set("modelMat", modelMat);
		
		auto material = sceneObj->GetMaterial();
		if (material && material->GetUVDataSize() > 0)
			shaderPro->Set("albedoColor", material->GetColor());

		GLOBAL.render->Draw(sceneObj);
	}
	glDisable(GL_BLEND);
}