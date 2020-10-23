package com.dava.engine;

import java.util.Vector;

import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLDisplay;
import android.opengl.GLSurfaceView;
import android.util.Log;

public class GLConfigChooser implements GLSurfaceView.EGLConfigChooser {
	private static final int EGL_RENDERABLE_TYPE = 0x3040;
	private static final int EGL_OPENGL_ES2_BIT = 0x0004;
	
	private static final int EGL_DEPTH_ENCODING_NV = 0x30E2;
	private static final int EGL_DEPTH_ENCODING_NONLINEAR_NV = 0x30E3;

    /** The number of bits requested for the red component */
    protected int redSize = 8;
    /** The number of bits requested for the green component */
    protected int greenSize = 8;
    /** The number of bits requested for the blue component */
    protected int blueSize = 8;
    /** The number of bits requested for the alpha component */
    protected int alphaSize = 8;
    /** The number of bits requested for the stencil component */
    protected int stencilSize = 8;
    /** The number of bits requested for the depth component */
    protected int depthSize = 24;
	
    private static int curDepthBufferSize = 0;
    public static int GetDepthBufferSize() {
    	return curDepthBufferSize;
    }

	public EGLConfig chooseConfig(EGL10 egl, EGLDisplay display) {
		int[] version = new int[2];
		boolean ret = egl.eglInitialize(display, version);
		if (!ret) {
			return null;
		}
		if (egl.eglGetError() != EGL10.EGL_SUCCESS) {
			return null;
		}

		Vector<Integer> baseConfig = new Vector<Integer>();
		baseConfig.add(EGL_RENDERABLE_TYPE);		baseConfig.add(EGL_OPENGL_ES2_BIT);
		baseConfig.add(EGL10.EGL_RED_SIZE);			baseConfig.add(redSize);
		baseConfig.add(EGL10.EGL_GREEN_SIZE);		baseConfig.add(greenSize);
		baseConfig.add(EGL10.EGL_BLUE_SIZE);		baseConfig.add(blueSize);
		baseConfig.add(EGL10.EGL_ALPHA_SIZE);		baseConfig.add(alphaSize);
		baseConfig.add(EGL10.EGL_STENCIL_SIZE);		baseConfig.add(stencilSize);

		EGLConfig eglConfig = null;
		{ //try initialize 24 bit depth buffer
			Vector<Integer> depth24bit = new Vector<Integer>();
			depthSize = 24;
			depth24bit.add(EGL10.EGL_DEPTH_SIZE);	depth24bit.add(depthSize);
			eglConfig = chooseConfig(egl, display, baseConfig, depth24bit);
		}
		if (eglConfig == null) { //try initialize 16 bit depth buffer with NVidia extension
			depthSize = 16;
			Vector<Integer> nvConfig = new Vector<Integer>();
			nvConfig.add(EGL10.EGL_DEPTH_SIZE);		nvConfig.add(16);
			nvConfig.add(EGL_DEPTH_ENCODING_NV);	nvConfig.add(EGL_DEPTH_ENCODING_NONLINEAR_NV);
			eglConfig = chooseConfig(egl, display, baseConfig, nvConfig);
		}
		if (eglConfig == null) { //worst case only 16 bit depth buffer
			depthSize = 16;
			Vector<Integer> depth16bit = new Vector<Integer>();
			depth16bit.add(EGL10.EGL_DEPTH_SIZE);	depth16bit.add(16);
			eglConfig = chooseConfig(egl, display, baseConfig, depth16bit);
		}
		
		if (eglConfig == null) {
			Log.e(DavaActivity.LOG_TAG, "Error initialize gl");
			curDepthBufferSize = 0;
			return null;
		}
		
		curDepthBufferSize = depthSize;
		printConfig(egl, display, eglConfig);
		return eglConfig;
	}
	
	private EGLConfig chooseConfig(EGL10 egl, EGLDisplay display, Vector<Integer> baseConfig, Vector<Integer> specificConfig) {
		int[] configAttrs = new int[baseConfig.size() + ((specificConfig == null) ? 0 : specificConfig.size()) + 1];
		for (int i = 0; i < baseConfig.size(); ++i) {
			configAttrs[i] = baseConfig.get(i);
		}
		if (specificConfig != null) {
			for (int i = 0; i < specificConfig.size(); ++i) {
				configAttrs[baseConfig.size() + i] = specificConfig.get(i);
			}
		}
		configAttrs[configAttrs.length - 1] = EGL10.EGL_NONE;
		
		final EGLConfig[] config = new EGLConfig[20];
		int num_configs[] = new int[1];
		egl.eglChooseConfig(display, configAttrs, config, config.length, num_configs);
		if (egl.eglGetError() != EGL10.EGL_SUCCESS) {
			Log.w(DavaActivity.LOG_TAG, "eglChooseConfig err: " + egl.eglGetError());
			return null;
		}

		EGLConfig eglConfig = null;
		int score = 1 << 24; 	// to make sure even worst score is better than
								// this, like 8888 when request 565...
		int val[] = new int[1];
		for (int i = 0; i < num_configs[0]; i++) {
			int currScore = 0;
			int r, g, b, a, d, s;
			egl.eglGetConfigAttrib(display, config[i], EGL10.EGL_RED_SIZE, val);	r = val[0];
			egl.eglGetConfigAttrib(display, config[i], EGL10.EGL_GREEN_SIZE, val);	g = val[0];
			egl.eglGetConfigAttrib(display, config[i], EGL10.EGL_BLUE_SIZE, val);	b = val[0];
			egl.eglGetConfigAttrib(display, config[i], EGL10.EGL_ALPHA_SIZE, val);	a = val[0];
			egl.eglGetConfigAttrib(display, config[i], EGL10.EGL_DEPTH_SIZE, val);	d = val[0];
			egl.eglGetConfigAttrib(display, config[i], EGL10.EGL_STENCIL_SIZE, val);s = val[0];

			currScore = (Math.abs(r - redSize) + Math.abs(g - greenSize) + Math.abs(b - blueSize) + Math.abs(a - alphaSize)) << 16;
			currScore += Math.abs(d - depthSize) << 8;
			currScore += Math.abs(s - stencilSize);

			if (currScore < score) {
				Log.w(DavaActivity.LOG_TAG, "--------------------------");
				Log.w(DavaActivity.LOG_TAG, "New config chosen: " + i);
				/*
				 * for (int j = 0; j < (configAttrs.length-1)>>1; j++) {
				 * egl.eglGetConfigAttrib(display, config[i], configAttrs[j*2],
				 * val); if (val[0] >= configAttrs[j*2+1]) {
				 * Log.w(JNIConst.LOG_TAG, "setting " + j + ", matches: " +
				 * val[0]); } }
				 */

				score = currScore;
				eglConfig = config[i];
			}
		}

		return eglConfig;
	}

