package com.lmy.ffmpeg.widget;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Rect;
import android.graphics.SurfaceTexture;
import android.media.AudioFormat;
import android.os.AsyncTask;
import android.os.Build;
import android.util.AttributeSet;
import android.util.Log;
import android.view.Surface;
import android.view.TextureView;

import com.lmy.ffmpeg.codec.AVFrame;
import com.lmy.ffmpeg.codec.AudioTrackWraper;
import com.lmy.ffmpeg.codec.MediaDecoder;

import java.nio.ByteBuffer;

/**
 * Created by 李明艺 on 2017/1/13.
 *
 * @author lrlmy@foxmail.com
 */

public class VideoView extends ScalableTextureView implements TextureView.SurfaceTextureListener {
    private final static String TAG = "VideoView";
    private Surface mSurface;
    private Canvas mCanvas;
    private AudioTrackWraper mAudioDevice;
    private MediaDecoder mDecoder;
    private AsyncTask mTask;
    private AVFrame mFrame;
    private Bitmap mBitmap;
    private Rect mDrawRect;
    private int[] mBuffer;

    public VideoView(Context context) {
        super(context);
        initView();
    }

    public VideoView(Context context, AttributeSet attrs) {
        super(context, attrs);
        initView();
    }

    public VideoView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        initView();
    }

    public VideoView(Context context, AttributeSet attrs, int defStyleAttr, int defStyleRes) {
        super(context, attrs, defStyleAttr, defStyleRes);
        initView();
    }

    private void initView() {
        mDecoder = new MediaDecoder();
        setScaleType(ScaleType.CENTER_INSIDE);
        setSurfaceTextureListener(this);//设置监听函数  重写4个方法
    }

    public void setDataSource(String path) {
        mDecoder.setDataSource(path);
        mAudioDevice = AudioTrackWraper.build(mDecoder.getSample_rate(), mDecoder.getChannels(), AudioFormat.ENCODING_PCM_16BIT);
        mAudioDevice.start();
//        setContentWidth(mDecoder.getWidth());
//        setContentHeight(mDecoder.getHeight());
//        updateTextureViewSize();
        mBitmap = Bitmap.createBitmap(mDecoder.getWidth(), mDecoder.getHeight(), Bitmap.Config.ARGB_8888);
        mDrawRect = new Rect(0, 0, mDecoder.getWidth(), mDecoder.getHeight());
    }

    public void start() {
        mTask = new AsyncTask<Void, Void, Void>() {
            @Override
            protected Void doInBackground(Void... voids) {
                int ret;
                long startTime = System.currentTimeMillis();
                while ((ret = mDecoder.nextFrame()) != 1) {
                    if (-1 == ret) continue;
                    long decodeTime = System.currentTimeMillis() - startTime;
                    mFrame = mDecoder.getFrame();
                    if (mFrame.getChannels() > 0) {
                        Log.v(TAG, "音频帧");
                        mAudioDevice.play(mFrame.getData());
                        continue;
                    }
                    long drawTime = System.currentTimeMillis();
//                    fillToBitmap(mFrame.getData(), mFrame.getWidth(), mFrame.getHeight());
                    mBitmap.copyPixelsFromBuffer(ByteBuffer.wrap(mFrame.getData()));
                    mBitmap.prepareToDraw();
                    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M)
                        mCanvas = mSurface.lockHardwareCanvas();
                    else
                        mCanvas = mSurface.lockCanvas(mDrawRect);
                    onDraw();
                    long sleep = (1000 / mDecoder.getFrameRate()) - System.currentTimeMillis() + startTime;
                    if (sleep > 0)
                        try {
                            Thread.sleep(sleep);
                        } catch (InterruptedException e) {
                            e.printStackTrace();
                        }
                    long total = System.currentTimeMillis() - startTime;
                    Log.v(TAG, String.format("decode draw, total, fps: %d, %d, %d  %d  %d", sleep, decodeTime, System.currentTimeMillis() - drawTime, total, 1000 / total));
                    startTime = System.currentTimeMillis();
                }
                return null;
            }

            public void onDraw() {
                mCanvas.drawColor(Color.BLACK);//这里是绘制背景
                mCanvas.drawBitmap(mBitmap, 0, 0, null);
                mSurface.unlockCanvasAndPost(mCanvas);
            }

            public void fillRGB565(byte[] data, int width, int height) {
                int frameSize = width * height;
                if (mBuffer == null)
                    mBuffer = new int[frameSize];
                for (int i = 0; i < frameSize; i++) {
                    int rgb = ((int) data[i * 2]) << 8 | data[i * 2 + 1];
                    int r = (rgb & 0xF800) >> 11;
                    int g = (rgb & 0x07E0) >> 5;
                    int b = (rgb & 0x001F);
                    mBuffer[i] = Color.rgb(r, g, b);
                }
                mBitmap.setPixels(mBuffer, 0, width, 0, 0, width, height);
            }

            public void fillToYUV420SP(byte[] data, int width, int height) {
                final int frameSize = width * height;
                if (mBuffer == null)
                    mBuffer = new int[frameSize];

                for (int j = 0, yp = 0; j < height; j++) {
                    int uvp = frameSize + (j >> 1) * width, u = 0, v = 0;
                    for (int i = 0; i < width; i++, yp++) {
                        int y = (0xff & ((int) data[yp])) - 16;
                        if (y < 0) y = 0;
                        if ((i & 1) == 0) {
                            v = (0xff & data[uvp++]) - 128;
                            u = (0xff & data[uvp++]) - 128;
                        }

                        int y1192 = 1192 * y;
                        int r = (y1192 + 1634 * v);
                        int g = (y1192 - 833 * v - 400 * u);
                        int b = (y1192 + 2066 * u);

                        if (r < 0) r = 0;
                        else if (r > 262143) r = 262143;
                        if (g < 0) g = 0;
                        else if (g > 262143) g = 262143;
                        if (b < 0) b = 0;
                        else if (b > 262143) b = 262143;

                        mBuffer[yp] = 0xff000000 | ((r << 6) & 0xff0000) | ((g >> 2) & 0xff00) | ((b >> 10) & 0xff);
                    }
                }
                mBitmap.setPixels(mBuffer, 0, width, 0, 0, width, height);
            }

            public void fillToBitmap(byte[] data, int width, int height) {
                int frameSize = width * height;
                if (mBuffer == null)
                    mBuffer = new int[frameSize];

                for (int i = 0; i < height; i++)
                    for (int j = 0; j < width; j++) {
                        int y = (0xff & ((int) data[i * width + j]));
                        int u = (0xff & ((int) data[frameSize + i * height / 4 + j]));
                        int v = (0xff & ((int) data[frameSize + frameSize / 4 + i * height / 4 + j]));
                        y = y < 16 ? 16 : y;

                        int r = Math.round(1.164f * (y - 16) + 1.596f * (v - 128));
                        int g = Math.round(1.164f * (y - 16) - 0.813f * (v - 128) - 0.391f * (u - 128));
                        int b = Math.round(1.164f * (y - 16) + 2.018f * (u - 128));

                        r = r < 0 ? 0 : (r > 255 ? 255 : r);
                        g = g < 0 ? 0 : (g > 255 ? 255 : g);
                        b = b < 0 ? 0 : (b > 255 ? 255 : b);

                        mBuffer[i * width + j] = 0xff000000 + (b << 16) + (g << 8) + r;
                    }
                mBitmap.setPixels(mBuffer, 0, width, 0, 0, width, height);
            }
        }.execute();
    }

    @Override
    public void onSurfaceTextureAvailable(SurfaceTexture texture, int width, int height) {
        mSurface = new Surface(texture);
    }

    @Override
    public void onSurfaceTextureSizeChanged(SurfaceTexture texture, int width, int height) {
        mSurface = new Surface(texture);
    }

    @Override
    public boolean onSurfaceTextureDestroyed(SurfaceTexture surfaceTexture) {
        mSurface.release();
        mSurface = null;
        return true;
    }

    @Override
    public void onSurfaceTextureUpdated(SurfaceTexture surfaceTexture) {
    }

    public void release() {
        if (mTask != null) {
            mTask.cancel(true);
        }
        if (mDecoder != null) {
            mDecoder.release();
            mDecoder = null;
        }
        if (mAudioDevice != null) {
            mAudioDevice.stop();
            mAudioDevice.release();
            mAudioDevice = null;
        }
    }
}
