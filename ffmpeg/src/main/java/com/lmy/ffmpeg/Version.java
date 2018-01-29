package com.lmy.ffmpeg;

/**
 * Created by lmy on 2017/4/20.
 */

public class Version {
    static {
        System.loadLibrary("avcodec-56");
        System.loadLibrary("avdevice-56");
        System.loadLibrary("avfilter-5");
        System.loadLibrary("avformat-56");
        System.loadLibrary("avutil-54");
        System.loadLibrary("swresample-1");
        System.loadLibrary("swscale-3");
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
