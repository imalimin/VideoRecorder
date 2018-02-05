//
// Created by lmy on 2017/4/28.
//
#include <com_lmy_ffmpeg_codec_AudioDecoder.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#include "libyuv.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
#include "libavutil/log.h"
//Log
#ifdef ANDROID

#include <jni.h>
#include <android/log.h>
#include <libavutil/opt.h>

#define LOGE(format, ...)  __android_log_print(ANDROID_LOG_ERROR, "JNI", format, ##__VA_ARGS__)
#define LOGI(format, ...)  __android_log_print(ANDROID_LOG_INFO,  "JNI", format, ##__VA_ARGS__)
#else
#define LOGE(format, ...)  printf("JNI" format "\n", ##__VA_ARGS__)
#define LOGI(format, ...)  printf("JNI" format "\n", ##__VA_ARGS__)
#endif

static char input[500] = {0};
static AVFormatContext *pFormatCtx;
static int audio_index = -1;
static AVCodecContext *aCodecCtx;
static AVCodec *pCodec;
static AVFrame *pFrame, *pAudioFrame;
static AVPacket *packet;
static int ret, got_frame;
static struct SwrContext *audio_convert_ctx;
static int frame_cnt;
//音频数据转换
static enum AVSampleFormat dst_sample_fmt;
static int dst_rate;
static int max_dst_nb_samples;
static int dst_nb_channels;
static int dst_nb_samples;
static int64_t dst_ch_layout;
//缓存帧
static jobject avframe;
static jbyteArray buffer;
static jbyte *pBuffer;

static void audio_sws_init() {
    dst_sample_fmt = AV_SAMPLE_FMT_S16P;
    enum AVSampleFormat src_sample_fmt = aCodecCtx->sample_fmt;
    if (src_sample_fmt ==
        dst_sample_fmt) {//aCodecCtx -> sample_fmt == AV_SAMPLE_FMT_S16 || aCodecCtx -> sample_fmt == AV_SAMPLE_FMT_S32 || aCodecCtx -> sample_fmt == AV_SAMPLE_FMT_U8
        LOGE("codec->sample_fmt:%d", src_sample_fmt);
        if (NULL != audio_convert_ctx) {
            swr_free(audio_convert_ctx);
            audio_convert_ctx = NULL;
        }
        return;
    }
    if (NULL != audio_convert_ctx) {
        swr_free(audio_convert_ctx);
    }
    int ret = 0;
    int64_t src_ch_layout = aCodecCtx->channel_layout;
    dst_ch_layout = aCodecCtx->channel_layout;
    int src_rate = aCodecCtx->sample_rate;
    dst_rate = aCodecCtx->sample_rate;
    int src_nb_samples = 1024;

    /* create resampler context */
    audio_convert_ctx = swr_alloc();
    if (!audio_convert_ctx) {
        LOGE("Could not allocate resampler context\n");
        return;
    }
    /* set options */
    av_opt_set_int(audio_convert_ctx, "in_channel_layout", src_ch_layout, 0);
    av_opt_set_int(audio_convert_ctx, "in_sample_rate", src_rate, 0);
    av_opt_set_sample_fmt(audio_convert_ctx, "in_sample_fmt", src_sample_fmt, 0);

    av_opt_set_int(audio_convert_ctx, "out_channel_layout", dst_ch_layout, 0);
    av_opt_set_int(audio_convert_ctx, "out_sample_rate", dst_rate, 0);
    av_opt_set_sample_fmt(audio_convert_ctx, "out_sample_fmt", dst_sample_fmt, 0);
    /* initialize the resampling context */
    if ((ret = swr_init(audio_convert_ctx)) < 0) {
        LOGE("Failed to initialize the resampling context\n");
        return;
    }

    pAudioFrame = av_frame_alloc();

    /* compute the number of converted samples: buffering is avoided
     * ensuring that the output buffer will contain at least all the
     * converted input samples */
    max_dst_nb_samples = dst_nb_samples =
            av_rescale_rnd(src_nb_samples, dst_rate, src_rate, AV_ROUND_UP);

    /* buffer is going to be directly written to a rawaudio file, no alignment */
    dst_nb_channels = av_get_channel_layout_nb_channels(dst_ch_layout);
    LOGE("dst_nb_channels=%d", dst_nb_channels);
    ret = av_samples_alloc_array_and_samples(pAudioFrame->data, pAudioFrame->linesize,
                                             dst_nb_channels,
                                             dst_nb_samples, dst_sample_fmt, 0);
    if (ret < 0) {
        LOGE("Could not allocate destination samples\n");
        return;
    }
}

