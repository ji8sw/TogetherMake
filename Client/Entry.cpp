#include "NetManager.h"
#include "GraphicsManager.h"
#include "Object.h"
#include "StoredAction.h"

#ifdef _DEBUG
#include <iostream>
#include <format>
#endif
#define SELECTION_COLOUR IM_COL32(255, 0, 0, 255)
#define OTHER_PLAYER_SELECTION_COLOUR IM_COL32(255, 129, 0, 255)
#define SELECTION_MAX_DISTANCE 1.25f // when selecting things, if the closest is X away, nothing will be selected
#define SMOOTH_MOVE // syncs vertices every frame instead of when changes are confirmed
#define MAKETOGETHER_VERSION 1

enum EMode
{
	None,
	Move
};

NetManager::Manager NManager;
Object MainObject;
int SelectedVertexIndex = -1;
EMode SelectedMode = None;

class MoveVertexAction : public StoredActionData
{
public:

	int VertexIndex = INVALID_INT;
	glm::vec3 From;
	glm::vec3 To;
	glm::vec2 MouseStart;
	bool Finished = false;

	StoredActionData* Execute(bool Networked)
	{
		if (VertexIndex == INVALID_INT) return this;

		MainObject.SetVertexWorldPosition(VertexIndex, glm::vec4(To, 1.0f));

		if (Networked && NManager.Server)
			NManager.SendUpdateVertexPosition(VertexIndex, To);

		return this;
	}

	StoredActionData* Reverse(bool Networked)
	{
		if (VertexIndex == INVALID_INT) return this;

		MainObject.SetVertexWorldPosition(VertexIndex, glm::vec4(From, 1.0f));

		if (Networked && NManager.Server)
			NManager.SendUpdateVertexPosition(VertexIndex, From);

		return this;
	}

	MoveVertexAction(int InVertexIndex, glm::vec2 InMouseStart) :
		VertexIndex(InVertexIndex)
	{
		MouseStart = InMouseStart;
		From = MainObject.GetVertexWorldPosition(InVertexIndex);
	}
};

MoveVertexAction* MoveVertexData = nullptr;
std::vector<StoredActionData*> PreviousActions;

