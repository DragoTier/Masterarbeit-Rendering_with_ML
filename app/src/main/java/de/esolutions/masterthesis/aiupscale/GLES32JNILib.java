package de.esolutions.masterthesis.aiupscale;
// Wrapper for native library

public class GLES32JNILib {

    /**
    * @param width the current view width
    * @param height the current view height
    */
    public static native void init(int width, int height);
    public static native void step();
    public static native void setNativeHandle(long nativeHandle);
    public static native void upscaleImg();

    public static native void initHR(int width, int height);
    public static native void stepHR();

}
