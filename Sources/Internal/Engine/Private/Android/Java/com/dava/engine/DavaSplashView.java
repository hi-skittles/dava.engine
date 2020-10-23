package com.dava.engine;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.opengl.GLSurfaceView;
import android.opengl.GLUtils;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;
import java.nio.ShortBuffer;

// Class responsible for drawing splash image & collecting device info related to rendering
public class DavaSplashView extends GLSurfaceView
{
    /* from RenderBase.h
    enum eGPUFamily : uint8
    {
        GPU_POWERVR_IOS = 0,
        GPU_POWERVR_ANDROID,
        GPU_TEGRA,
        GPU_MALI,
        GPU_ADRENO,
        GPU_DX11,
        GPU_ORIGIN, // not a device - for development only
        GPU_FAMILY_COUNT,

        GPU_DEVICE_COUNT = GPU_ORIGIN,
        GPU_INVALID = 127
    };
    */

    public static byte GPU_POWERVR_ANDROID = 1;
    public static byte GPU_TEGRA = 2;
    public static byte GPU_MALI = 3;
    public static byte GPU_ADRENO = 4;
    public static byte GPU_INVALID = 127;

    private Renderer renderer;

    private static class Renderer implements GLSurfaceView.Renderer
    {
        private Bitmap powerOfTwoBitmap; // Bitmap, passed to the ctor, and converted to power of two, used for drawing
        private int splashTextureId = -1; // OpenGL texture id created from a bitmap
        private float originalBitmapWidth;
        private float originalBitmapHeight;

        // Helper variables for drawing splash view
        private FloatBuffer verticesBuffer;
        private ShortBuffer indicesBuffer;
        private FloatBuffer uvCoordsBuffer;
        private int indicesCount = -1;

        public Renderer(Bitmap splashViewBitmap)
        {
            if (splashViewBitmap != null)
            {
                // Some devices do not support non POW2 textures,
                // so we need to convert splashViewBitmap to be power of two (if it's not)

                originalBitmapWidth = splashViewBitmap.getWidth();
                originalBitmapHeight = splashViewBitmap.getHeight();

                if (!isPowerOfTwo(splashViewBitmap.getWidth()) || !isPowerOfTwo(splashViewBitmap.getHeight()))
                {
                    int powerOfTwoWidth = 1;
                    int powerOfTwoHeight = 1;

                    while (powerOfTwoWidth < originalBitmapWidth) {
                        powerOfTwoWidth *= 2;
                    }

                    while (powerOfTwoHeight < originalBitmapHeight) {
                        powerOfTwoHeight *= 2;
                    }

                    powerOfTwoBitmap = Bitmap.createBitmap(powerOfTwoWidth, powerOfTwoHeight, Bitmap.Config.ARGB_8888);
                    Canvas canvas = new Canvas(powerOfTwoBitmap);
                    canvas.drawBitmap(splashViewBitmap, 0, 0, null);

                    splashViewBitmap.recycle();
                }
                else
                {
                    powerOfTwoBitmap = splashViewBitmap;
                }
            }
        }

        private boolean isPowerOfTwo(int i)
        {
            return ((i & -i) == i);
        }

        public void cleanup()
        {
            if (powerOfTwoBitmap != null)
            {
                powerOfTwoBitmap.recycle();
                powerOfTwoBitmap = null;
            }
        }

        private int convertBitmapToOpenGLTexture(GL10 gl, Bitmap bitmap)
        {
            int[] textures = new int[1];
            gl.glGenTextures(1, textures, 0);

            gl.glBindTexture(GL10.GL_TEXTURE_2D, textures[0]);

            gl.glTexParameterf(GL10.GL_TEXTURE_2D, GL10.GL_TEXTURE_MIN_FILTER,
                    GL10.GL_LINEAR);
            gl.glTexParameterf(GL10.GL_TEXTURE_2D, GL10.GL_TEXTURE_MAG_FILTER,
                    GL10.GL_LINEAR);

            GLUtils.texImage2D(GL10.GL_TEXTURE_2D, 0, powerOfTwoBitmap, 0);

            return textures[0];
        }

