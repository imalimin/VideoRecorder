package com.lmy.ffmpeg;

/**
 * Created by lmy on 2017/4/20.
 */

public class Version {
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

    /**
     * ffmpeg版本
     *
     * @return
     */
    public native String version();

    /**
     * ffmpeg类库支持的协议
     *
     * @return
     */
    public native String protocol();

    /**
     * ffmpeg类库支持的封装格式
     *
     * @return
     */
    public native String format();

    /**
     * ffmpeg类库支持的编解码器
     *
     * @return
     */
    public native String codec();

    /**
     * ffmpeg类库支持的滤镜
     *
     * @return
     */
    public native String filter();

    /**
     * ffmpeg类库的配置信息
     *
     * @return
     */
    public native String configure();
}
