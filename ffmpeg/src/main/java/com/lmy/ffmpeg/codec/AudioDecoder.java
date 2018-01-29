package com.lmy.ffmpeg.codec;

/**
 * Created by lmy on 2017/4/28.
 */

public class AudioDecoder {
    static {
        System.loadLibrary("avcodec-56");
        System.loadLibrary("avfilter-5");
        System.loadLibrary("avformat-56");
        System.loadLibrary("avutil-54");
        System.loadLibrary("swresample-1");
        System.loadLibrary("swscale-3");
        System.loadLibrary("ffmpeg");
    }

    private AVFrame mFrame;
    private int width, height;
    /**
     * （音频）采样率
     * Sample rate of the audio data.
     * <p>
     * - encoding: unused
     * - decoding: read by user
     */
    private int sample_rate;
    /**
     * number of audio channels, only used for audio.
     * Code outside libavcodec should access this field using:
     * av_frame_get_channels(frame)
     * - encoding: unused
     * - decoding: Read by user.
     */
    private int channels;

    public AudioDecoder() {
        this.mFrame = new AVFrame();
    }

    public void setDataSource(String path) {
        setDataSource(path, mFrame);
    }

    private native void setDataSource(String path, AVFrame frame);

    public native int nextFrame();

    public native void release();

    public AVFrame getFrame() {
        return mFrame;
    }

    public int getWidth() {
        return width;
    }

    public void setWidth(int width) {
        this.width = width;
    }

    public int getHeight() {
        return height;
    }

    public void setHeight(int height) {
        this.height = height;
    }

    public int getSample_rate() {
        return sample_rate;
    }

    public void setSample_rate(int sample_rate) {
        this.sample_rate = sample_rate;
    }

    public int getChannels() {
        return channels;
    }

    public void setChannels(int channels) {
        this.channels = channels;
    }
}