	public static void printConfig(EGL10 egl, EGLDisplay display,
			EGLConfig config) {
		int[] attributes = {
				EGL10.EGL_BUFFER_SIZE,
				EGL10.EGL_ALPHA_SIZE,
				EGL10.EGL_BLUE_SIZE,
				EGL10.EGL_GREEN_SIZE,
				EGL10.EGL_RED_SIZE,
				EGL10.EGL_DEPTH_SIZE,
				EGL10.EGL_STENCIL_SIZE,
				EGL10.EGL_CONFIG_CAVEAT,
				EGL10.EGL_CONFIG_ID,
				EGL10.EGL_LEVEL,
				EGL10.EGL_MAX_PBUFFER_HEIGHT,
				EGL10.EGL_MAX_PBUFFER_PIXELS,
				EGL10.EGL_MAX_PBUFFER_WIDTH,
				EGL10.EGL_NATIVE_RENDERABLE,
				EGL10.EGL_NATIVE_VISUAL_ID,
				EGL10.EGL_NATIVE_VISUAL_TYPE,
				0x3030, // EGL10.EGL_PRESERVED_RESOURCES,
				EGL10.EGL_SAMPLES,
				EGL10.EGL_SAMPLE_BUFFERS,
				EGL10.EGL_SURFACE_TYPE,
				EGL10.EGL_TRANSPARENT_TYPE,
				EGL10.EGL_TRANSPARENT_RED_VALUE,
				EGL10.EGL_TRANSPARENT_GREEN_VALUE,
				EGL10.EGL_TRANSPARENT_BLUE_VALUE,
				0x3039, // EGL10.EGL_BIND_TO_TEXTURE_RGB,
				0x303A, // EGL10.EGL_BIND_TO_TEXTURE_RGBA,
				0x303B, // EGL10.EGL_MIN_SWAP_INTERVAL,
				0x303C, // EGL10.EGL_MAX_SWAP_INTERVAL,
				EGL10.EGL_LUMINANCE_SIZE,
				EGL10.EGL_ALPHA_MASK_SIZE,
				EGL10.EGL_COLOR_BUFFER_TYPE,
				EGL10.EGL_RENDERABLE_TYPE,
				0x3042 // EGL10.EGL_CONFORMANT
		};
		String[] names = {
				"EGL_BUFFER_SIZE",
				"EGL_ALPHA_SIZE",
				"EGL_BLUE_SIZE",
				"EGL_GREEN_SIZE",
				"EGL_RED_SIZE",
				"EGL_DEPTH_SIZE",
				"EGL_STENCIL_SIZE",
				"EGL_CONFIG_CAVEAT",
				"EGL_CONFIG_ID",
				"EGL_LEVEL",
				"EGL_MAX_PBUFFER_HEIGHT",
				"EGL_MAX_PBUFFER_PIXELS",
				"EGL_MAX_PBUFFER_WIDTH",
				"EGL_NATIVE_RENDERABLE",
				"EGL_NATIVE_VISUAL_ID",
				"EGL_NATIVE_VISUAL_TYPE",
				"EGL_PRESERVED_RESOURCES",
				"EGL_SAMPLES",
				"EGL_SAMPLE_BUFFERS",
				"EGL_SURFACE_TYPE",
				"EGL_TRANSPARENT_TYPE",
				"EGL_TRANSPARENT_RED_VALUE",
				"EGL_TRANSPARENT_GREEN_VALUE",
				"EGL_TRANSPARENT_BLUE_VALUE",
				"EGL_BIND_TO_TEXTURE_RGB",
				"EGL_BIND_TO_TEXTURE_RGBA",
				"EGL_MIN_SWAP_INTERVAL",
				"EGL_MAX_SWAP_INTERVAL",
				"EGL_LUMINANCE_SIZE",
				"EGL_ALPHA_MASK_SIZE",
				"EGL_COLOR_BUFFER_TYPE",
				"EGL_RENDERABLE_TYPE",
				"EGL_CONFORMANT" };

		int[] value = new int[1];
		for (int i = 0; i < attributes.length; i++) {
			int attribute = attributes[i];
			String name = names[i];
			if (egl.eglGetConfigAttrib(display, config, attribute, value)) {
				Log.w(DavaActivity.LOG_TAG, String.format("  %s: %d\n", name, value[0]));
			} else {
				Log.w(DavaActivity.LOG_TAG, String.format("  %s: failed\n", name));
				while (egl.eglGetError() != EGL10.EGL_SUCCESS);
			}
		}
	}
}
