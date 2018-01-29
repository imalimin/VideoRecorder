package com.lmy.ffmpeg.codec;

/**
 * Created by lmy on 2017/4/20.
 */

public class AVFrame {
    private final static int AV_NUM_DATA_POINTERS = 8;
    /**
     * 图像数据
     * pointer to the picture/channel planes.
     * This might be different from the first allocated byte
     * - encoding: Set by user
     * - decoding: set by AVCodecContext.get_buffer()
     */
    private byte[] data;
    private float[] samples;
    /**
     * Size, in bytes, of the data for each picture/channel plane.
     * <p>
     * For audio, only linesize[0] may be set. For planar audio, each channel
     * plane must be the same size.
     * <p>
     * - encoding: Set by user
     * - decoding: set by AVCodecContext.get_buffer()
     */
    private int linesize[];
    private int width, height;
    /**
     * 音频的一个AVFrame中可能包含多个音频帧，在此标记包含了几个
     */
    private int nb_samples;
    /**
     * 解码后原始数据类型（YUV420，YUV422，RGB24...）
     */
    private int format;
    /**
     * 是否是关键帧
     */
    private int key_frame;

    /**
     * 帧类型（I,B,P...）
     */
    private AVPictureType pict_type;
    /**
     * 宽高比（16:9，4:3...）
     */
    private AVRational sample_aspect_ratio;
    /**
     * 显示时间戳
     * presentation timestamp in time_base units (time when frame should be shown to user)
     * If AV_NOPTS_VALUE then frame_rate = 1/time_base will be assumed.
     * - encoding: MUST be set by user.
     * - decoding: Set by libavcodec.
     */
    private long pts;
    /**
     * 编码帧序号
     */
    private int coded_picture_number;
    /**
     * 显示帧序号
     * picture number in bitstream order
     * - encoding: set by
     * - decoding: Set by libavcodec.
     */
    private int display_picture_number;
//    int8_t *qscale_table：QP表
//    uint8_t *mbskip_table：跳过宏块表
//    int16_t (*motion_val[2])[2]：运动矢量表
//    uint32_t *mb_type：宏块类型表
//    short *dct_coeff：DCT系数，这个没有提取过
//    int8_t *ref_index[2]：运动估计参考帧列表（貌似H.264这种比较新的标准才会涉及到多参考帧）

    /**
     * reordered pts from the last AVPacket that has been input into the decoder
     * - encoding: unused
     * - decoding: Read by user.
     */
    private int pkt_pts;

    /**
     * dts from the last AVPacket that has been input into the decoder
     * - encoding: unused
     * - decoding: Read by user.
     */
    private int pkt_dts;
    /**
     * quality (between 1 (good) and FF_LAMBDA_MAX (bad))
     * - encoding: Set by libavcodec. for coded_picture (and set by user for input).
     * - decoding: Set by libavcodec.
     */
    private int quality;
    /**
     * 是否是隔行扫描
     */
    private int interlaced_frame;
//    uint8_t motion_subsample_log2：一个宏块中的运动矢量采样个数，取log的
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

    public byte[] getData() {
        return data;
    }

    public void setData(byte[] data) {
        this.data = data;
    }

    public float[] getSamples() {
        return samples;
    }

    public void setSamples(float[] samples) {
        this.samples = samples;
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

    public int getNb_samples() {
        return nb_samples;
    }

    public void setNb_samples(int nb_samples) {
        this.nb_samples = nb_samples;
    }

    public int getFormat() {
        return format;
    }

    public void setFormat(int format) {
        this.format = format;
    }

    public int getKey_frame() {
        return key_frame;
    }

    public void setKey_frame(int key_frame) {
        this.key_frame = key_frame;
    }

    public AVPictureType getPict_type() {
        return pict_type;
    }

    public void setPict_type(AVPictureType pict_type) {
        this.pict_type = pict_type;
    }

    public AVRational getSample_aspect_ratio() {
        return sample_aspect_ratio;
    }

    public void setSample_aspect_ratio(AVRational sample_aspect_ratio) {
        this.sample_aspect_ratio = sample_aspect_ratio;
    }

    public long getPts() {
        return pts;
    }

    public void setPts(long pts) {
        this.pts = pts;
    }

    public int getCoded_picture_number() {
        return coded_picture_number;
    }

    public void setCoded_picture_number(int coded_picture_number) {
        this.coded_picture_number = coded_picture_number;
    }

    public int getDisplay_picture_number() {
        return display_picture_number;
    }

    public void setDisplay_picture_number(int display_picture_number) {
        this.display_picture_number = display_picture_number;
    }

    public int getInterlaced_frame() {
        return interlaced_frame;
    }

    public void setInterlaced_frame(int interlaced_frame) {
        this.interlaced_frame = interlaced_frame;
    }

    public int[] getLinesize() {
        return linesize;
    }

    public int getPkt_pts() {
        return pkt_pts;
    }

    public int getPkt_dts() {
        return pkt_dts;
    }

    public int getQuality() {
        return quality;
    }

    public int getSample_rate() {
        return sample_rate;
    }

    public int getChannels() {
        return channels;
    }
}