void init_audio_info(JNIEnv *env, jobject thiz) {
    jfieldID fieldId;
    jclass cls = (*env)->GetObjectClass(env, thiz);//获得Java层该对象实例的类引用
    fieldId = (*env)->GetFieldID(env, cls, "sample_rate", "I");
    (*env)->SetIntField(env, thiz, fieldId, aCodecCtx->sample_rate);
    fieldId = (*env)->GetFieldID(env, cls, "channels", "I");
    (*env)->SetIntField(env, thiz, fieldId, aCodecCtx->channels);
    LOGI("init %d %d", aCodecCtx->sample_rate, aCodecCtx->channels);
}

void init_audio_frame(JNIEnv *env, AVFrame *av, jobject frame) {
    if (frame == NULL) {
        LOGE("AVFrame is NULL!");
        return;
    }

    jfieldID fieldId;
    jclass cls = (*env)->GetObjectClass(env, frame);//获得Java层该对象实例的类引用
    fieldId = (*env)->GetFieldID(env, cls, "width", "I");//获得属性句柄
    (*env)->SetIntField(env, frame, fieldId, av->width);//设置属性值
    fieldId = (*env)->GetFieldID(env, cls, "height", "I");
    (*env)->SetIntField(env, frame, fieldId, av->height);
    fieldId = (*env)->GetFieldID(env, cls, "format", "I");
    (*env)->SetIntField(env, frame, fieldId, av->format);
    fieldId = (*env)->GetFieldID(env, cls, "sample_rate", "I");
    (*env)->SetIntField(env, frame, fieldId, av->sample_rate);
    fieldId = (*env)->GetFieldID(env, cls, "nb_samples", "I");
    (*env)->SetIntField(env, frame, fieldId, av->nb_samples);
    fieldId = (*env)->GetFieldID(env, cls, "channels", "I");
    (*env)->SetIntField(env, frame, fieldId, av->channels);
}

void _swap_audio_frame(JNIEnv *env, AVFrame *av, jobject frame) {
    if (frame == NULL) {
        LOGE("AVFrame is NULL!");
        return;
    }
//    init_frame(env, av, frame);

    int data_size = av_get_bytes_per_sample(dst_sample_fmt);
    if (data_size < 0) {
        /* This should not occur, checking just for paranoia */
        LOGE("Failed to calculate data size\n");
        return;
    }

    jfieldID fieldId;
    jclass cls = (*env)->GetObjectClass(env, frame);//获得Java层该对象实例的类引用
    fieldId = (*env)->GetFieldID(env, cls, "data", "[B");

    int nb_samples = av->nb_samples, channels = aCodecCtx->channels;
    int size = av->linesize[0] * channels;
    LOGE("audio buffer %d %d %d %d", nb_samples, channels, size, data_size);
    buffer = (*env)->NewByteArray(env, size);
    pBuffer = (*env)->GetByteArrayElements(env, buffer, 0);

    int i = 0, ch = 0, count = 0;
    for (i = 0; i < nb_samples; i++)
        for (ch = 0; ch < channels; ch++) {
            memcpy(pBuffer + count, av->data[ch] + data_size * i, data_size);
            count += data_size;
        }

    LOGI("samples: %d %d %d %d %d", pBuffer[0], pBuffer[1], pBuffer[2], pBuffer[3], pBuffer[4]);
    LOGI("samples: %d %d %d %d %d", av->data[0][0], av->data[0][1], av->data[1][0], av->data[1][1],
         av->data[0][2]);

    (*env)->SetByteArrayRegion(env, buffer, 0, size, pBuffer);
    (*env)->SetObjectField(env, frame, fieldId, buffer);
}

/*
 * Class:     com_lmy_ffmpeg_codec_AudioDecoder
 * Method:    setDataSource
 * Signature: (Ljava/lang/String;Lcom/lmy/ffmpeg/codec/AVFrame;)V
 */
