#version 330 core
layout (location = 0) in vec3 aPos;			//vertex position
layout (location = 1) in vec2 aTexCoords;	
layout (location = 2) in vec3 aGrassPosition;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

out vec2 TexCoords;

void main()
{
	//Billboard transformations
	vec3 cameraRight = vec3(view[0][0], view[1][0], view[2][0]);
	vec3 cameraUp = vec3(view[0][1], view[1][1], view[2][1]);

	vec3 vertexPosition = aGrassPosition + aPos.x * cameraRight + aPos.y * cameraUp;

	gl_Position = projection * view * model * vec4(vertexPosition, 1.0);
	TexCoords = aTexCoords; 
}