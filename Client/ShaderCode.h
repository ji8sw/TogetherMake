#pragma once
const char* StandardVertexShaderSource =
R"(
#version 330 core
#extension GL_ARB_explicit_uniform_location : enable
layout(location = 0) in vec3 VertexPosition;
layout (location = 1) in vec3 Normal;
layout (location = 2) in vec2 TexCoord;

out vec3 FragPosition;
out vec3 FragNormal;
out vec2 FragTexCoord;

uniform mat4 Model;
uniform mat4 View;
uniform mat4 Projection;

void main()
{
    FragPosition = vec3(Model * vec4(VertexPosition, 1.0));
    FragNormal = mat3(transpose(inverse(Model))) * Normal;
    FragTexCoord = TexCoord;

	gl_Position = Projection * View * Model * vec4(VertexPosition, 1.0);
};
)";

const char* StandardFragmentShaderSource =
R"(
#version 330 core
#extension GL_ARB_explicit_uniform_location : enable
in vec3 FragPosition;
in vec3 FragNormal;
in vec2 FragTexCoord;

uniform mat4 Model;
uniform vec4 Colour;
uniform vec3 LightPosition;
uniform vec3 LightColour;
uniform vec3 ViewPosition;
uniform float LightRange;
const float Shine = 32.0f;

out vec4 FragColor;

void main()
{
    // Distance and Attenuation
    float Distance = length(LightPosition - FragPosition);
    float Attenuation = 1.0 / (1.0 + (Distance * Distance) / (LightRange * LightRange));

    float AmbientStrength = 0.1;
    vec3 Ambient = AmbientStrength * LightColour;

    // Diffuse
    vec3 Normal = normalize(FragNormal);
	vec3 LightDirection = normalize(LightPosition - FragPosition);
    float Diff = max(dot(Normal, LightDirection), 0.0f);
    vec3 Diffuse = Diff * LightColour * Colour.xyz;
    
    // Specular
    vec3 ViewDirection = normalize(ViewPosition - FragPosition);
    vec3 HalfwayDirection = normalize(LightDirection + ViewDirection);
    float Spec = pow(max(dot(Normal, HalfwayDirection), 0.0), Shine);
    vec3 Specular = LightColour * Spec * 0.3;
    
    // Apply attenuation
    Diffuse *= Attenuation;
    Specular *= Attenuation;
    
    FragColor = vec4(Ambient + Diffuse + Specular, 1.0);
}
)";