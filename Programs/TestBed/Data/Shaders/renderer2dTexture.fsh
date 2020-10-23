#ifdef GL_ES
// define default precision for float, vec, mat.
precision highp float;
#else
#define lowp
#define highp
#define mediump
#endif

uniform sampler2D sampler2d;
varying lowp vec4 varColor;
varying mediump vec2 varTexCoord;

void main()
{
    lowp vec4 texColor = texture2D(sampler2d, varTexCoord);
#ifdef ALPHA_TEST_ENABLED
    if (texColor.a < 0.9)
        discard;
#endif

#ifdef IMAGE_A8

#ifdef ADD_COLOR
	gl_FragColor = vec4(varColor.rgb, texColor.a + varColor.a);
#else
    gl_FragColor = vec4(varColor.rgb, texColor.a * varColor.a);
#endif //ADD_COLOR

#else

#ifdef ADD_COLOR
	gl_FragColor = texColor + varColor;
#else
    gl_FragColor = texColor * varColor;
#endif //ADD_COLOR

#endif //IMAGE_A8
}
