#ifdef GL_ES
// define default precision for float, vec, mat.
precision highp float;
#else
#define lowp
#define highp
#define mediump
#endif


attribute vec4 inPosition;
#ifdef VERTEX_COLOR
attribute vec4 inColor;
#endif
attribute vec2 inTexCoord0;

uniform mat4 worldViewProjMatrix;
#ifndef VERTEX_COLOR
uniform lowp vec4 flatColor;
#endif
varying lowp vec4 varColor;
varying mediump vec2 varTexCoord;

void main()
{
	gl_Position = worldViewProjMatrix * inPosition;
#ifdef VERTEX_COLOR
	varColor = inColor;
#else
	varColor = flatColor;
#endif
	varTexCoord = inTexCoord0;
}
