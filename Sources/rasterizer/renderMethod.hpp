#pragma once

#include <memory>
#include "../shadermgr/shaderProgram.hpp"


namespace IceRender
{
	// different render methods
	namespace RasterizerRender
	{
		void NoRender();
		void RenderSimple();
		void RenderPhong();
		void RenderSceenQuad();
		void RenderDeferred();
		void RenderPostProcess(); // it will be called by any other post-processing drawcall to prepare the texture capturing the whole scene.

#pragma region Rendering techinique for fun
		void RenderSonarLight(); // just for fun
		void RenderDissolve(); // just for fun
		void RenderSimpleWater(); // just for fun
		void RenderDistortion(); // just for fun
#pragma endregion
	}

	namespace RayTracerRender
	{
		// TODO:
	}
}
