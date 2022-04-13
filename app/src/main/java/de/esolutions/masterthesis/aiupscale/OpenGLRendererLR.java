package de.esolutions.masterthesis.aiupscale;
import android.opengl.GLSurfaceView;

import android.os.Handler;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

public class OpenGLRendererLR implements GLSurfaceView.Renderer {

    private final Object obj;
    public Handler handler;

    OpenGLRendererLR(Object obj){
        this.obj = obj;
    }

    @Override
    public void onDrawFrame(GL10 gl) {
        synchronized (obj){
            GLES32JNILib.step();
            GLES32JNILib.upscaleImg();
        }


     /*   Message msg = handler.obtainMessage();
        Bundle bundle = new Bundle();
        bundle.putIntArray("pixels", img);
        msg.setData(bundle);
        handler.sendMessage(msg); */
    }

    @Override
    public void onSurfaceChanged(GL10 gl, int width, int height) {
        GLES32JNILib.init(width, height);
    }

    @Override
    public void onSurfaceCreated(GL10 gl, EGLConfig config) {

    }


}
