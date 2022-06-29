package de.esolutions.masterthesis.aiupscale;
import android.opengl.GLSurfaceView;

import android.os.Handler;

import java.nio.MappedByteBuffer;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

/**
 * Java Class Renderer for High Res Context
 */
public class OpenGLRendererLR implements GLSurfaceView.Renderer {

    private final Object obj;
    private final MappedByteBuffer obj_model;
    private final int[] noiseTexArray;
    public Handler handler;

    OpenGLRendererLR(Object obj, MappedByteBuffer obj_model, int[] noiseTexArray){
        this.obj = obj;
        this.obj_model = obj_model;
        this.noiseTexArray = noiseTexArray;
    }

    /**
     * invoked for every frame
     */
    @Override
    public void onDrawFrame(GL10 gl) {

        synchronized (obj){
            OpenGLNativeWrapper.renderFrameLR();
            OpenGLNativeWrapper.upscaleImg();
        }
    }

    /**
     * Gets also invoked on startup
     */
    @Override
    public void onSurfaceChanged(GL10 gl, int width, int height) {
        OpenGLNativeWrapper.initializeContextLR(width, height, obj_model, noiseTexArray);
    }

    @Override
    public void onSurfaceCreated(GL10 gl, EGLConfig config) {

    }


}
