/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.gl2jni;

import android.Manifest;
import android.app.Activity;
import android.graphics.ColorSpace;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.WindowManager;

import java.io.File;

import com.android.gl2jni.R;
import android.content.res.AssetFileDescriptor;
import android.content.res.AssetManager;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.drawable.BitmapDrawable;
import android.os.Bundle;
import android.os.SystemClock;
import android.app.Activity;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;
import android.widget.Button;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.Switch;
import android.widget.TextView;
import android.widget.Toast;

import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.MappedByteBuffer;
import java.nio.channels.FileChannel;

//import org.tensorflow.lite.nnapi.NnApiDelegate;

public class GL2JNIActivity extends Activity {

    GLSurfaceView mView;
    static {
        System.loadLibrary("gl2jni");
    }


    private static final String TAG = "SuperResolution";
    private static final String MODEL_NAME = "model_from_h5.tflite";

    //private static final String MODEL_NAME = "super-Resolution_converted.tflite";
    private static final int LR_IMAGE_HEIGHT = 800;
    private static final int LR_IMAGE_WIDTH = 360;
    private static final int UPSCALE_FACTOR = 3;
    private static final int SR_IMAGE_HEIGHT = LR_IMAGE_HEIGHT * UPSCALE_FACTOR;
    private static final int SR_IMAGE_WIDTH = LR_IMAGE_WIDTH * UPSCALE_FACTOR;
    private static final String LR_IMG_1 = "testImage.png";
    private static final String LR_IMG_2 = "lr-2.jpg";
    private static final String LR_IMG_3 = "lr-3.jpg";

    private MappedByteBuffer model;
    private long superResolutionNativeHandle = 0;
    private Bitmap selectedLRBitmap = null;
    private boolean useGPU = false;

    private ImageView lowResImageView1;
    private ImageView lowResImageView2;
    private ImageView lowResImageView3;
    private TextView selectedImageTextView;
    private Switch gpuSwitch;

    private static final int PERMISSION_REQUEST_CODE = 111111111;

    @Override protected void onCreate(Bundle icicle) {
        super.onCreate(icicle);
        setContentView(R.layout.activity_main);

        final ImageView superResolutionImageView = findViewById(R.id.imageView);
        superResolutionImageView.setImageDrawable(null);

        superResolutionNativeHandle = initTFLiteInterpreter(true);
        GL2JNILib.setNativeHandle(superResolutionNativeHandle);
        mView = (GLSurfaceView) findViewById(R.id.viewTest); //new GL2JNIView(getApplication());
        mView.setEGLContextClientVersion(3);
        OpenGLRenderer renderer = new OpenGLRenderer();

        renderer.handler = new Handler(new HandlerCallback(SR_IMAGE_WIDTH, SR_IMAGE_HEIGHT, superResolutionImageView));;
        mView.setRenderer(renderer);
        //setContentView(mView);

        requestPermissions(new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE}, PERMISSION_REQUEST_CODE);

        /*Bitmap bitmap = null;
        try{
            AssetManager assetManager = getAssets();
            InputStream inputStream = assetManager.open(LR_IMG_1);
            bitmap = BitmapFactory.decodeStream(inputStream);
        } catch (IOException e){
            Log.e(TAG, "Failed to open an low resolution image");
        }
        int[] lowResRGB = new int[LR_IMAGE_WIDTH * LR_IMAGE_HEIGHT];
        bitmap.getPixels(lowResRGB, 0, bitmap.getRowBytes()/4, 0, 0, LR_IMAGE_WIDTH, LR_IMAGE_HEIGHT);
        Bitmap.Config config = bitmap.getConfig();
        final long startTime = SystemClock.uptimeMillis();
        int[] superResRGB = doSuperResolution(lowResRGB);//GL2JNILib.getSuperResImg(); //do super resolution
        final long processingTimeMs = SystemClock.uptimeMillis() - startTime;
        if(superResRGB == null){
            Log.e(TAG, "super resolution failed");
        }

        Bitmap srImgBitmap = Bitmap.createBitmap(superResRGB, SR_IMAGE_WIDTH, SR_IMAGE_HEIGHT, Bitmap.Config.ARGB_8888);
        final ImageView superResolutionImageView = findViewById(R.id.imageView);
        superResolutionImageView.setImageDrawable(null);
        superResolutionImageView.setImageBitmap(srImgBitmap); */
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);

        if(requestCode == PERMISSION_REQUEST_CODE){
            //muss nicht gefüllt werden
        }
    }


    @Override
    protected void onPause() {
        super.onPause();
        mView.onPause();
    }

    @Override
    protected void onResume() {
        super.onResume();
        mView.onResume();
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        deinit();
    }

    //@WorkerThread
    public synchronized int[] doSuperResolution(int[] lowResRGB) {
        return superResolutionFromJNI(superResolutionNativeHandle, lowResRGB);
    }

    private MappedByteBuffer loadModelFile() throws IOException {
        try (AssetFileDescriptor fileDescriptor =
                     AssetsUtil.getAssetFileDescriptorOrCached(getApplicationContext(), MODEL_NAME);
             FileInputStream inputStream = new FileInputStream(fileDescriptor.getFileDescriptor())) {
            FileChannel fileChannel = inputStream.getChannel();
            long startOffset = fileDescriptor.getStartOffset();
            long declaredLength = fileDescriptor.getDeclaredLength();
            return fileChannel.map(FileChannel.MapMode.READ_ONLY, startOffset, declaredLength);
        }
    }

    private long initTFLiteInterpreter(boolean useNNAPI) {
        try {
            model = loadModelFile();
        } catch (IOException e) {
            Log.e(TAG, "Fail to load model", e);
        }
        return initWithByteBufferFromJNI(model, useNNAPI);
    }

    private void deinit() {
        deinitFromJNI(superResolutionNativeHandle);
    }

    private native int[] superResolutionFromJNI(long superResolutionNativeHandle, int[] lowResRGB);

    private native long initWithByteBufferFromJNI(MappedByteBuffer modelBuffer, boolean useGPU);

    private native void deinitFromJNI(long superResolutionNativeHandle);
}

