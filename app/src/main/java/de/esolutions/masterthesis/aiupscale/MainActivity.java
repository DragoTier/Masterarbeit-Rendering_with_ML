package de.esolutions.masterthesis.aiupscale;

import android.Manifest;
import android.app.Activity;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.util.Log;
import android.content.res.AssetFileDescriptor;
import java.io.FileInputStream;
import java.io.IOException;
import java.nio.MappedByteBuffer;
import java.nio.channels.FileChannel;


public class MainActivity extends Activity {

    static {
        System.loadLibrary("gl3aiupscale");
    }

    private GLSurfaceView viewLR;
    private GLSurfaceView viewHR;
    private static final String TAG = "SuperResolution";
    private static final String MODEL_NAME = "model_from_h5_edge.tflite";
    private static final String OBJ_NAME = "bunny.obj";

    private static final int UPSCALE_FACTOR = 3;

    private MappedByteBuffer model;
    private long superResolutionNativeHandle = 0;

    private static final int PERMISSION_REQUEST_CODE = 111111111;

    @Override protected void onCreate(Bundle icicle) {
        super.onCreate(icicle);
        setContentView(R.layout.activity_main);
        requestPermissions(new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE}, PERMISSION_REQUEST_CODE);

       // ImageView superResolutionImageView = (ImageView) findViewById(R.id.imageView);
       // superResolutionImageView.setImageDrawable(null);
        Object obj = new Object();
        int tflite_accelerator = 1;
        superResolutionNativeHandle = initTFLiteInterpreter(tflite_accelerator);
        GLES32JNILib.setNativeHandle(superResolutionNativeHandle);
        viewLR = (GLSurfaceView) findViewById(R.id.lowResView); //new GL2JNIView(getApplication());
        viewLR.setEGLContextClientVersion(3);
        MappedByteBuffer obj_Model = loadFile(OBJ_NAME);
        OpenGLRendererLR rendererLR = new OpenGLRendererLR(obj, obj_Model);
        //rendererLR.handler = new Handler(new HandlerCallback(SR_IMAGE_WIDTH, SR_IMAGE_HEIGHT, superResolutionImageView));;
        viewLR.setRenderer(rendererLR);

        viewHR = (GLSurfaceView) findViewById(R.id.highResView); //new GL2JNIView(getApplication());
        viewHR.setEGLContextClientVersion(3);
        OpenGLRendererHR rendererHR = new OpenGLRendererHR(obj);
        viewHR.setRenderer(rendererHR);
        viewHR.setZOrderMediaOverlay(true);
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);

        if(requestCode == PERMISSION_REQUEST_CODE){
            //muss nicht gefuellt werden
        }
    }

    private MappedByteBuffer loadFile(String file) {
        try (AssetFileDescriptor fileDescriptor =
                     AssetsUtil.getAssetFileDescriptorOrCached(getApplicationContext(), file);
             FileInputStream inputStream = new FileInputStream(fileDescriptor.getFileDescriptor())) {
            FileChannel fileChannel = inputStream.getChannel();
            long startOffset = fileDescriptor.getStartOffset();
            long declaredLength = fileDescriptor.getDeclaredLength();
            return fileChannel.map(FileChannel.MapMode.READ_ONLY, startOffset, declaredLength);
        } catch (IOException e){
            Log.e(TAG, "Fail to load file", e);
            return null;
        }
    }

    private long initTFLiteInterpreter(int useAccelerator) {
        model = loadFile(MODEL_NAME);
        return initWithByteBufferFromJNI(model, useAccelerator);
    }

    @Override
    protected void onPause() {
        super.onPause();
        viewLR.onPause();
    }

    @Override
    protected void onResume() {
        super.onResume();
        viewLR.onResume();
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        deinitFromJNI(superResolutionNativeHandle);
    }

    private native long initWithByteBufferFromJNI(MappedByteBuffer modelBuffer, int useAccelerator);
    private native void deinitFromJNI(long superResolutionNativeHandle);
}

