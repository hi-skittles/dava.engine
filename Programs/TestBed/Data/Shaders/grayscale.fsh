#ifdef GL_ES
// define default precision for float, vec, mat.
precision highp float;
#else
#define lowp
#define highp
#define mediump
#endif

uniform sampler2D sampler2d;
uniform mediump float alphaMultiplier;
varying mediump vec2 varTexCoord;

void main()
{
    lowp vec4 texColor = texture2D(sampler2d, varTexCoord);
#ifdef ALPHA_TEST_ENABLED
    if (texColor.a < 0.9)
        discard;
#endif
	float gray = dot(texColor.rgb, vec3(0.3, 0.59, 0.11));
	gl_FragColor = vec4(gray, gray, gray, texColor.a * alphaMultiplier);
}
