#ifdef GL_ES
// define default precision for float, vec, mat.
precision highp float;
#else
#define lowp
#define highp
#define mediump
#endif


attribute vec4 inPosition;
attribute vec2 inTexCoord0;

uniform mat4 worldViewProjMatrix;
varying mediump vec2 varTexCoord;

void main()
{
	gl_Position = worldViewProjMatrix * inPosition;
	varTexCoord = inTexCoord0;
}