int main()
{
	// Initialize local server, we won't connect yet.
	if (!NetManager::InitializeENet()) throw "Failed to start.";
	NManager = NetManager::Manager();
	if (!NManager.TryCreateLocalServer()) throw "Failed to start.";

	// Initialize OpenGL (creates window and whatnot)
	GraphicsManager::Manager GManager = GraphicsManager::Manager();
	if (!GManager.Initialize()) throw "Failed to start.";
	GManager.SetupStandardShaders();
	NManager.MainObject = &MainObject;
	GManager.UpdateCameraOrbit(glm::vec3(0, 0, 0));
	std::string NameInput = "Player";

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
			MoveVertexData = new MoveVertexAction(SelectedVertexIndex, glm::vec2(MousePosition.x, MousePosition.y));
		}

		if (GManager.IManager->Keys[ESCAPE].JustReleased) // Escape: cancel changes, de-select vertices
		{
			switch (SelectedMode)
			{
			case Move:
			{
				if (SelectedVertexIndex == INVALID_INT) SelectedMode = None;

#ifdef SMOOTH_MOVE
				MoveVertexData->Reverse(true);
#else
				MoveVertexData->Reverse(false);
#endif
				break;
			}
			case None:
			{
				SelectedVertexIndex = INVALID_INT;
				break;
			}
			}
			SelectedMode = None;
		}

		if (GManager.IManager->Keys[ENTER].JustReleased && SelectedMode != None && MainObject.VertexDoesExist(SelectedVertexIndex)) // Enter: confirm changes
		{
			switch (SelectedMode)
			{
			case Move:
			{
				NManager.SendUpdateVertexPosition(SelectedVertexIndex, MainObject.Vertices[SelectedVertexIndex].Position);
				MoveVertexData->Finished = true;
				MoveVertexData->To = MainObject.Vertices[SelectedVertexIndex].Position;
				PreviousActions.push_back(MoveVertexData);
				break;
			}
			case None:
			{
				break;
			}
			}
			SelectedMode = None;
		}

		if (GManager.IManager->Keys[CTRL].JustReleased && GManager.IManager->Keys[Z].JustReleased) // CTRL + Z: Reverse action / undo
		{
			if (!PreviousActions.empty()) PreviousActions[PreviousActions.size() - 1]->Reverse();
		}

		switch (SelectedMode)
		{
			case Move:
			{
				if (SelectedVertexIndex == INVALID_INT) { SelectedMode = None; break; }
				glm::vec2 MouseDelta = glm::vec2(MousePosition.x, MousePosition.y) - MoveVertexData->MouseStart;

				glm::vec3 CameraRight = glm::vec3(glm::inverse(GManager.View)[0]); // x

				float Sensitivity = 0.00007f;
				glm::vec2 Scale = GManager.GetWorldPerPixel(MoveVertexData->From);

				// convert screen space to world space movement (delta)
				glm::vec3 Change =
					(-MouseDelta.x * -CameraRight / Scale.x +
						-MouseDelta.y * GManager.CameraUp / Scale.y) * Sensitivity;

				glm::vec3 NewWorldPosition = MoveVertexData->From + Change;

				if (GManager.IManager->Keys[S].JustReleased) // S: snap to closest vertex
				{ // we will find closest vertex and go to it
					int ClosestVertex = MainObject.GetClosestVertexToPosition(NewWorldPosition, 100.0f, SelectedVertexIndex);
					if (MainObject.VertexDoesExist(ClosestVertex))
					{
						NewWorldPosition = MainObject.Vertices[ClosestVertex].Position;
						SelectedMode = None;

#ifndef SMOOTH_MOVE // if its not defined, the vertex movement change wont be networked
						NManager.SendUpdateVertexPosition(SelectedVertexIndex, NewWorldPosition);
#endif
					}
				}

				MainObject.SetVertexWorldPosition(SelectedVertexIndex, glm::vec4(NewWorldPosition, 1.0f));

#ifdef SMOOTH_MOVE
				NManager.SendUpdateVertexPosition(SelectedVertexIndex, NewWorldPosition);
#endif
				break;
			}
		}

		MainObject.Render(GManager);

		if (GManager.IManager->Keys[LEFT_MOUSE].JustReleased && SelectedMode == None) // Left click: select vertex
		{
			glm::vec3 RayOrigin = GManager.CameraPosition; // Or extract from View matrix
			glm::vec3 RayDirection = GManager.ScreenToWorldPosition(MousePosition);
			int DesiredVertexIndex = MainObject.GetClosestVertexToRay(RayOrigin, RayDirection, SELECTION_MAX_DISTANCE);
			if (DesiredVertexIndex != INVALID_INT && MainObject.VertexDoesExist(DesiredVertexIndex))
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
			else if (SelectedVertexIndex != INVALID_INT) // the player has a vertex selected but clicked away, probably to stop selecting.
			{
				SelectedVertexIndex = INVALID_INT;
				SelectedMode = None;
			}
		}

		if (MainObject.VertexDoesExist(SelectedVertexIndex))
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
					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem("Sun"))
				{
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

						ImGui::InputText("Nickname", &NameInput);

						if (ImGui::Button("Connect"))
						{
							if (!NManager.TryConnectToServer())
							{
								std::cout << "Failed to connect...\n";
							}
							else // connected, send nickname to server
							{
								Packet packet = Packet(PROVIDE_JOINER_INFO);
								appendString(packet.data, NameInput.c_str());
								appendInt(packet.data, MAKETOGETHER_VERSION);
								appendString(packet.data, ""); // password
								Samurai::sendNow(packet, NManager.Server); // TODO: REPLACE WITH CHOSEN NICKNAME

								Packet SyncVerticesPacket = Packet(REQUEST_VERTICES);
								Samurai::sendNow(SyncVerticesPacket, NManager.Server);
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

	//GManager.Cleanup();
	for (auto Action : PreviousActions)
		delete Action;
	NManager.Cleanup();
}