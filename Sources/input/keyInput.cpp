#include "../globals.hpp"
#include "../helpers/utility.hpp"

using namespace IceRender;

KeyInput::KeyInput()
{
	// TODO: keep update here if adding any new key. Don't delete this todo. Just for reminders.
	keyStateMap[GLFW_KEY_F1] = false;
	keyStateMap[GLFW_KEY_GRAVE_ACCENT] = false;
}

KeyInput::~KeyInput()
{
	keyStateMap.clear();
}

void KeyInput::ProcessInput(GLFWwindow* _window)
{
	// By using #define to simplify coding
#define PressMap(_key, _codes) if (glfwGetKey(_window, _key) == GLFW_PRESS) _codes;
	// ------------------------------------------------------------------------------ //

	PressMap(GLFW_KEY_ESCAPE, glfwSetWindowShouldClose(_window, true));

	// camera movement
	PressMap(GLFW_KEY_UP, GLOBAL.camCtrller->MoveCamera(Utility::upV2));
	PressMap(GLFW_KEY_DOWN, GLOBAL.camCtrller->MoveCamera(Utility::downV2));
	PressMap(GLFW_KEY_LEFT, GLOBAL.camCtrller->MoveCamera(Utility::leftV2));
	PressMap(GLFW_KEY_RIGHT, GLOBAL.camCtrller->MoveCamera(Utility::rightV2));

	// camera rotation
	PressMap(GLFW_KEY_W, GLOBAL.camCtrller->RotateCamera(Utility::rightV2));
	PressMap(GLFW_KEY_S, GLOBAL.camCtrller->RotateCamera(Utility::leftV2));
	PressMap(GLFW_KEY_A, GLOBAL.camCtrller->RotateCamera(Utility::upV2));
	PressMap(GLFW_KEY_D, GLOBAL.camCtrller->RotateCamera(Utility::downV2));

	// camera zoom
	PressMap(GLFW_KEY_Z, GLOBAL.camCtrller->ZoomCamera(-1));
	PressMap(GLFW_KEY_X, GLOBAL.camCtrller->ZoomCamera(1));

	// camera rise/descend
	PressMap(GLFW_KEY_E, GLOBAL.camCtrller->RiseDescend(1));
	PressMap(GLFW_KEY_Q, GLOBAL.camCtrller->RiseDescend(-1));

	// ------------------------------------------------------------------------------ //
#undef PressMap
	

	// By using #define to simplify coding
	bool current;
#define ClickMap(_key, _codes) current = glfwGetKey(_window, _key); \
if (current == GLFW_PRESS && !keyStateMap[_key]) \
{keyStateMap[_key] = true; _codes;} \
else if (current == GLFW_RELEASE && keyStateMap[_key])keyStateMap[_key] = false;
	// ------------------------------------------------------------------------------ //

	ClickMap(GLFW_KEY_F1, GLOBAL.console->PrintHelp());
	
	ClickMap(GLFW_KEY_GRAVE_ACCENT, GLOBAL.console->HandleCommand(GLOBAL.console->GetCommandFromConsole()));

	ClickMap(GLFW_KEY_F2, GLOBAL.camCtrller->ResetCamera());

	ClickMap(GLFW_KEY_F3, {
		std::string preCommand = GLOBAL.console->GetPreviousCommand();
		if (!preCommand.empty())
		{
			Print("Handling previous command: " + preCommand);
			GLOBAL.console->HandleCommand(preCommand);
		}
		else
			Print("There are no commands which have been executed.");
	});
	// ------------------------------------------------------------------------------ //
#undef ClickMap
}