        private void setVerticesBuffer(float[] vertices)
        {
            ByteBuffer vbb = ByteBuffer.allocateDirect(vertices.length * 4);
            vbb.order(ByteOrder.nativeOrder());
            verticesBuffer = vbb.asFloatBuffer();
            verticesBuffer.put(vertices);
            verticesBuffer.position(0);
        }

        private void setIndicesBuffer(short[] indices)
        {
            ByteBuffer ibb = ByteBuffer.allocateDirect(indices.length * 2);
            ibb.order(ByteOrder.nativeOrder());
            indicesBuffer = ibb.asShortBuffer();
            indicesBuffer.put(indices);
            indicesBuffer.position(0);
            indicesCount = indices.length;
        }

        private void setUvCoordsBuffer(float[] uvCoords)
        {
            ByteBuffer byteBuf = ByteBuffer.allocateDirect(uvCoords.length * 4);
            byteBuf.order(ByteOrder.nativeOrder());
            uvCoordsBuffer = byteBuf.asFloatBuffer();
            uvCoordsBuffer.put(uvCoords);
            uvCoordsBuffer.position(0);
        }

        private void drawTexturedQuad(GL10 gl)
        {
            gl.glEnableClientState(GL10.GL_VERTEX_ARRAY);
            gl.glVertexPointer(3, GL10.GL_FLOAT, 0, verticesBuffer);

            gl.glEnable(GL10.GL_TEXTURE_2D);
            gl.glEnableClientState(GL10.GL_TEXTURE_COORD_ARRAY);
            gl.glTexCoordPointer(2, GL10.GL_FLOAT, 0, uvCoordsBuffer);
            gl.glBindTexture(GL10.GL_TEXTURE_2D, splashTextureId);

            gl.glDrawElements(GL10.GL_TRIANGLES, indicesCount, GL10.GL_UNSIGNED_SHORT, indicesBuffer);

            gl.glDisableClientState(GL10.GL_VERTEX_ARRAY);
            gl.glDisableClientState(GL10.GL_TEXTURE_COORD_ARRAY);
        }

        private float[] caculateAspectFitVertices()
        {
            float splashWidth = originalBitmapWidth;
            float splashHeight = originalBitmapHeight;
            
            DeviceManager.DisplayInfo mainDisplayInfo = DeviceManager.instance().getDisplaysInfo()[0];
            float screenWidth = mainDisplayInfo.width;
            float screenHeight = mainDisplayInfo.height;

            float aspectX = splashWidth / screenWidth;
            float aspectY = splashHeight / screenHeight;

            float[] vertices;

            if (aspectX > aspectY)
            {
                float height = (screenWidth * splashHeight / splashWidth);
                float halfHeightNormalized = (height / screenHeight);
                vertices = new float[] {
                    -1.0f, -halfHeightNormalized, 0.0f,
                    1.0f, -halfHeightNormalized, 0.0f,
                    -1.0f,  halfHeightNormalized, 0.0f,
                    1.0f,  halfHeightNormalized, 0.0f
                };
            }
            else
            {
                float width = (screenHeight * splashWidth / splashHeight);
                float halfWidthNormalized = (width / screenWidth);
                vertices = new float[] {
                    -halfWidthNormalized, -1.0f, 0.0f,
                    halfWidthNormalized, -1.0f, 0.0f,
                    -halfWidthNormalized,  1.0f, 0.0f,
                    halfWidthNormalized,  1.0f, 0.0f
                };
            }

            return vertices;
        }

