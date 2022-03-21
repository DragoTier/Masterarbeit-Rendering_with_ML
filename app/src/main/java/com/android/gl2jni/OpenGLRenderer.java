package com.android.gl2jni;

import android.opengl.GLSurfaceView;

import android.os.Bundle;
import android.os.Handler;
import android.os.Message;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

public class OpenGLRenderer implements GLSurfaceView.Renderer {

    public Handler handler;

    @Override
    public void onDrawFrame(GL10 gl) {
        GL2JNILib.step();

    }

    @Override
    public void onSurfaceChanged(GL10 gl, int width, int height) {
        GL2JNILib.init(width, height);

        int[] img = GL2JNILib.getSuperResImg();
        Message msg = handler.obtainMessage();
        Bundle bundle = new Bundle();
        bundle.putIntArray("pixels", img);
        msg.setData(bundle);
        handler.sendMessage(msg);
    }

    @Override
    public void onSurfaceCreated(GL10 gl, EGLConfig config) {
        // Do nothing.
    }


}
