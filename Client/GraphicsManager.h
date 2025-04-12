#pragma once
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "GL/glew.h"
#include "GLFW/glfw3.h"

#include "imgui.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_glfw.h"
#include "imgui_stdlib.h"

#include "ShaderCode.h"
#include "Input.h"
#include <vector>
#include <chrono>
#include <string>
#define USE_VSYNC 1

std::vector<float> CubeVertices =
{
	// Front face
	-0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
	 0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
	 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
	 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
	-0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
	-0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

	// Back face
	-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
	 0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
	 0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
	 0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
	-0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
	-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

	// Left face
	-0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
	-0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
	-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
	-0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
	-0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
	-0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

	// Right face
	 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
	 0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
	 0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
	 0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
	 0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
	 0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

	 // Bottom face
	 -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
	  0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
	  0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
	  0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
	 -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
	 -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

	 // Top face
	 -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
	  0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
	  0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
	  0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
	 -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
	 -0.5f,  0.5f, -0.5f,  0.0f, 1.0f
};

namespace GraphicsManager
{
	class Manager
	{
	public:
		GLFWwindow* Window = nullptr;
		unsigned int VBO, VAO, EBO;
		unsigned int PrimaryShaderProgram;
		glm::mat4 View = glm::mat4(1.0f);
		glm::mat4 Projection = glm::mat4(1.0f);
		glm::vec3 CameraPosition = glm::vec3(0.0f, 0.0f, 15.0f);
		glm::vec3 CameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
		glm::vec3 CameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
		double DeltaTime = 0.0f;
		std::chrono::high_resolution_clock::time_point PreviousTime;
		std::vector<unsigned int> ShadersToDestroyOnCleanup;
		Input* IManager = nullptr;

		// Sun Light Info:
		glm::vec3 LightPosition = glm::vec3(0.0f, -1.0f, -0.3f);
		glm::vec3 LightColour = glm::vec3(1.0f, 1.0f, 1.0f);
		float LightRange = 15.0f;

		bool Initialize()
		{
			// GLFW
			if (!glfwInit())
				return false;

			glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
			glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
			glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);

			Window = glfwCreateWindow(1920, 1080, "Together Make", nullptr, nullptr);
			if (!Window)
			{
				glfwTerminate();
				return false;
			}

			glfwMakeContextCurrent(Window);
			glfwSwapInterval(USE_VSYNC); // vsync
			glewInit();

			glEnable(GL_DEPTH_TEST);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			// OpenGL3
			IMGUI_CHECKVERSION();
			ImGui::CreateContext();
			ImGuiIO& io = ImGui::GetIO();
			(void)io;

			ImGui_ImplGlfw_InitForOpenGL(Window, false);
			ImGui_ImplOpenGL3_Init("#version 440");
			ImGui::StyleColorsDark();
			IManager = new Input(Window);

			return true;
		}

		void Cleanup()
		{
			if (!Window) return;

			for (auto Shader : ShadersToDestroyOnCleanup) glDeleteShader(Shader);

			ImGui_ImplOpenGL3_Shutdown();
			ImGui_ImplGlfw_Shutdown();
			ImGui::DestroyContext();
			glfwDestroyWindow(Window);
			glfwTerminate();
			if (IManager) { delete IManager; IManager = nullptr; }
		}

		// handles standard things like creating a new imgui frame
		// returns false if the frame shouldnt be drawn due to glfwWindowShouldClose
		// will handle cleanup if glfwWindowShouldClose is true
		bool StandardFrameStart(GLclampf r = 0.1f, GLclampf g = 0.1f, GLclampf b = 0.1f, GLclampf a = 0.1f)
		{
			if (!Window) return false;
			glfwPollEvents();
			if (glfwWindowShouldClose(Window)) { Cleanup(); return false; }
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glClearColor(r, g, b, a);

			// Delta Time
			auto CurrentTime = std::chrono::high_resolution_clock::now();
			std::chrono::duration<double> DeltaTimeDur = CurrentTime - PreviousTime;
			DeltaTime = DeltaTimeDur.count();
			PreviousTime = CurrentTime;
			return true;
		}

		// handles standard things like rendering and swapping the buffers, params are the glClearColor params
		void StandardFrameEnd()
		{
			if (!Window) return;
			ImGui::Render();
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
			glfwSwapBuffers(Window);
		}

		unsigned int CreateShader(const char* Source, int Type = GL_VERTEX_SHADER)
		{
			unsigned int NewShader = glCreateShader(Type);
			glShaderSource(NewShader, 1, &Source, NULL);
			glCompileShader(NewShader);

			int Success;
			char InfoLog[512];

			glGetShaderiv(NewShader, GL_COMPILE_STATUS, &Success);

			if (!Success)
			{
				glGetShaderInfoLog(NewShader, 512, NULL, InfoLog);
#ifdef _DEBUG
				std::cout << "Shader compilation failed, error: " << InfoLog << std::endl;
#endif
				throw;
			}

			return NewShader;
		}