JNIEXPORT void JNICALL Java_com_lmy_ffmpeg_codec_AudioDecoder_setDataSource
        (JNIEnv *env, jobject thiz, jstring path, jobject frame) {
    avframe = (*env)->NewGlobalRef(env, frame);
    sprintf(input, "%s", (*env)->GetStringUTFChars(env, path, NULL));
    audio_index = -1;
    frame_cnt = 0;

    av_register_all();
    avformat_network_init();

    pFormatCtx = avformat_alloc_context();

    if (avformat_open_input(&pFormatCtx, input, NULL, NULL) != 0) {
        LOGE("无法打开输入文件\n");
        return;
    }
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        LOGE("无法获取输入流信息\n");
        return;
    }
    int i = 0;
    for (i = 0; i < pFormatCtx->nb_streams; i++) {
        int type = pFormatCtx->streams[i]->codec->codec_type;
        if (type == AVMEDIA_TYPE_AUDIO) {
            audio_index = i;
        }
    }

    aCodecCtx = pFormatCtx->streams[audio_index]->codec;
    pCodec = avcodec_find_decoder(aCodecCtx->codec_id);
    if (pCodec == NULL) {
        LOGE("没有合适的音频解码器\n");
        return;
    }
    if (avcodec_open2(aCodecCtx, pCodec, NULL) < 0) {
        LOGE("无法打开音频解码器\n");
        return;
    }

    pFrame = av_frame_alloc();
    packet = (AVPacket *) av_malloc(sizeof(AVPacket));

    init_audio_info(env, thiz);
    audio_sws_init();
}

/*
 * Class:     com_lmy_ffmpeg_codec_AudioDecoder
 * Method:    nextFrame
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_lmy_ffmpeg_codec_AudioDecoder_nextFrame
        (JNIEnv *env, jobject thiz) {
    while (av_read_frame(pFormatCtx, packet) >= 0) {
        if (packet->stream_index == audio_index && aCodecCtx != NULL) {
            ret = avcodec_decode_audio4(aCodecCtx, pFrame, &got_frame, packet);
            if (ret < 0) {
                LOGE("音频解码失败\n");
                return (jint) -1;
            }
            if (!got_frame)
                continue;
            LOGI("音频: %d %d %d %d", pFrame->sample_rate, pFrame->nb_samples, pFrame->linesize[0],
                 aCodecCtx->sample_fmt);

            /* compute destination number of samples */
            /*dst_nb_samples = av_rescale_rnd(swr_get_delay(audio_convert_ctx, pFrame -> sample_rate) + pFrame -> nb_samples, dst_rate, pFrame -> sample_rate, AV_ROUND_UP);
            if (dst_nb_samples > max_dst_nb_samples) {
                av_freep(pAudioFrame -> data);
                ret = av_samples_alloc(pAudioFrame -> data,pAudioFrame -> linesize, dst_nb_channels,
                                   dst_nb_samples, dst_sample_fmt, 1);
                if (ret < 0)
                    break;
                max_dst_nb_samples = dst_nb_samples;
            }

            ret = swr_convert(audio_convert_ctx, pAudioFrame -> data, dst_nb_samples, pFrame -> data, pFrame -> nb_samples);
            if(ret < 0){
                LOGE("音频解码失败\n");
                return (jint)-1;
            }*/
            ret = av_samples_alloc(pAudioFrame->data, pAudioFrame->linesize, dst_nb_channels,
                                   dst_nb_samples, dst_sample_fmt, 0);
            //LOGE("av_samples_alloc: %d %d %d %d %d", pAudioFrame -> nb_samples, aCodecCtx -> sample_fmt, dst_nb_samples, dst_sample_fmt, AV_SAMPLE_FMT_S16);
            if (ret < 0) {
                LOGE("AudioFrame分配失败");
            }
            LOGE("av_samples_alloc");
            ret = swr_convert(audio_convert_ctx, pAudioFrame->data, dst_nb_samples, pFrame->data,
                              pFrame->nb_samples);
            if (ret < 0) {
                LOGE("sample转换失败");
            }
            pAudioFrame->channels = aCodecCtx->channels;
            pAudioFrame->sample_rate = dst_rate;
            pAudioFrame->nb_samples = dst_nb_samples;
            LOGI("swr_convert %d %d %d %d", dst_rate, dst_nb_samples, pAudioFrame->linesize[0],
                 dst_sample_fmt);
            //LOGI("src samples: %d %d %d %d %d", pFrame -> data[0][0], pFrame -> data[0][1], pFrame -> data[1][0], pFrame -> data[1][1], pFrame -> data[0][2]);
            _swap_audio_frame(env, pAudioFrame, avframe);
            return (jint) 0;
        }
        av_free_packet(packet);
    }
    LOGI("解码完成");
    return (jint) 1;
}

/*
 * Class:     com_lmy_ffmpeg_codec_AudioDecoder
 * Method:    release
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_lmy_ffmpeg_codec_AudioDecoder_release
        (JNIEnv *env, jobject thiz) {
    av_frame_free(&pFrame);
    avcodec_close(aCodecCtx);
    avformat_close_input(&pFormatCtx);
}