#include "NetManager.h"
#include "GraphicsManager.h"
#include "Object.h"
#ifdef _DEBUG
#include <iostream>
#endif
#define SELECTION_COLOUR IM_COL32(255, 0, 0, 255)

int main()
{
	// Initialize local server, we won't connect yet.
	if (!NetManager::InitializeENet()) throw "Failed to start.";
	NetManager::Manager NManager = NetManager::Manager();
	if (!NManager.TryCreateLocalServer()) throw "Failed to start.";

	// Initialize OpenGL (creates window and whatnot)
	GraphicsManager::Manager GManager = GraphicsManager::Manager();
	if (!GManager.Initialize()) throw "Failed to start.";
	GManager.SetupStandardShaders();
	Object MainObject;
	GManager.UpdateCameraOrbit(glm::vec3(0, 0, 0));
	int SelectedVertexIndex = 0;

	while (GManager.StandardFrameStart(0.2f, 0.3f, 0.4f, 1.0f))
	{
		GManager.Projection = glm::perspective(glm::radians(45.0f), 1920.0f / 1080.0f, 0.1f, 100.0f);

		if (glfwGetMouseButton(GManager.Window, GLFW_MOUSE_BUTTON_MIDDLE))
			GManager.UpdateCameraOrbit(glm::vec3(0, 0, 0));

		// Draw modes
		if (GManager.IManager->Keys[F1].JustReleased)
		{
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			GManager.IManager->Keys[F1].JustReleased = false;
		}
		if (GManager.IManager->Keys[F2].JustReleased) 
		{
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			GManager.IManager->Keys[F2].JustReleased = false;
		}
		if (GManager.IManager->Keys[F3].JustReleased)
		{
			glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
			GManager.IManager->Keys[F3].JustReleased = false;
		}

		MainObject.Render(GManager);

		if (glfwGetMouseButton(GManager.Window, 0) == GLFW_PRESS)
		{
			ImVec2 MouseScreenPosition = ImGui::GetIO().MousePos;
			auto CursorWorldPosition = GManager.ScreenToWorldPosition(MouseScreenPosition);
			SelectedVertexIndex = MainObject.GetClosestVertex(CursorWorldPosition);
		}

		auto VertexPosition = MainObject.GetVertexWorldPosition(SelectedVertexIndex);
		auto VertexScreenPosition = GManager.WorldToScreenPosition(VertexPosition);

		// Draw the circle
		ImDrawList* DrawList = ImGui::GetBackgroundDrawList();
		DrawList->AddCircle(VertexScreenPosition, 10.0f, SELECTION_COLOUR);

		if (ImGui::Begin("Together Make"))
		{
			if (ImGui::BeginTabBar("Tabs"))
			{
				if (ImGui::BeginTabItem("Material"))
				{
					ImGui::ColorPicker4("Colour", (float*)&MainObject.Colour, ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreviewHalf);
					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem("Multiplayer"))
				{
					if (NManager.Server)
					{

					}
					else
					{
						ImGui::Text("Not Connected...");
						ImGui::SameLine();
						if (ImGui::Button("Connect"))
						{
							if (!NManager.TryConnectToMatchmakingServer())
							{
								std::cout << "failed to connect\n";
							}
						}
					}
					ImGui::EndTabItem();
				}
				ImGui::EndTabBar();
			}
			ImGui::End();
		}

		GManager.StandardFrameEnd();
	}
}