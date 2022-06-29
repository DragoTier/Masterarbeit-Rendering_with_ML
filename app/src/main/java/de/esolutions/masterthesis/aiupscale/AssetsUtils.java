package de.esolutions.masterthesis.aiupscale;

import static android.content.res.AssetManager.ACCESS_BUFFER;
import static android.os.ParcelFileDescriptor.MODE_READ_ONLY;

import android.content.Context;
import android.content.res.AssetFileDescriptor;
import android.os.ParcelFileDescriptor;
import com.google.common.io.ByteStreams;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;


public class AssetsUtils {

  private AssetsUtils() {}

  /**
   * Gets AssetFileDescriptor for path
   */
  public static AssetFileDescriptor getAssetFileDescriptor(Context context, String path) throws IOException {
    try {
      return context.getAssets().openFd(path);

    } catch (FileNotFoundException e) {
      File cacheFile = new File(context.getCacheDir(), path);
      cacheFile.getParentFile().mkdirs();
      try (InputStream inputStream = context.getAssets().open(path, ACCESS_BUFFER);
          FileOutputStream fileOutputStream = new FileOutputStream(cacheFile, false)) {
            ByteStreams.copy(inputStream, fileOutputStream);
      }
      ParcelFileDescriptor fileDescriptor = ParcelFileDescriptor.open(cacheFile, MODE_READ_ONLY);
      return new AssetFileDescriptor(fileDescriptor, 0, cacheFile.length());
    }

  }


}
