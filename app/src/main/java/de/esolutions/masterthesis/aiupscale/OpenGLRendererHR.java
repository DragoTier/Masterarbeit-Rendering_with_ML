package de.esolutions.masterthesis.aiupscale;
import android.opengl.GLSurfaceView;

import android.os.Handler;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

/**
 * Java Class Renderer for High Res Context
 */
public class OpenGLRendererHR implements GLSurfaceView.Renderer {

    private final Object obj;
    public Handler handler;

    public OpenGLRendererHR(Object obj){
        this.obj = obj;
    }

    /**
     * invoked for every frame
     */
    @Override
    public void onDrawFrame(GL10 gl) {
        synchronized (obj){
            OpenGLNativeWrapper.renderFrameHR();
        }

    }

    /**
     * Gets also invoked on startup
     */
    @Override
    public void onSurfaceChanged(GL10 gl, int width, int height) {
        System.out.println(gl.glGetString(GL10.GL_VERSION));
        OpenGLNativeWrapper.initializeContextHR(width, height);
    }

    @Override
    public void onSurfaceCreated(GL10 gl, EGLConfig config) {

    }


}
