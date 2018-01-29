package com.lmy.ffmpeg.player;

/**
 * Created by lmy on 2017/4/20.
 */

public class Player {
    static {
        System.loadLibrary("avcodec-57");
        System.loadLibrary("avdevice-57");
        System.loadLibrary("avfilter-6");
        System.loadLibrary("avformat-57");
        System.loadLibrary("avutil-55");
        System.loadLibrary("swresample-2");
        System.loadLibrary("swscale-4");
        System.loadLibrary("ffmpeg");
    }

    public native void decode(String in, String out);

    /**
     * 命令行转码
     *
     * @param num
     * @param cmds
     */
    public native void codec(int num, String[] cmds);
}
