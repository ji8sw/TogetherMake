#pragma once
#include "GraphicsManager.h"

class Object
{
public:
	inline glm::mat4 GetModel() { return glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, 0)); }

	// The following turns a vertex local position using its model matrix
	inline glm::vec4 GetVertexWorldPosition(int VertexIndex)
	{
		glm::vec3 LocalVertexPos(CubeVertices[VertexIndex], CubeVertices[VertexIndex + 1], CubeVertices[VertexIndex + 2]);
		return GetModel() * glm::vec4(LocalVertexPos, 1.0f);
	}

	// To should be world position
	int GetClosestVertex(glm::vec3 To, float* OutDistance = nullptr)
	{
		int ClosestIndex = 0;
		float ClosestDistance = 100000.0f;

		for (int VertexIndex = 0; VertexIndex < CubeVertices.size(); VertexIndex += 5)
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

		for (int VertexIndex = 0; VertexIndex < CubeVertices.size(); VertexIndex += 5)
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
		glUseProgram(Manager.PrimaryShaderProgram);
		glBindVertexArray(Manager.VAO);

		glUniformMatrix4fv(3, 1, GL_FALSE, &Manager.View[0][0]); // layout(location = 3) uniform mat4 View;
		glUniformMatrix4fv(4, 1, GL_FALSE, glm::value_ptr(Manager.Projection)); // layout(location = 4) uniform mat4 Projection;

		glUniformMatrix4fv(2, 1, GL_FALSE, glm::value_ptr(GetModel())); // layout(location = 2) uniform mat4 Model
		glDrawArrays(GL_TRIANGLES, 0, 36);
	}
};