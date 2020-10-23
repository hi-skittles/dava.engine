#ifdef GL_ES
// define default precision for float, vec, mat.
precision highp float;
#else
#define lowp
#define highp
#define mediump
#endif

uniform mat4 worldViewProjMatrix;

attribute vec4 inPosition;
attribute vec2 inTexCoord0;

varying mediump vec2 v_texCoord;

void main() {
    gl_Position = worldViewProjMatrix * inPosition;
    v_texCoord = inTexCoord0;
}