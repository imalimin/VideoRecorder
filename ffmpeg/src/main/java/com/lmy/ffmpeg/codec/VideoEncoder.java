package com.lmy.ffmpeg.codec;

import android.util.Log;

import java.io.File;
import java.util.LinkedList;
import java.util.Queue;

/**
 * Created by limin on 2018/1/30.
 */

public class VideoEncoder {
    private final static String TAG = "VideoEncoder";

    public static VideoEncoder build(String path, int srcWidth, int srcHeight, int width, int height) {
        return new VideoEncoder(path, srcWidth, srcHeight, width, height);
    }

    private int mSrcWidth = 0, mSrcHeight = 0;
    private int mWidth = 0, mHeight = 0;
    private boolean ready = false;
    private FrameQueue mFrameQueue;
    private EncodeThread mEncodeThread;
    private int mDropCount = 0;

    public VideoEncoder(String path, int srcWidth, int srcHeight, int width, int height) {
        mSrcWidth = srcWidth;
        mSrcHeight = srcHeight;
        mWidth = width;
        mHeight = height;
        checkFile(path);
        if (init(path, height, width, width, height) < 0) {
            Log.e(TAG, "Init Failed");
            ready = false;
            return;
        }
        mFrameQueue = FrameQueue.build(width * height * 3 / 2, 10);
        mEncodeThread = new EncodeThread(this, mFrameQueue);
        mEncodeThread.start();//开启线程
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

    private void cacheAndCropFrame(byte[] data) {
        FrameBuffer buffer = mFrameQueue.getCacheBuffer();
        //没有可用缓存，跳过
        if (null == buffer) {
            ++mDropCount;
            Log.e(TAG, "没有可用缓存，总共丢弃: " + mDropCount);
            return;
        }
        int offset = (mSrcWidth - mHeight) / 2;
        for (int i = 0; i < mSrcHeight * 1.5; i++) {
            System.arraycopy(data, i * mSrcWidth + offset, buffer.getBuffer(), i * mHeight, mHeight);
        }
        mFrameQueue.offer(buffer);
    }

    public void encode2(byte[] data) {
        if (null == data || !isReady()) return;
        cacheAndCropFrame(data);
        Log.i(TAG, "cache: " + mFrameQueue.getRecycleSize());
    }

    public void flush2() {
        Log.i(TAG, "flush2");
        if (null != mEncodeThread)
            mEncodeThread.stopProxy();
        if (null != mFrameQueue)
            mFrameQueue.release();
    }

    public native int init(String path, int srcWidth, int srcHeight, int width, int height);

    public native void encode(byte[] data);

    public native void flush();

    private static class EncodeThread extends Thread {
        private VideoEncoder mVideoEncoder;
        private FrameQueue mFrameQueue;
        private boolean mStop = false;

        EncodeThread(VideoEncoder videoEncoder, FrameQueue frameQueue) {
            this.mVideoEncoder = videoEncoder;
            this.mFrameQueue = frameQueue;
        }

        @Override
        public void run() {
            while (true) {
                FrameBuffer buffer = mFrameQueue.poll();
                if (null != buffer) {
                    mVideoEncoder.encode(buffer.getBuffer());
                    mFrameQueue.recycle(buffer);
                    Log.i(TAG, "encoded");
                }
                if (mStop) {
                    mVideoEncoder.flush();
                    return;
                }
            }
        }

        void stopProxy() {
            mStop = true;
        }
    }

    /**
     * 缓存队列
     */
    private static class FrameQueue {
        public static FrameQueue build(int bufferSize, int size) {
            return new FrameQueue(bufferSize, size);
        }

        private int bufferSize;
        private final Queue<FrameBuffer> mQueue = new LinkedList<>();
        private final Queue<FrameBuffer> mRecycleQueue = new LinkedList<>();

        private FrameQueue(int bufferSize, int size) {
            this.bufferSize = bufferSize;
            for (int i = 0; i < size; i++) {
                mRecycleQueue.offer(new FrameBuffer(bufferSize));
            }
        }

        int getRecycleSize() {
            synchronized (mRecycleQueue) {
                return mRecycleQueue.size();
            }
        }

        FrameBuffer getCacheBuffer() {
            synchronized (mRecycleQueue) {
                return mRecycleQueue.poll();
            }
        }

        void offer(FrameBuffer buffer) {
            synchronized (mQueue) {
                buffer.setRecycled(false);
                mQueue.offer(buffer);
            }
        }

        FrameBuffer poll() {
            synchronized (mQueue) {
                FrameBuffer buffer = mQueue.poll();
                if (null == buffer) return null;
                if (buffer.isRecycled()) {
                    recycle(buffer);
                    return null;
                }
                return buffer;
            }
        }

        void recycle(FrameBuffer buffer) {
            buffer.setRecycled(true);
            mRecycleQueue.offer(buffer);
        }

        public void release() {
            while (!mQueue.isEmpty())
                mQueue.poll();
            while (!mRecycleQueue.isEmpty())
                mRecycleQueue.poll();
        }
    }

    /**
     * 帧缓存结构
     */
    private static class FrameBuffer {
        private byte[] mBuffer;
        private boolean recycled = true;

        FrameBuffer(int size) {
            mBuffer = new byte[size];
        }

        public byte[] getBuffer() {
            return mBuffer;
        }

        public void setBuffer(byte[] buffer) {
            this.mBuffer = buffer;
        }

        public boolean isRecycled() {
            return recycled;
        }

        void setRecycled(boolean recycled) {
            this.recycled = recycled;
        }
    }
}
