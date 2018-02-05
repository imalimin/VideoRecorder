
#include <com_lmy_ffmpeg_codec_VideoEncoder.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#include "libyuv.h"
//Log
#ifdef ANDROID

#include <jni.h>
#include <android/log.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/mem.h>
#include <libavformat/avio.h>
#include <libavutil/frame.h>
#include <libavutil/dict.h>
#include <libavutil/avutil.h>
#include <libavutil/pixfmt.h>
#include <libavfilter/avcodec.h>
#include <libavcodec/dv_profile.h>
#include <libavfilter/buffersrc.h>
#include <libavfilter/buffersink.h>
#include <libyuv/rotate.h>

#define LOGE(format, ...)  __android_log_print(ANDROID_LOG_ERROR, "JNI", format, ##__VA_ARGS__)
#define LOGI(format, ...)  __android_log_print(ANDROID_LOG_INFO,  "JNI", format, ##__VA_ARGS__)
#else
#define LOGE(format, ...)  printf("JNI" format "\n", ##__VA_ARGS__)
#define LOGI(format, ...)  printf("JNI" format "\n", ##__VA_ARGS__)
#endif

const char *output;
AVFormatContext *pFormatCtx;
AVOutputFormat *fmt;
AVStream *video_st;
AVCodecContext *pCodecCtx;
AVCodec *pCodec;
AVPacket pkt;
uint8_t *picture_buf;
AVFrame *pFrame;
int picture_size;
int y_size;
int frame_count = 0;
int width = 0, height = 0;
int src_width = 0, src_height = 0;
const uint8_t *buffer;

static long long get_now() {
    struct timeval xTime;
    gettimeofday(&xTime, NULL);
    long long xFactor = 1;
    return (xFactor * xTime.tv_sec * 1000) + (xTime.tv_usec / 1000);
}

static void crop_frame(uint8_t *src) {
    int i = 0, offset = (src_width - height) / 2;
    for (i = 0; i < src_height * 1.5; i++) {
        memcpy(buffer + i * height, src + i * src_width + offset, height);
    }
}

static void convert_frame(uint8_t *src) {
    //1280x720 crop to 480x720
    crop_frame(src);
    //480x720 rotate to 720x480
    NV12ToI420Rotate(buffer, height,
                     buffer + y_size, height,
                     pFrame->data[0], width,
                     pFrame->data[2], width / 2,
                     pFrame->data[1], width / 2,
                     height, width, kRotate90);
}

static int init(jint s_w, jint s_h, jint w, jint h) {
    width = w;
    height = h;
    src_width = s_w;
    src_height = s_h;
    LOGI("init src(%dx%d), dest(%dx%d)", src_width, src_height, width, height);

    av_register_all();
    avformat_alloc_output_context2(&pFormatCtx, NULL, "mp4", output);
//    pFormatCtx = avformat_alloc_context();
    //Guess Format
    fmt = pFormatCtx->oformat;

    if (avio_open(&pFormatCtx->pb, output, AVIO_FLAG_READ_WRITE) < 0) {
        LOGE("Failed to open output file!");
        return -1;
    }
    video_st = avformat_new_stream(pFormatCtx, 0);
    if (video_st == NULL) {
        return -1;
    }

    pCodecCtx = video_st->codec;
    pCodecCtx->codec_id = AV_CODEC_ID_MPEG4;
    pCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
    pCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
    pCodecCtx->width = width;
    pCodecCtx->height = height;
    pCodecCtx->bit_rate = 1024000;
    pCodecCtx->gop_size = 250;

    pCodecCtx->time_base.num = 1;
    pCodecCtx->time_base.den = 25;

    pCodecCtx->qmin = 10;
    pCodecCtx->qmax = 51;
    pCodecCtx->max_b_frames = 3;

    //Set Option
    AVDictionary *param = 0;
    //H.264
    if (pCodecCtx->codec_id == AV_CODEC_ID_H264) {
        LOGI("AV_CODEC_ID_H264");
        av_dict_set(&param, "preset", "slow", 0);
        av_dict_set(&param, "tune", "zerolatency", 0);
        //av_dict_set(param, "profile", "main", 0);
    }
    //Show some Information
    //av_dump_format(pFormatCtx, 0, output, 1);

    pCodec = avcodec_find_encoder(pCodecCtx->codec_id);
    if (!pCodec) {
        LOGE("Can not find encoder!");
        return -1;
    }
    if (avcodec_open2(pCodecCtx, pCodec, &param) < 0) {
        LOGE("Failed to open encoder!");
        return -1;
    }
    avformat_write_header(pFormatCtx, NULL);

    pFrame = av_frame_alloc();
    picture_size = avpicture_get_size(pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height);
    picture_buf = (uint8_t *) av_malloc(picture_size);
    avpicture_fill((AVPicture *) pFrame, picture_buf, pCodecCtx->pix_fmt, pCodecCtx->width,
                   pCodecCtx->height);
    av_new_packet(&pkt, picture_size);
    y_size = pCodecCtx->width * pCodecCtx->height;

    buffer = malloc(sizeof(uint8_t) * (width * height * 3 / 2));
    if (NULL == buffer) {
        LOGE("malloc failed!");
        return -1;
    }
    return 0;
}

