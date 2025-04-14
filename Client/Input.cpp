#include "Input.h"
#include "imgui_impl_glfw.h"
#include <iostream>
#include <Windows.h>

void Input::SetMouseAcceleration(bool Enabled)
{
	if (glfwRawMouseMotionSupported())
		glfwSetInputMode(Window, GLFW_RAW_MOUSE_MOTION, Enabled);
}

void Input::SetStandardKeys()
{
	Keys[A] = { GLFW_KEY_A };
	Keys[D] = { GLFW_KEY_D };
	Keys[W] = { GLFW_KEY_W };
	Keys[S] = { GLFW_KEY_S };
	Keys[Q] = { GLFW_KEY_Q };
	Keys[E] = { GLFW_KEY_E };
	Keys[X] = { GLFW_KEY_X };
	Keys[Z] = { GLFW_KEY_Z };
	Keys[G] = { GLFW_KEY_G };
	Keys[SPACE] = { GLFW_KEY_SPACE };
	Keys[ARROW_LEFT] = { GLFW_KEY_LEFT };
	Keys[ARROW_RIGHT] = { GLFW_KEY_RIGHT };
	Keys[ARROW_UP] = { GLFW_KEY_UP };
	Keys[ARROW_DOWN] = { GLFW_KEY_DOWN };
	Keys[LSHIFT] = { GLFW_KEY_LEFT_SHIFT };
	Keys[LCTRL] = { GLFW_KEY_LEFT_CONTROL };
	Keys[RSHIFT] = { GLFW_KEY_RIGHT_SHIFT };
	Keys[RCTRL] = { GLFW_KEY_RIGHT_CONTROL };
	Keys[ENTER] = { GLFW_KEY_ENTER };
	Keys[PAGE_UP] = { GLFW_KEY_PAGE_UP };
	Keys[PAGE_DOWN] = { GLFW_KEY_PAGE_DOWN };
	Keys[ESCAPE] = { GLFW_KEY_ESCAPE };
	Keys[F1] = { GLFW_KEY_F1 };
	Keys[F2] = { GLFW_KEY_F2 };
	Keys[F3] = { GLFW_KEY_F3 };
}

void Input::SetCallbacks()
{
	glfwSetWindowUserPointer(Window, this);

	glfwSetKeyCallback(Window, [](GLFWwindow* Window, int Keycode, int ScanCode, int Action, int Mods)
		{
			Input* Instance = static_cast<Input*>(glfwGetWindowUserPointer(Window));
			if (Instance && Window == Instance->Window)
				Instance->KeyCallback(Window, Keycode, ScanCode, Action, Mods);
		});

	glfwSetMouseButtonCallback(Window, [](GLFWwindow* Window, int Button, int Action, int Mods)
		{
			Input* Instance = static_cast<Input*>(glfwGetWindowUserPointer(Window));
			if (Instance && Window == Instance->Window)
				Instance->MouseButtonCallback(Window, Button, Action, Mods);
		});

	glfwSetScrollCallback(Window, [](GLFWwindow* Window, double X, double Y)
		{
			Input* Instance = static_cast<Input*>(glfwGetWindowUserPointer(Window));
			if (Instance && Window == Instance->Window)
				Instance->ScrollCallback(Window, X, Y);
		});

	glfwSetCursorEnterCallback(Window, [](GLFWwindow* Window, int Entered) // causes crashing
		{
			Input* Instance = static_cast<Input*>(glfwGetWindowUserPointer(Window));
			if (Instance && Window == Instance->Window)
				Instance->CursorEnterCallback(Window, Entered);
		});

	glfwSetCursorPosCallback(Window, [](GLFWwindow* Window, double X, double Y)
		{
			try
			{
				Input* Instance = static_cast<Input*>(glfwGetWindowUserPointer(Window));
				if (Instance && Window == Instance->Window) Instance->CursorPositionCallback(Window, X, Y);
			}
			catch (...)
			{
#ifdef _DEBUG
				printf("An exception was caused by glfwSetCursorEnterCallback (did you auto size the window?)\n");
#endif
			}
		});
}

void Input::ResetJustReleased()
{
	for (auto& KeyEntry : Keys)
	{
		auto& Key = KeyEntry.second;
		Key.JustReleased = false;
	}
}

glm::vec2 Input::GetMouseWorldPosition()
{
	double XPosition, YPosition;
	glfwGetCursorPos(Window, &XPosition, &YPosition);

	int WindowWidth, WindowHeight;
	glfwGetWindowSize(Window, &WindowWidth, &WindowHeight);

	float NDCX = (XPosition / WindowWidth) * 2.0 - 1.0;
	float NDCY = (YPosition / WindowHeight) * 2.0 - 1.0;

	return glm::vec2(NDCX, NDCY);
}

void Input::KeyCallback(GLFWwindow* Window, int Keycode, int ScanCode, int Action, int Mods)
{
	for (auto& KeyEntry : Keys)
	{
		auto& Key = KeyEntry.second;
		if (Key.Keycode == Keycode)
		{
			if (Key.PressedOrRepeated && Action == GLFW_RELEASE) Key.JustReleased = true;
			Key.State = Action;
			Key.PressedOrRepeated = (Action == GLFW_PRESS || Action == GLFW_REPEAT);
		}
	}
}

void Input::MouseButtonCallback(GLFWwindow* Window, int Button, int Action, int Mods)
{
	ImGui_ImplGlfw_MouseButtonCallback(Window, Button, Action, Mods);
}

void Input::ScrollCallback(GLFWwindow* Window, double X, double Y)
{

}

void Input::CursorEnterCallback(GLFWwindow* Window, int Entered)
{

}

void Input::CursorPositionCallback(GLFWwindow* Window, double X, double Y)
{
	// to stop jumping when first receiving inputs
	if (FirstMouse)
	{
		LastX = X;
		LastY = Y;
		FirstMouse = false;
	}

	// calculate difference between frames
	XChange = X - LastX;
	YChange = LastY - Y; // Reversed: Y-coordinates go from bottom to top
	LastX = X;
	LastY = Y;

	// adjust for sensitivity
	float Sensitivity = 0.1f;
	XChange *= Sensitivity;
	YChange *= Sensitivity;

	Yaw += static_cast<float>(XChange);
	Pitch += static_cast<float>(YChange);
}
