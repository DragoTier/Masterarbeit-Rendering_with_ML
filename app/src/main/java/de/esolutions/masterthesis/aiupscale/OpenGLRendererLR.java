package de.esolutions.masterthesis.aiupscale;
import android.opengl.GLSurfaceView;

import android.os.Handler;

import java.nio.MappedByteBuffer;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

public class OpenGLRendererLR implements GLSurfaceView.Renderer {

    private final Object obj;
    private final MappedByteBuffer obj_model;
    public Handler handler;

    OpenGLRendererLR(Object obj, MappedByteBuffer obj_model){
        this.obj = obj;
        this.obj_model = obj_model;
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
        GLES32JNILib.init(width, height, obj_model);
    }

    @Override
    public void onSurfaceCreated(GL10 gl, EGLConfig config) {

    }


}