JNIEXPORT jint JNICALL Java_com_lmy_ffmpeg_codec_VideoEncoder_init
        (JNIEnv *env, jobject thiz, jstring out_path, jint s_w, jint s_h, jint w,
         jint h) {
    output = (*env)->GetStringUTFChars(env, out_path, NULL);

    int ret = init(s_w, s_h, w, h);

    (*env)->ReleaseStringUTFChars(env, out_path, output);
    return ret;
}

static void encode(uint8_t *data_p) {
    LOGI("encoding...");
    convert_frame(data_p);
//    pFrame->pts = frame_count * (video_st->time_base.den) / ((video_st->time_base.num) * 25);
    int got_picture = 0;
    //Encode
    int ret = avcodec_encode_video2(pCodecCtx, &pkt, pFrame, &got_picture);
    if (ret < 0) {
        LOGI("Failed to encode!");
        return;
    }

    if (got_picture == 1) {
        pkt.pts = frame_count++ * (video_st->time_base.den) / ((video_st->time_base.num) * 25);
        pkt.stream_index = video_st->index;
        pkt.dts = pkt.pts;
        pkt.duration = 1;
        ret = av_write_frame(pFormatCtx, &pkt);
        av_free_packet(&pkt);
        LOGI("Succeed to encode frame: %d", frame_count);
    }
}

JNIEXPORT void JNICALL Java_com_lmy_ffmpeg_codec_VideoEncoder_encode
        (JNIEnv *env, jobject thiz, jbyteArray data) {
    //jbyteArray转为jbyte*
    jbyte *data_p = (*env)->GetByteArrayElements(env, data, NULL);

    long long start = get_now();
    encode((uint8_t *) data_p);
    int d = (int) (get_now() - start);
    LOGI("cost: %dms, fps: %.1f", d, 1000 / (float) d);

    (*env)->ReleaseByteArrayElements(env, data, data_p, NULL);
}

static int flush_encoder(AVFormatContext *fmt_ctx, unsigned int stream_index) {
    int ret;
    int got_frame;
    AVPacket enc_pkt;
    if (!(fmt_ctx->streams[stream_index]->codec->codec->capabilities &
          CODEC_CAP_DELAY))
        return 0;
    while (1) {
        enc_pkt.data = NULL;
        enc_pkt.size = 0;
        av_init_packet(&enc_pkt);
        ret = avcodec_encode_video2(fmt_ctx->streams[stream_index]->codec, &enc_pkt,
                                    NULL, &got_frame);
        av_frame_free(NULL);
        if (ret < 0)
            break;
        if (!got_frame) {
            ret = 0;
            break;
        }
        printf("Flush Encoder: Succeed to encode 1 frame!\tsize:%5d\n", enc_pkt.size);
        /* mux encoded frame */
        ret = av_write_frame(fmt_ctx, &enc_pkt);
        if (ret < 0)
            break;
    }
    return ret;
}

JNIEXPORT void JNICALL Java_com_lmy_ffmpeg_codec_VideoEncoder_flush
        (JNIEnv *env, jobject thiz) {
    free(buffer);
    //Flush Encoder
    if (pFormatCtx) {
        int ret = flush_encoder(pFormatCtx, 0);
        if (ret < 0) {
            LOGE("Flushing encoder failed");
//            return;
        }
        //Write file trailer
        if (frame_count > 0)
            av_write_trailer(pFormatCtx);
    }

    //Clean
    if (video_st)
        avcodec_close(video_st->codec);
    if (pFrame)
        av_free(pFrame);
    if (picture_buf)
        av_free(picture_buf);
    if (pFormatCtx) {
        avio_close(pFormatCtx->pb);
        avformat_free_context(pFormatCtx);
    }
}