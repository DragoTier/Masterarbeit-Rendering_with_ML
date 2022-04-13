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


public class GL2JNIActivity extends Activity {

    static {
        System.loadLibrary("gl3aiupscale");
    }

    private GLSurfaceView viewLR;
    private GLSurfaceView viewHR;
    private static final String TAG = "SuperResolution";
    private static final String MODEL_NAME = "model_from_h5_edge.tflite";

    private static final int LR_IMAGE_HEIGHT = 800;
    private static final int LR_IMAGE_WIDTH = 360;
    private static final int UPSCALE_FACTOR = 3;
    private static final int SR_IMAGE_HEIGHT = LR_IMAGE_HEIGHT * UPSCALE_FACTOR;
    private static final int SR_IMAGE_WIDTH = LR_IMAGE_WIDTH * UPSCALE_FACTOR;

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

        superResolutionNativeHandle = initTFLiteInterpreter(true);
        GL2JNILib.setNativeHandle(superResolutionNativeHandle);
        viewLR = (GLSurfaceView) findViewById(R.id.lowResView); //new GL2JNIView(getApplication());
        viewLR.setEGLContextClientVersion(3);

        OpenGLRendererLR rendererLR = new OpenGLRendererLR(obj);
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
            //muss nicht gefüllt werden
        }
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
        deinit();
    }

    private void deinit() {
        deinitFromJNI(superResolutionNativeHandle);
    }

    private native int[] superResolutionFromJNI(long superResolutionNativeHandle, int[] lowResRGB);

    private native long initWithByteBufferFromJNI(MappedByteBuffer modelBuffer, boolean useGPU);

    private native void deinitFromJNI(long superResolutionNativeHandle);
}

