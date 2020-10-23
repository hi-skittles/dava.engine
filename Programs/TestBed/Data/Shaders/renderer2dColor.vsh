#ifdef GL_ES
// define default precision for float, vec, mat.
precision highp float;
#else
#define lowp
#define highp
#define mediump
#endif

attribute vec4 inPosition;

uniform mat4 worldViewProjMatrix;

void main()
{
	gl_Position = worldViewProjMatrix * inPosition;	
}