		unsigned int CreateShaderProgram(std::vector<unsigned int> Shaders)
		{
			unsigned int NewShaderProgram = glCreateProgram();
			for (unsigned int Shader : Shaders)
				glAttachShader(NewShaderProgram, Shader);
			glLinkProgram(NewShaderProgram);

			return NewShaderProgram;
		}

		bool CreateBuffers()
		{
			glGenVertexArrays(1, &VAO);
			glGenBuffers(1, &VBO);

			glBindVertexArray(VAO);

			glBindBuffer(GL_ARRAY_BUFFER, VBO);
			glBufferData(GL_ARRAY_BUFFER, CubeVertices.size() * sizeof(float), CubeVertices.data(), GL_STATIC_DRAW);

			// position attribute
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
			glEnableVertexAttribArray(0);
			// texture coord attribute
			//glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
			//glEnableVertexAttribArray(1);
			// colour coord attribute
			//glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
			//glEnableVertexAttribArray(2);

			return true;
		}

		void SetupStandardShaders()
		{
			auto VertexShader = CreateShader(StandardVertexShaderSource, GL_VERTEX_SHADER);
			ShadersToDestroyOnCleanup.push_back(VertexShader);
			auto FragmentShader = CreateShader(StandardFragmentShaderSource, GL_FRAGMENT_SHADER);
			ShadersToDestroyOnCleanup.push_back(FragmentShader);
			PrimaryShaderProgram = CreateShaderProgram(std::vector<unsigned int> { VertexShader, FragmentShader });
			CreateBuffers();
		}

		inline ImVec2 WorldToScreenPosition(glm::vec4 WorldPosition)
		{
			glm::vec4 ClipSpace = Projection * View * WorldPosition;
			glm::vec3 NDC = glm::vec3(ClipSpace) / ClipSpace.w;
			float ScreenX = (NDC.x * 0.5f + 0.5f) * 1920;
			float ScreenY = (1.0f - (NDC.y * 0.5f + 0.5f)) * 1080; // flip Y for top-left
			return ImVec2(ScreenX, ScreenY);
		}

		inline ImVec2 WorldToScreenPosition(glm::vec3 WorldPosition)
		{
			return WorldToScreenPosition(glm::vec4(WorldPosition, 1.0f));
		}

		inline glm::vec3 ScreenToWorldPosition(ImVec2 ScreenPosition)
		{
			float NDCX = (2.0f * ScreenPosition.x) / 1920.0f - 1.0f;
			float NDCY = 1.0f - (2.0f * ScreenPosition.y) / 1080.0f; // Flip Y
			glm::vec4 RayClip = glm::vec4(NDCX, NDCY, -1.0f, 1.0f);
			glm::vec4 RayEye = glm::inverse(Projection) * RayClip;
			RayEye = glm::vec4(RayEye.x, RayEye.y, -1.0f, 0.0f);
			glm::vec4 RayWorld = glm::inverse(View) * RayEye;
			glm::vec3 RayDirection = glm::normalize(glm::vec3(RayWorld));

			return RayDirection;
		}
		
		void UpdateCameraFirstPerson()
		{
			float ClampedPitch = glm::clamp(IManager->Pitch, -89.0f, 89.0f);

			float PitchRad = glm::radians(ClampedPitch);
			float YawRad = glm::radians(IManager->Yaw);
			CameraFront.x = cos(YawRad) * cos(PitchRad);
			CameraFront.y = sin(PitchRad);
			CameraFront.z = sin(YawRad) * cos(PitchRad);
			CameraFront = glm::normalize(CameraFront);

			View = glm::lookAt(CameraPosition, CameraPosition + CameraFront, CameraUp);
		}

		void UpdateCameraOrbit(glm::vec3 Around)
		{
			glm::vec2 MouseDelta = glm::vec2(IManager->XChange, IManager->YChange);

			// 2. Adjust Yaw and Pitch based on the delta
			float Sensitivity = 0.1f; // Adjust sensitivity for smooth camera movement
			IManager->Yaw += MouseDelta.x * Sensitivity;
			IManager->Pitch -= MouseDelta.y * Sensitivity; // Invert Y for typical camera control

			// 3. Clamp Pitch
			float ClampedPitch = glm::clamp(IManager->Pitch, -89.0f, 89.0f);
			float PitchRad = glm::radians(ClampedPitch);
			float YawRad = glm::radians(IManager->Yaw);

			// 4. Calculate Camera Position and Front
			float OrbitDistance = 10.0f;
			float XPos = Around.x + OrbitDistance * cos(-YawRad) * cos(PitchRad);
			float YPos = Around.y + OrbitDistance * sin(PitchRad);
			float ZPos = Around.z + OrbitDistance * sin(-YawRad) * cos(PitchRad);
			CameraPosition = glm::vec3(XPos, YPos, ZPos);
			CameraFront = glm::normalize(Around - CameraPosition);

			// 5. Update View Matrix
			View = glm::lookAt(CameraPosition, CameraPosition + CameraFront, CameraUp);
		}
	};
}