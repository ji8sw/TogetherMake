#pragma once
const char* StandardVertexShaderSource =
R"(
#version 330 core
#extension GL_ARB_explicit_uniform_location : enable
layout(location = 0) in vec3 VertexPosition;

out vec3 FragmentPosition;
out vec2 TextureCoordinates;

layout(location = 2) uniform mat4 Model;
layout(location = 3) uniform mat4 View;
layout(location = 4) uniform mat4 Projection;

void main()
{
    FragmentPosition = VertexPosition;

	gl_Position = Projection * View * Model * vec4(VertexPosition, 1.0);
};
)";

const char* StandardFragmentShaderSource =
R"(
#version 330 core
#extension GL_ARB_explicit_uniform_location : enable
out vec4 FragColor;

in vec3 FragmentPosition;
uniform vec4 Colour;

void main()
{
    FragColor = Colour;
}
)";