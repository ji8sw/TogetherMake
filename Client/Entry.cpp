#include "NetManager.h"
#include "GraphicsManager.h"
#include "Object.h"
#ifdef _DEBUG
#include <iostream>
#include <format>
#endif
#define SELECTION_COLOUR IM_COL32(255, 0, 0, 255)
#define OTHER_PLAYER_SELECTION_COLOUR IM_COL32(255, 129, 0, 255)

enum EMode
{
	None,
	Move
};

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
	NManager.MainObject = &MainObject;
	GManager.UpdateCameraOrbit(glm::vec3(0, 0, 0));
	int SelectedVertexIndex = -1;
	EMode SelectedMode = None;
	glm::vec3 DragStartWorldPosition;
	glm::vec2 DragStartMousePosition;

	while (GManager.StandardFrameStart(0.2f, 0.3f, 0.4f, 1.0f))
	{
		NManager.RecievePackets();
		GManager.Projection = glm::perspective(glm::radians(GManager.CameraFOV), 1920.0f / 1080.0f, 0.1f, 100.0f);
		ImDrawList* DrawList = ImGui::GetBackgroundDrawList();
		auto MousePosition = ImGui::GetIO().MousePos;

		if (GManager.IManager->Keys[MIDDLE_MOUSE].PressedOrRepeated)
			GManager.UpdateCameraOrbit(glm::vec3(0, 0, 0));

		// Draw modes
		if (GManager.IManager->Keys[F1].JustReleased)
		{
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}
		if (GManager.IManager->Keys[F2].JustReleased) 
		{
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}
		if (GManager.IManager->Keys[F3].JustReleased)
		{
			glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
		}

		if (GManager.IManager->Keys[G].JustReleased && SelectedVertexIndex != INVALID_INT) // G: Move vertex mode
		{
			SelectedMode = Move;
			DragStartMousePosition = glm::vec2(MousePosition.x, MousePosition.y);
			DragStartWorldPosition = MainObject.GetVertexWorldPosition(SelectedVertexIndex);
		}

		if (GManager.IManager->Keys[ESCAPE].JustReleased && SelectedMode != None) // Escape: cancel changes
		{
			switch (SelectedMode)
			{
			case Move:
			{
				if (SelectedVertexIndex == INVALID_INT) SelectedMode = None;
				MainObject.SetVertexWorldPosition(SelectedVertexIndex, glm::vec4(DragStartWorldPosition, 1.0f)); // resets vertex position to original
				break;
			}
			}

			SelectedMode = None;
		}

		if (GManager.IManager->Keys[ENTER].JustReleased && SelectedMode != None) // Enter: confirm changes
		{
			SelectedMode = None;
			NManager.SendUpdateVertexPosition(SelectedVertexIndex, MainObject.Vertices[SelectedVertexIndex].Position);
		}

		switch (SelectedMode)
		{
			case Move:
			{
				if (SelectedVertexIndex == INVALID_INT) SelectedMode = None;
				glm::vec2 MouseDelta = glm::vec2(MousePosition.x, MousePosition.y) - DragStartMousePosition;

				glm::vec3 CameraRight = glm::vec3(glm::inverse(GManager.View)[0]); // x

				float Sensitivity = 0.00007f;
				glm::vec2 Scale = GManager.GetWorldPerPixel(DragStartWorldPosition);

				// convert screen space to world space movement (delta)
				glm::vec3 Change =
					(-MouseDelta.x * -CameraRight / Scale.x +
						-MouseDelta.y * GManager.CameraUp / Scale.y) * Sensitivity;

				glm::vec3 NewWorldPosition = DragStartWorldPosition + Change;

				MainObject.SetVertexWorldPosition(SelectedVertexIndex, glm::vec4(NewWorldPosition, 1.0f));
				break;
			}
		}

		MainObject.Render(GManager);

		if (glfwGetMouseButton(GManager.Window, 0) == GLFW_PRESS && SelectedMode == None) // Left click: select vertex
		{
			glm::vec3 RayOrigin = GManager.CameraPosition; // Or extract from View matrix
			glm::vec3 RayDirection = GManager.ScreenToWorldPosition(MousePosition);
			int DesiredVertexIndex = MainObject.GetClosestVertexToRay(RayOrigin, RayDirection, 1);
			if (DesiredVertexIndex != INVALID_INT)
			{
				bool AlreadySelected = SelectedVertexIndex == DesiredVertexIndex;
				// Ensure vertex is not grabbed by another player
				if (!AlreadySelected && !MainObject.Vertices[DesiredVertexIndex].IsGrabbed)
				{
					// Deselect current vertex then select the other
					if (SelectedVertexIndex != INVALID_INT) NManager.SendOnDeselectVertex(SelectedVertexIndex);
					SelectedVertexIndex = DesiredVertexIndex;
					NManager.SendOnSelectVertex(SelectedVertexIndex);
				}
			}
		}

		if (SelectedVertexIndex != INVALID_INT)
		{
			auto VertexPosition = MainObject.GetVertexWorldPosition(SelectedVertexIndex);
			auto VertexScreenPosition = GManager.WorldToScreenPosition(VertexPosition);

			// Draw the circle
			DrawList->AddCircle(VertexScreenPosition, 10.0f, SELECTION_COLOUR);
		}

		// if we are connected, draw circles on all vertices that are grabbed by other players
		if (NManager.Server)
		{
			for (const NetVertex& Vertex : MainObject.Vertices)
			{
				if (Vertex.IsGrabbed)
				{
					auto VertexPosition = MainObject.GetVertexWorldPosition(Vertex);
					auto VertexScreenPosition = GManager.WorldToScreenPosition(VertexPosition);

					DrawList->AddCircle(VertexScreenPosition, 10.0f, OTHER_PLAYER_SELECTION_COLOUR);
				}
			}
		}

		if (ImGui::Begin("Together Make"))
		{
			if (ImGui::BeginTabBar("Tabs"))
			{
				if (ImGui::BeginTabItem("Material"))
				{
					ImGui::ColorPicker4("Colour", (float*)&MainObject.Colour, ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreviewHalf);
					ImGui::DragFloat3("Sun Position", (float*)&GManager.LightPosition, 0.1f);
					ImGui::DragFloat3("Sun Colour", (float*)&GManager.LightColour, 0.1f);
					ImGui::DragFloat("Sun Range", &GManager.LightRange, 0.1f);

					// Draw fake sun while in this menu
					DrawList->AddCircle(GManager.WorldToScreenPosition(GManager.LightPosition), 10.0f, IM_COL32(208, 245, 0, 255));
					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem("Multiplayer"))
				{
					if (NManager.Server)
					{
						ImGui::Text("Connected");

						if (ImGui::Button("Disconnect"))
						{
							NManager.DisconnectFromServer();
						}
					}
					else
					{
						ImGui::Text("Not Connected...");
						ImGui::SameLine();
						if (ImGui::Button("Connect"))
						{
							if (!NManager.TryConnectToServer())
							{
								std::cout << "Failed to connect...\n";
							}
							else // connected, send nickname to server
							{
								Packet packet = Packet(PROVIDE_JOINER_INFO);
								appendString(packet.data, "Player");
								Samurai::sendNow(packet, NManager.Server); // TODO: REPLACE WITH CHOSEN NICKNAME
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
		GManager.IManager->ResetJustReleased();
	}
}