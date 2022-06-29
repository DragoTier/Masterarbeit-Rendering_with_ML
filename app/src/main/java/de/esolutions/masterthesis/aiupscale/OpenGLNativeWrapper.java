package de.esolutions.masterthesis.aiupscale;

import java.nio.MappedByteBuffer;

/**
 * Wrapper Class for native methods in gl_code_lr and gl_code_hr
 */
public class OpenGLNativeWrapper {


    //low res context
    public static native void initializeContextLR(int width, int height, MappedByteBuffer model_buffer, int[] noiseTexture);
    public static native void renderFrameLR();
    public static native void setNativeHandle(long nativeHandle);
    public static native void upscaleImg();

    //High res context
    public static native void initializeContextHR(int width, int height);
    public static native void renderFrameHR();

}
