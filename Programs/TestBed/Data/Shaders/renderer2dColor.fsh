#ifdef GL_ES
// define default precision for float, vec, mat.
precision highp float;
#else
#define lowp
#define highp
#define mediump
#endif

uniform lowp vec4 flatColor;

void main()
{
	gl_FragColor = flatColor;
}
