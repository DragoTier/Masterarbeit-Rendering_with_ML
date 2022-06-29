package de.esolutions.masterthesis.aiupscale;

import android.Manifest;
import android.app.Activity;
import android.content.res.AssetManager;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.util.Log;
import android.content.res.AssetFileDescriptor;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.MappedByteBuffer;
import java.nio.channels.FileChannel;

/**
 * Main Activity to control all components of the application
 */
public class MainActivity extends Activity {

    static {
        System.loadLibrary("gl3aiupscale");
    }

    private GLSurfaceView viewLR;
    private GLSurfaceView viewHR;
    private static final String PRINT_TAG = "SuperResolution-MainActivity";
    private static final String MODEL_NAME = "model_from_h5_edge_4.tflite"; //ESPCN Model
    private static final String OBJ_NAME = "bunny2.obj"; //3D Object to be rendered
    private static final String NOISE_TEXTURE = "noiseTexture.png"; //texture
    private static final int PERMISSION_REQUEST_CODE = 111111111;

    private long superResolutionNativeHandle = 0;


    @Override protected void onCreate(Bundle icicle) {
        super.onCreate(icicle);
        setContentView(R.layout.activity_main);
        requestPermissions(new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE}, PERMISSION_REQUEST_CODE);

        Object obj = new Object(); //for synchronization between gl threads
        int tflite_accelerator = 1; //0 => CPU, 1 => NNAPI, 2 => GPU
        MappedByteBuffer model = loadFile(MODEL_NAME);
        superResolutionNativeHandle = createSuperResolutionInstance(model, tflite_accelerator);
        OpenGLNativeWrapper.setNativeHandle(superResolutionNativeHandle);

        //load texture from assets
        Bitmap noiseTexture = null;
        try{
            AssetManager assetManager = getAssets();
            InputStream inputStream = assetManager.open(NOISE_TEXTURE);
            noiseTexture = BitmapFactory.decodeStream(inputStream);
        } catch (IOException e){
            Log.e(PRINT_TAG, "Failed to open texture");
        }
        int[] noiseTexArray = new int[noiseTexture.getWidth() * noiseTexture.getHeight()];
        noiseTexture.getPixels(noiseTexArray, 0, noiseTexture.getRowBytes()/4, 0, 0, noiseTexture.getWidth(), noiseTexture.getHeight());

        //init low res GL context
        viewLR = (GLSurfaceView) findViewById(R.id.lowResView); //new GL2JNIView(getApplication());
        viewLR.setEGLContextClientVersion(3);
        MappedByteBuffer obj_Model = loadFile(OBJ_NAME);
        OpenGLRendererLR rendererLR = new OpenGLRendererLR(obj, obj_Model, noiseTexArray);
        viewLR.setRenderer(rendererLR);

        //init high res GL context
        viewHR = (GLSurfaceView) findViewById(R.id.highResView); //new GL2JNIView(getApplication());
        viewHR.setEGLContextClientVersion(3);
        OpenGLRendererHR rendererHR = new OpenGLRendererHR(obj);
        viewHR.setRenderer(rendererHR);
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);

        if(requestCode == PERMISSION_REQUEST_CODE){
            //muss nicht gefuellt werden
        }
    }

    private MappedByteBuffer loadFile(String file) {
        try (AssetFileDescriptor fileDescriptor = AssetsUtils.getAssetFileDescriptor(getApplicationContext(), file);
             FileInputStream inputStream = new FileInputStream(fileDescriptor.getFileDescriptor())) {
            FileChannel fileChannel = inputStream.getChannel();
            long startOffset = fileDescriptor.getStartOffset();
            long declaredLength = fileDescriptor.getDeclaredLength();
            return fileChannel.map(FileChannel.MapMode.READ_ONLY, startOffset, declaredLength);
        } catch (IOException e){
            Log.e(PRINT_TAG, "Failed to load file", e);
            return null;
        }
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
        deleteSuperResolutionInstance(superResolutionNativeHandle);
    }

    private native long createSuperResolutionInstance(MappedByteBuffer modelBuffer, int accelerator);
    private native void deleteSuperResolutionInstance(long superResolutionNativeHandle);
}

