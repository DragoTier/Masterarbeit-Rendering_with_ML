package de.esolutions.masterthesis.aiupscale;
import android.graphics.Bitmap;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.widget.ImageView;

import androidx.annotation.NonNull;

public class HandlerCallback implements Handler.Callback{

    private final int width;
    private final int height;
    private final ImageView srImageView;

    public HandlerCallback(int width, int height, ImageView srImageView){
            this.width = width;
            this.height = height;
            this.srImageView = srImageView;
    }

    @Override
    public boolean handleMessage(@NonNull Message msg) {
      /*  Bundle bundle = msg.getData();
        if(bundle != null){
            int[] pixels = bundle.getIntArray("pixels");
            Bitmap srImgBitmap = Bitmap.createBitmap(pixels, width, height, Bitmap.Config.ARGB_8888);
            srImageView.setImageBitmap(null);
            srImageView.setImageBitmap(srImgBitmap);
            return true;
        } */
        return false;
    }
}
