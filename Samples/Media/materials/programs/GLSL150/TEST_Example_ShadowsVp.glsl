
#version 330

in vec4 vertex;

out float psDepth;
uniform float shadowConstantBias;


uniform mat4x4 worldViewProj;
uniform vec4 depthRange0;


void main()
{
	gl_Position	= worldViewProj * vertex;

	//Linear depth
    psDepth	= (gl_Position.z - depthRange0.x + shadowConstantBias) * depthRange0.w;

	//We can't make the depth buffer linear without Z out in the fragment shader;
	//however we can use a cheap approximation ("pseudo linear depth")
	//see http://yosoygames.com.ar/wp/2014/01/linear-depth-buffer-my-ass/
	gl_Position.z = gl_Position.z * (gl_Position.w * depthRange0.w);
}
