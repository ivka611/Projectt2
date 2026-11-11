#version 330 core
in vec4 Color;
out vec4 FragColor;
uniform float alpha;
void main()
{
	FragColor = vec4(Color.rgb, alpha);
}