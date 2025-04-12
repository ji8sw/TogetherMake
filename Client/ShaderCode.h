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
const float Shine = 16.0f;

out vec4 FragColor;

void main()
{
    float AmbientStrength = 0.3;
    vec3 Ambient = AmbientStrength * LightColour;

	vec3 DiffuseLight = Ambient;
	vec3 SpecularLight = vec3(0);
	vec3 SurfaceNormal = normalize(FragNormal);

	vec3 ViewDirection = normalize(ViewPosition - FragPosition);
    
    vec3 DirectionToLight = LightPosition - FragPosition;

	// Light Falloff
	float Distance = length(DirectionToLight);
	float RangeAttenuation = clamp(1.0 - (Distance / LightRange), 0.0, 1.0);
	RangeAttenuation *= RangeAttenuation; // optional: smooth falloff
	float Attenuation = RangeAttenuation / (Distance * Distance + 0.01);

	DirectionToLight = normalize(DirectionToLight);

	float CosAngleIncidence = max(dot(normalize(SurfaceNormal), DirectionToLight), 0);
	vec3 Intensity = LightColour * Attenuation;

	DiffuseLight += Intensity * CosAngleIncidence;

	// Specular Lighting
	vec3 HalfAngle = normalize(DirectionToLight + ViewDirection);
	float BlinnTerm = dot(SurfaceNormal, HalfAngle);
	BlinnTerm = clamp(BlinnTerm, 0, 1);
	BlinnTerm = pow(BlinnTerm, Shine); // Higher = Sharper Highlight
	SpecularLight += Intensity * BlinnTerm;

	FragColor = vec4((DiffuseLight * Colour.xyz + SpecularLight * Colour.xyz), 1.0f);
}
)";