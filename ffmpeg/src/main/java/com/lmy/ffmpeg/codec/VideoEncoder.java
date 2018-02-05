package com.lmy.ffmpeg.codec;

import android.util.Log;

import java.io.File;

/**
 * Created by limin on 2018/1/30.
 */

public class VideoEncoder {
    private final static String TAG = "VideoEncoder";

    public static VideoEncoder build(String path, int srcWidth, int srcHeight, int width, int height) {
        return new VideoEncoder(path, srcWidth, srcHeight, width, height);
    }

    private boolean ready = false;
    private byte[] mBuffer;

    public VideoEncoder(String path, int srcWidth, int srcHeight, int width, int height) {
        checkFile(path);
        if (init(path, srcWidth, srcHeight, width, height) < 0) {
            Log.e(TAG, "Init Failed");
            ready = false;
            return;
        }
        mBuffer = new byte[srcWidth * srcHeight * 3 / 2];
        ready = true;
    }

    static {
        System.loadLibrary("avcodec-56");
        System.loadLibrary("avfilter-5");
        System.loadLibrary("avformat-56");
        System.loadLibrary("avutil-54");
        System.loadLibrary("swresample-1");
        System.loadLibrary("swscale-3");
        System.loadLibrary("ffmpeg");
    }

    private void checkFile(String path) {
        File file = new File(path);
        if (file.exists()) {
            file.delete();
        }
    }

    private boolean isReady() {
        return ready;
    }

    public void encode2(byte[] data) {
        if (null == data || !isReady()) return;
        System.arraycopy(data, 0, mBuffer, 0, data.length);
        Log.i(TAG, "encode2");
        encode(mBuffer);
    }

    public native int init(String path, int srcWidth, int srcHeight, int width, int height);

    public native void encode(byte[] data);

    public native void flush();
}
