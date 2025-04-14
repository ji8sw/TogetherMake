#pragma once
#include "GraphicsManager.h"

class Object
{
public:
	ImVec4 Colour = ImVec4(0.9f, 0.9f, 0.9f, 1.0f);
	inline glm::mat4 GetModel() { return glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, 0)); }
	std::vector<float> Vertices = CubeVertices;

	// for the following vertex usages, the * 5 is because our vertices are layed out as: [x, y, z, u, v]

	// The following turns a vertex local position using its model matrix
	inline glm::vec4 GetVertexWorldPosition(int VertexIndex)
	{
		glm::vec3 LocalVertexPos(Vertices[VertexIndex], Vertices[VertexIndex + 1], Vertices[VertexIndex + 2]);
		return GetModel() * glm::vec4(LocalVertexPos, 1.0f);
	}

	// sets the vertex world position
	void SetVertexWorldPosition(int VertexIndex, glm::vec4 NewWorldPosition)
	{
		int DesiredVertex = VertexIndex * 5;
		if (DesiredVertex + 2 > Vertices.size()) return;
		glm::vec4 LocalPosition = glm::inverse(GetModel()) * NewWorldPosition;

		Vertices[DesiredVertex + 0] = LocalPosition.x;
		Vertices[DesiredVertex + 1] = LocalPosition.y;
		Vertices[DesiredVertex + 2] = LocalPosition.z;
	}

	// To should be world position
	int GetClosestVertex(glm::vec3 To, float* OutDistance = nullptr)
	{
		int ClosestIndex = 0;
		float ClosestDistance = 100000.0f;

		for (int VertexIndex = 0; VertexIndex < Vertices.size(); VertexIndex += 5)
		{
			auto VertexPosition = GetVertexWorldPosition(VertexIndex);
			float Distance = glm::distance(glm::vec3(VertexPosition), To);
			if (Distance < ClosestDistance)
			{
				ClosestIndex = VertexIndex;
				ClosestDistance = Distance;
			}
		}

		if (OutDistance) *OutDistance = ClosestDistance;
		return ClosestIndex;
	}

	// To should be world position
	glm::vec4 GetClosestVertexPosition(glm::vec3 To, float* OutDistance = nullptr)
	{
		glm::vec4 Closest;
		float ClosestDistance = 100000.0f;

		for (int VertexIndex = 0; VertexIndex < Vertices.size(); VertexIndex += 5)
		{
			auto VertexPosition = GetVertexWorldPosition(VertexIndex);
			float Distance = glm::distance(glm::vec3(VertexPosition), To);
			if (Distance < ClosestDistance)
			{
				Closest = VertexPosition;
				ClosestDistance = Distance;
			}
		}

		if (OutDistance) *OutDistance = ClosestDistance;
		return Closest;
	}

	void Render(GraphicsManager::Manager& Manager)
	{
		glBindBuffer(GL_ARRAY_BUFFER, Manager.VBO);
		glBufferData(GL_ARRAY_BUFFER, Vertices.size() * sizeof(float), Vertices.data(), GL_DYNAMIC_DRAW);

		glUseProgram(Manager.PrimaryShaderProgram);
		glBindVertexArray(Manager.VAO);

		glUniformMatrix4fv(glGetUniformLocation(Manager.PrimaryShaderProgram, "View"), 1, GL_FALSE, &Manager.View[0][0]); // layout(location = 3) uniform mat4 View;
		glUniformMatrix4fv(glGetUniformLocation(Manager.PrimaryShaderProgram, "Projection"), 1, GL_FALSE, glm::value_ptr(Manager.Projection));
		glUniform4fv(glGetUniformLocation(Manager.PrimaryShaderProgram, "Colour"), 1, &Colour.x);

		// Lighting
		glUniform3fv(glGetUniformLocation(Manager.PrimaryShaderProgram, "LightPosition"), 1, &Manager.LightPosition.x);
		glUniform3fv(glGetUniformLocation(Manager.PrimaryShaderProgram, "LightColour"), 1, &Manager.LightColour.x);
		glUniform3fv(glGetUniformLocation(Manager.PrimaryShaderProgram, "ViewPosition"), 1, &Manager.CameraPosition.x);
		glUniform1fv(glGetUniformLocation(Manager.PrimaryShaderProgram, "LightRange"), 1, &Manager.LightRange);

		glUniformMatrix4fv(glGetUniformLocation(Manager.PrimaryShaderProgram, "Model"), 1, GL_FALSE, glm::value_ptr(GetModel())); // layout(location = 2) uniform mat4 Model
		glDrawArrays(GL_TRIANGLES, 0, 36);
	}
};