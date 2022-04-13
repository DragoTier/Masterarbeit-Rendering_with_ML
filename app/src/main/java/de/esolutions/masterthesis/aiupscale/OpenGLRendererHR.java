package de.esolutions.masterthesis.aiupscale;
import android.opengl.GLES32;
import android.opengl.GLSurfaceView;

import android.os.Bundle;
import android.os.Handler;
import android.os.Message;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

public class OpenGLRendererHR implements GLSurfaceView.Renderer {

    private final Object obj;
    public Handler handler;

    public OpenGLRendererHR(Object obj){
        this.obj = obj;
    }
    @Override
    public void onDrawFrame(GL10 gl) {
        synchronized (obj){
            GL2JNILib.stepHR();
        }

    }

    @Override
    public void onSurfaceChanged(GL10 gl, int width, int height) {
        System.out.println(gl.glGetString(GL10.GL_VERSION));
        GL2JNILib.initHR(width, height);
    }

    @Override
    public void onSurfaceCreated(GL10 gl, EGLConfig config) {

    }


}
