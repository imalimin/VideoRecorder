package com.lmy.ffmpeg.codec;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.util.Log;

import java.util.Queue;
import java.util.concurrent.ConcurrentLinkedQueue;

/**
 * Created by lmy on 2017/4/25.
 */

public class AudioTrackWraper implements Runnable {
    private final static String TAG = "AudioTrackWraper";
    private AudioTrack mPlayer;
    private int sampleRateInHz = 48000;
    private int channelConfig = AudioFormat.CHANNEL_CONFIGURATION_STEREO;//双声道
    private int audioFormat;//采样格式
    private Thread mPlayThread;
    private final Queue<byte[]> mQueue = new ConcurrentLinkedQueue<>();
    private boolean stop = false;

    public static AudioTrackWraper build(int sampleRateInHz, int channelConfig, int audioFormat) {
        return new AudioTrackWraper(sampleRateInHz, channelConfig, audioFormat);
    }

    public AudioTrackWraper(int sampleRateInHz, int channelConfig, int audioFormat) {
        this.sampleRateInHz = sampleRateInHz;
        this.channelConfig = channelConfig;
        this.audioFormat = audioFormat;
        init();
    }

    private void init() {
        int bufferSize = AudioTrack.getMinBufferSize(sampleRateInHz, channelConfig, audioFormat);
        //创建AudioTrack
        mPlayer = new AudioTrack(AudioManager.STREAM_MUSIC, sampleRateInHz,
                AudioFormat.CHANNEL_CONFIGURATION_STEREO,
                audioFormat,
                bufferSize,
                AudioTrack.MODE_STREAM);
        this.stop = false;
        mPlayThread = new Thread(this);
        Log.e(TAG, String.format("init: sample=%d, channel=%d, audio=%d, bufferSize=%d", sampleRateInHz, channelConfig, audioFormat, bufferSize));
    }

    public void play(byte[] data) {
        byte[] dest = new byte[data.length];
        System.arraycopy(data, 0, dest, 0, dest.length);
        offer(dest);
    }

    public void start() {
        mPlayer.play();
        mPlayThread.start();
    }

    public void stop() {
        mPlayer.stop();
        this.stop = true;
    }

    public void release() {
        if (mPlayer != null) {
            mPlayer.release();
            mPlayer = null;
        }
        if (mPlayThread != null)
            mPlayThread = null;
    }

    private void offer(byte[] data) {
        synchronized (mQueue) {
            mQueue.offer(data);
        }
    }

    private byte[] poll() {
        synchronized (mQueue) {
            return mQueue.poll();
        }
    }

    @Override
    public void run() {
        byte[] data;
        while (!stop) {
            data = poll();
            if (data != null && mPlayer != null) {
                //long start = System.currentTimeMillis();
                mPlayer.write(data, 0, data.length);
                //Log.e(TAG, "write end: " + (System.currentTimeMillis() - start));
            }
        }
    }
}
