#pragma once
#include "GraphicsManager.h"

// contains vertex position and information related to network status
struct NetVertex
{
	glm::vec3 Position;
	glm::vec2 TextureCoordinates;
	bool IsGrabbed;

	// InVertexData: [x, y, z, u, v]
	static constexpr NetVertex FromRaw(std::vector<float> InVertexData)
	{
		NetVertex New = NetVertex();
		if (InVertexData.size() < 5) return New;
		New.Position.x = InVertexData[0];
		New.Position.y = InVertexData[1];
		New.Position.z = InVertexData[2];
		New.TextureCoordinates.x = InVertexData[3];
		New.TextureCoordinates.y = InVertexData[4];
		return New;
	}

	// InVertexDataList: [x, y, z, u, v] repeated
	static constexpr std::vector<NetVertex> FromRawList(std::vector<float> InVertexDataList)
	{
		std::vector<NetVertex> List = std::vector<NetVertex>();
		for (int Index = 0; Index < InVertexDataList.size(); Index += 5)
		{
			NetVertex New = NetVertex();
			New.Position.x = InVertexDataList[Index + 0];
			New.Position.y = InVertexDataList[Index + 1];
			New.Position.z = InVertexDataList[Index + 2];
			New.TextureCoordinates.x = InVertexDataList[Index + 3];
			New.TextureCoordinates.y = InVertexDataList[Index + 4];
			List.push_back(New);
		}
		return List;
	}

	// Return: [x, y, z, u, v] repeated
	static constexpr std::vector<float> ToRawList(std::vector<NetVertex> InVertexDataList)
	{
		std::vector<float> List = std::vector<float>();
		for (const NetVertex& Vertex : InVertexDataList)
		{
			List.push_back(Vertex.Position.x);
			List.push_back(Vertex.Position.y);
			List.push_back(Vertex.Position.z);
			List.push_back(Vertex.TextureCoordinates.x);
			List.push_back(Vertex.TextureCoordinates.y);
		}
		return List;
	}
};

class Object
{
public:
	ImVec4 Colour = ImVec4(0.9f, 0.9f, 0.9f, 1.0f);
	inline glm::mat4 GetModel() { return glm::translate(glm::mat4(1.0f), glm::vec3(0, 0, 0)); }
	std::vector<NetVertex> Vertices = NetVertex::FromRawList(CubeVertices);

	// for the following vertex usages, the * 5 is because our vertices are layed out as: [x, y, z, u, v]

	// The following turns a vertex local position into its world position using its model matrix
	inline glm::vec4 GetVertexWorldPosition(int VertexIndex)
	{
		return GetModel() * glm::vec4(Vertices[VertexIndex].Position, 1.0f);
	}

	// The following turns a vertex local position into its world position using its model matrix
	inline glm::vec4 GetVertexWorldPosition(const NetVertex& Vertex)
	{
		return GetModel() * glm::vec4(Vertex.Position, 1.0f);
	}

	// sets the vertex world position
	void SetVertexWorldPosition(int VertexIndex, glm::vec4 NewWorldPosition)
	{
		glm::vec4 LocalPosition = glm::inverse(GetModel()) * NewWorldPosition;
		Vertices[VertexIndex].Position = LocalPosition;
	}

	// To should be world position
	int GetClosestVertexToRay(glm::vec3 RayOrigin, glm::vec3 RayDirection, float MaxDistance = 0.1f, float* OutDistance = nullptr)
	{
		int ClosestIndex = -1;
		float ClosestDistToRay = MaxDistance;

		for (int Index = 0; Index < Vertices.size(); Index++)
		{
			glm::vec3 VertexPos = GetVertexWorldPosition(Index);

			// project vertex onto ray, find closest point on ray
			glm::vec3 RayToPoint = VertexPos - RayOrigin;
			float Length = glm::dot(RayToPoint, RayDirection); // projection length
			glm::vec3 ClosestPoint = RayOrigin + RayDirection * Length;

			// distance from vertex to closest point on ray
			float DistToRay = glm::distance(VertexPos, ClosestPoint);

			if (DistToRay < ClosestDistToRay)
			{
				ClosestDistToRay = DistToRay;
				ClosestIndex = Index;
			}
		}

		if (OutDistance) *OutDistance = ClosestDistToRay;
		return ClosestIndex;
	}

	// To should be world position
	glm::vec4 GetClosestVertexPosition(glm::vec3 To, float* OutDistance = nullptr)
	{
		glm::vec4 Closest;
		float ClosestDistance = 100000.0f;

		for (int VertexIndex = 0; VertexIndex < Vertices.size(); VertexIndex++)
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

	// uploads vertices to gpu
	inline void UploadVertices(GraphicsManager::Manager& Manager)
	{
		glBindBuffer(GL_ARRAY_BUFFER, Manager.VBO);
		auto RawVertices = NetVertex::ToRawList(Vertices);
		glBufferData(GL_ARRAY_BUFFER, RawVertices.size() * sizeof(float), RawVertices.data(), GL_DYNAMIC_DRAW);
	}

	void Render(GraphicsManager::Manager& Manager)
	{
		UploadVertices(Manager); // in future: only upload when changed

		glUseProgram(Manager.PrimaryShaderProgram);
		glBindVertexArray(Manager.VAO);

		// Camera
		glUniformMatrix4fv(glGetUniformLocation(Manager.PrimaryShaderProgram, "View"), 1, GL_FALSE, &Manager.View[0][0]); // layout(location = 3) uniform mat4 View;
		glUniformMatrix4fv(glGetUniformLocation(Manager.PrimaryShaderProgram, "Projection"), 1, GL_FALSE, glm::value_ptr(Manager.Projection));

		// Material
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