        @Override
        public void onSurfaceCreated(GL10 gl, EGLConfig config)
        {
            if (powerOfTwoBitmap != null)
            {
                splashTextureId = convertBitmapToOpenGLTexture(gl, powerOfTwoBitmap);
                if (splashTextureId != -1)
                {
                    // Create simple quad data for full-screen drawing
                    // (projection = modelview = identity)

                    float uvXRight = originalBitmapWidth / (float)powerOfTwoBitmap.getWidth();
                    float uvYTop = originalBitmapHeight / (float)powerOfTwoBitmap.getHeight();

                    float textureCoordinates[] = {
                            0.0f, uvYTop,
                            uvXRight, uvYTop,
                            0.0f, 0.0f,
                            uvXRight, 0.0f
                    };

                    short[] indices = {
                            0, 1, 2,
                            1, 3, 2
                    };

                    float[] vertices = caculateAspectFitVertices();

                    setUvCoordsBuffer(textureCoordinates);
                    setIndicesBuffer(indices);
                    setVerticesBuffer(vertices);
                }
            }

            getZBufferSize(gl);
            getGPUFamily(gl);
            
            // Now we collect all device info and while show splash to user
            // continue engine initialization
            DavaActivity.instance().onSplashFinishedCollectingDeviceInfo();
        }

        @Override
        public void onSurfaceChanged(GL10 gl, int width, int height)
        {
            gl.glMatrixMode(GL10.GL_PROJECTION);
            gl.glLoadIdentity();
            gl.glMatrixMode(GL10.GL_MODELVIEW);
            gl.glLoadIdentity();

            gl.glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

            gl.glViewport(0, 0, width, height);
        }

        @Override
        public void onDrawFrame(GL10 gl)
        {
            gl.glClear(GL10.GL_COLOR_BUFFER_BIT);

            if (splashTextureId != -1)
            {
                drawTexturedQuad(gl);
            }
        }
        
        private void getZBufferSize(GL10 gl)
        {
            int[] params = new int[1];
            gl.glGetIntegerv(GL10.GL_DEPTH_BITS, params, 0);
            DeviceInfo.zBufferSize = params[0];
        }

        private void getGPUFamily(GL10 gl)
        {
            String extensions = gl.glGetString(GL10.GL_EXTENSIONS);
            byte gpuArchitecture = getSupportedTextures(extensions);
            DeviceInfo.gpuFamily = gpuArchitecture;
        }
        
        private byte getSupportedTextures(String ext)
        {
            // next code from: gles_check_GL_extensions(rhi_GLES2.cpp)
            boolean ATC_Supported = ext.contains("GL_AMD_compressed_ATC_texture");
            boolean PVRTC_Supported = ext.contains("GL_IMG_texture_compression_pvrtc");
            boolean PVRTC2_Supported = ext.contains("GL_IMG_texture_compression_pvrtc2");
            boolean ETC1_Supported = ext.contains("GL_OES_compressed_ETC1_RGB8_texture");
            boolean ETC2_Supported = ext.contains("GL_OES_compressed_ETC2_RGB8_texture");
            // boolean EAC_Supported = ETC2_Supported;
            boolean DXT_Supported = ext.contains("GL_EXT_texture_compression_s3tc") || ext.contains("GL_NV_texture_compression_s3tc");

            byte gpuFamily = GPU_INVALID;
            // next code from: GetGPUFamily(DeviceInfoAndroid.cpp) and from: gles2_TextureFormatSupported(rhi_GLES2.cpp)
            if (PVRTC_Supported || PVRTC2_Supported) // TEXTURE_FORMAT_PVRTC_4BPP_RGBA
            {
                gpuFamily = GPU_POWERVR_ANDROID;
            }
            else if (DXT_Supported) // TEXTURE_FORMAT_DXT1
            {
                gpuFamily = GPU_TEGRA;
            }
            else if (ATC_Supported) // TEXTURE_FORMAT_ATC_RGB
            {
                gpuFamily = GPU_ADRENO;
            }
            else if (ETC1_Supported || ETC2_Supported) // TEXTURE_FORMAT_ETC1
            {
                gpuFamily = GPU_MALI;
            }
            return gpuFamily;
        }
    }
    
    public DavaSplashView(Context context, Bitmap splashImage)
    {
        super(context);

        setEGLContextClientVersion(1);

        renderer = new Renderer(splashImage);
        setRenderer(renderer);
    }

    // Required since bitmap that renderer owns should be disposed
    public void cleanup()
    {
        renderer.cleanup();
    }
}
