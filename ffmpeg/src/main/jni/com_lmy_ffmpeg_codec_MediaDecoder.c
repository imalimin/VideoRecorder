//
// Created by lmy on 2017/4/20.
//
#include <com_lmy_ffmpeg_codec_MediaDecoder.h>
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
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>

#define LOGE(format, ...)  __android_log_print(ANDROID_LOG_ERROR, "JNI", format, ##__VA_ARGS__)
#define LOGI(format, ...)  __android_log_print(ANDROID_LOG_INFO,  "JNI", format, ##__VA_ARGS__)
#else
#define LOGE(format, ...)  printf("JNI" format "\n", ##__VA_ARGS__)
#define LOGI(format, ...)  printf("JNI" format "\n", ##__VA_ARGS__)
#endif

//Output FFmpeg's av_log()
/*void custom_log(void *ptr, int level, const char* fmt, va_list vl){
    FILE *fp=fopen("/storage/emulated/0/av_log.txt","a+");
    if(fp){
        vfprintf(fp,fmt,vl);
        fflush(fp);
        fclose(fp);
    }
}*/

static char input[500] = {0};
static AVFormatContext *pFormatCtx;
static int video_index = -1, audio_index = -1, frame_rate = 0;
static AVCodecContext *pCodecCtx, *aCodecCtx;
//音频或视频解码器指针
static AVCodec *pCodec;
static AVFrame *pFrame, *pOutFrame, *pAudioFrame;
static AVPacket *packet;
static struct SwsContext *img_convert_ctx;
static int ret, got_frame;
static struct SwsContext *img_convert_ctx;
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
static jbyte* pBuffer;

static void print_type(int num, int type){
    char pict_type[10] = {0};
    switch(type){
        case AV_PICTURE_TYPE_I:
            sprintf(pict_type, "I");
            break;
        case AV_PICTURE_TYPE_P:
            sprintf(pict_type, "P");
            break;
        case AV_PICTURE_TYPE_B:
            sprintf(pict_type, "B");
            break;
        default:
            sprintf(pict_type, "Other");
    }
    LOGI("第%5d帧，类型：%s",num, pict_type);
}

static void init_info(JNIEnv *env, jobject thiz){
    jfieldID  fieldId;
    jclass cls = (*env)->GetObjectClass(env, thiz);//获得Java层该对象实例的类引用

    fieldId = (*env)->GetFieldID(env, cls , "width" , "I");//获得属性句柄
    (*env)->SetIntField(env, thiz , fieldId, pCodecCtx -> width);//获得属性值
    fieldId = (*env)->GetFieldID(env, cls , "height" , "I");
    (*env)->SetIntField(env, thiz , fieldId, pCodecCtx -> height);
    fieldId = (*env)->GetFieldID(env, cls , "frameRate" , "I");
    (*env)->SetIntField(env, thiz , fieldId, frame_rate);
    fieldId = (*env)->GetFieldID(env, cls , "sample_rate" , "I");
    (*env)->SetIntField(env, thiz , fieldId, aCodecCtx -> sample_rate);
    fieldId = (*env)->GetFieldID(env, cls , "channels" , "I");
    (*env)->SetIntField(env, thiz , fieldId, aCodecCtx -> channels);
    LOGI("init: 分辨率=%dx%d，帧率=%d，音频采样率=%d，声道数量=%d",
    pCodecCtx -> width, pCodecCtx -> height, frame_rate, aCodecCtx -> sample_rate, aCodecCtx -> channels);
}

static void audio_swr_init(){
    dst_sample_fmt = AV_SAMPLE_FMT_S16P;
    enum AVSampleFormat src_sample_fmt = aCodecCtx -> sample_fmt;
    if(src_sample_fmt == dst_sample_fmt){//aCodecCtx -> sample_fmt == AV_SAMPLE_FMT_S16 || aCodecCtx -> sample_fmt == AV_SAMPLE_FMT_S32 || aCodecCtx -> sample_fmt == AV_SAMPLE_FMT_U8
        LOGE("codec->sample_fmt:%d",src_sample_fmt);
        if(NULL != audio_convert_ctx){
            swr_free(audio_convert_ctx);
            audio_convert_ctx = NULL;
        }
        return;
    }
    if(NULL != audio_convert_ctx){
        swr_free(audio_convert_ctx);
    }
    int ret = 0;
    int64_t src_ch_layout = aCodecCtx -> channel_layout;
    dst_ch_layout = aCodecCtx -> channel_layout;
    int src_rate = aCodecCtx -> sample_rate;
    dst_rate = aCodecCtx -> sample_rate;
    int src_nb_samples = 1024;

    /* create resampler context */
    audio_convert_ctx = swr_alloc();
    if (!audio_convert_ctx) {
        LOGE("Could not allocate resampler context\n");
        return;
    }
    /* set options */
    av_opt_set_int(audio_convert_ctx, "in_channel_layout",    src_ch_layout, 0);
    av_opt_set_int(audio_convert_ctx, "in_sample_rate",       src_rate, 0);
    av_opt_set_sample_fmt(audio_convert_ctx, "in_sample_fmt", src_sample_fmt, 0);

    av_opt_set_int(audio_convert_ctx, "out_channel_layout",    dst_ch_layout, 0);
    av_opt_set_int(audio_convert_ctx, "out_sample_rate",       dst_rate, 0);
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
    ret = av_samples_alloc_array_and_samples(pAudioFrame -> data, pAudioFrame -> linesize, dst_nb_channels,
                                             dst_nb_samples, dst_sample_fmt, 0);
    if (ret < 0) {
        LOGE("Could not allocate destination samples\n");
        return;
    }
}

void init_frame(JNIEnv *env, AVFrame* av, jobject frame){
    if(frame == NULL){
        LOGE("AVFrame is NULL!");
        return;
    }

    jfieldID  fieldId;
    jclass cls = (*env)->GetObjectClass(env, frame);//获得Java层该对象实例的类引用
    fieldId = (*env)->GetFieldID(env, cls , "width" , "I");//获得属性句柄
    (*env)->SetIntField(env, frame , fieldId, av -> width);//设置属性值
    fieldId = (*env)->GetFieldID(env, cls , "height" , "I");
    (*env)->SetIntField(env, frame , fieldId, av -> height);
    fieldId = (*env)->GetFieldID(env, cls , "format" , "I");
    (*env)->SetIntField(env, frame , fieldId, av -> format);
    fieldId = (*env)->GetFieldID(env, cls , "sample_rate" , "I");
    (*env)->SetIntField(env, frame , fieldId, av -> sample_rate);
    fieldId = (*env)->GetFieldID(env, cls , "nb_samples" , "I");
    (*env)->SetIntField(env, frame , fieldId, av -> nb_samples);
    fieldId = (*env)->GetFieldID(env, cls , "channels" , "I");
    (*env)->SetIntField(env, frame , fieldId, av -> channels);
}

static void swap_frame(JNIEnv *env, AVFrame* av, jobject frame){
    if(frame == NULL){
        LOGE("AVFrame is NULL!");
        return;
    }
    clock_t start = clock();
    init_frame(env ,av, frame);
    int width = av -> width;
    int height = av -> height;

    jfieldID  fieldId;
    jclass cls = (*env)->GetObjectClass(env, frame);//获得Java层该对象实例的类引用

    fieldId = (*env)->GetFieldID(env, cls , "data" , "[B");

    int len = 0;
    int size = av_image_get_buffer_size(av -> format, width, height, 1);

    switch(av -> format){
        case AV_PIX_FMT_YUV420P:
            LOGI("AV_PIX_FMT_YUV420P %d, %d, %d", width, height, av -> linesize[0]);
            /*size = width * height * 4;
            buffer = (*env)->NewByteArray(env, size);
            pBuffer = (*env)->GetByteArrayElements(env, buffer, 0);
            yuv420_2_rgb8888(pBuffer, av -> data[0], av -> data[1], av -> data[2], width, height);*/
            //cvt_i420_NV21(av -> data, pBuffer, width, height);
            size = width * height * 4;
            buffer = (*env)->NewByteArray(env, size);
            pBuffer = (*env)->GetByteArrayElements(env, buffer, 0);
            I420ToARGB(av -> data[0], av -> linesize[0],
            av -> data[2], av -> linesize[2],
            av -> data[1], av -> linesize[1],
            pBuffer, width * 4, width, height);
            /*memcpy(pBuffer, av -> data[0], av -> linesize[0] * height);
            len = av -> linesize[1] * height / 2;
            memcpy(pBuffer + av -> linesize[0] * height, av -> data[1], len);
            memcpy(pBuffer + av -> linesize[0] * height + len, av -> data[2], len);*/
            break;
        default:
            buffer = (*env)->NewByteArray(env, size);
            pBuffer = (*env)->GetByteArrayElements(env, buffer, 0);
            len = av -> linesize[0] * height;
            memcpy(pBuffer, av -> data[0], len);
    }

    (*env)->SetByteArrayRegion(env, buffer, 0, size, pBuffer);
    (*env)->SetObjectField(env, frame , fieldId, buffer);
    LOGI("swap_frame time = %d", (long)((clock() - start)/1000));
}

static void swap_audio_frame(JNIEnv *env, AVFrame* av, jobject frame){
    if(frame == NULL){
        LOGE("AVFrame is NULL!");
        return;
    }
    init_frame(env, av, frame);

    int data_size = av_get_bytes_per_sample(dst_sample_fmt);
    if (data_size < 0) {
        /* This should not occur, checking just for paranoia */
        LOGE("Failed to calculate data size\n");
        return;
    }

    jfieldID  fieldId;
    jclass cls = (*env)->GetObjectClass(env, frame);//获得Java层该对象实例的类引用
    fieldId = (*env)->GetFieldID(env, cls , "data" , "[B");

    int nb_samples = av -> nb_samples, channels = aCodecCtx -> channels;
    int size = av -> linesize[0] * channels;
    //LOGE("audio buffer %d %d %d %d", nb_samples, channels, size, data_size);
    buffer = (*env)->NewByteArray(env, size);
    pBuffer = (*env)->GetByteArrayElements(env, buffer, 0);

    int i = 0, ch = 0, count = 0;
    for (i=0; i < nb_samples; i++)
        for (ch=0; ch< channels; ch++){
            memcpy(pBuffer + count, av -> data[ch] + data_size * i, data_size);
            count += data_size;
        }

    //LOGI("samples: %d %d %d %d %d", pBuffer[0], pBuffer[1], pBuffer[2], pBuffer[3], pBuffer[4]);
    //LOGI("samples: %d %d %d %d %d", av -> data[0][0], av -> data[0][1], av -> data[1][0], av -> data[1][1], av -> data[0][2]);

    (*env)->SetByteArrayRegion(env, buffer, 0, size, pBuffer);
    (*env)->SetObjectField(env, frame , fieldId, buffer);
}

/*
 * Class:     com_lmy_ffmpeg_codec_MediaDecoder
 * Method:    setDataSource
 * Signature: (Ljava/lang/String;Lcom/lmy/ffmpeg/codec/AVFrame;)V
 */
JNIEXPORT void JNICALL Java_com_lmy_ffmpeg_codec_MediaDecoder_setDataSource
  (JNIEnv *env, jobject thiz, jstring path, jobject frame){
    avframe = (*env)->NewGlobalRef(env, frame);
    sprintf(input, "%s", (*env)->GetStringUTFChars(env, path, NULL));
    video_index = -1;
    frame_cnt = 0;

    av_register_all();
    avformat_network_init();

    pFormatCtx = avformat_alloc_context();

    if(avformat_open_input(&pFormatCtx, input, NULL, NULL) != 0){
        LOGE("无法打开输入文件\n");
        return;
    }
    if(avformat_find_stream_info(pFormatCtx, NULL) < 0){
        LOGE("无法获取输入流信息\n");
        return;
    }
    int i = 0;
    for(i = 0; i < pFormatCtx -> nb_streams; i++){
        int type = pFormatCtx -> streams[i] -> codec -> codec_type;
        if(type == AVMEDIA_TYPE_VIDEO){
            video_index = i;
            frame_rate = pFormatCtx -> streams[video_index] -> r_frame_rate.num;
        }else if(type == AVMEDIA_TYPE_AUDIO){
            audio_index = i;
        }
        if(video_index != -1 && audio_index != -1) break;
    }
    if(video_index == -1){
        LOGE("无法找到视频流信息\n");
        return;
    }
    if(audio_index == -1){
        LOGE("无法找到音频流信息\n");
    }

    pCodecCtx = pFormatCtx ->streams[video_index] -> codec;
    pCodec = avcodec_find_decoder(pCodecCtx -> codec_id);
    if(pCodec == NULL){
        LOGE("没有合适的视频解码器\n");
        return;
    }
    if(avcodec_open2(pCodecCtx, pCodec, NULL) < 0){
        LOGE("无法打开视频解码器\n");
        return;
    }

    aCodecCtx = pFormatCtx ->streams[audio_index] -> codec;
    pCodec = avcodec_find_decoder(aCodecCtx -> codec_id);
    if(pCodec == NULL){
        LOGE("没有合适的音频解码器\n");
        return;
    }
    if(avcodec_open2(aCodecCtx, pCodec, NULL) < 0){
        LOGE("无法打开音频解码器\n");
        return;
    }

    pFrame = av_frame_alloc();
    pOutFrame = av_frame_alloc();
    pOutFrame -> format = AV_PIX_FMT_RGB565LE;
    pOutFrame -> width = pCodecCtx -> width;
    pOutFrame -> height = pCodecCtx -> height;
    int size = av_image_get_buffer_size(pOutFrame -> format, pCodecCtx -> width, pCodecCtx -> height, 1);
    uint8_t* out_buffer = (unsigned char *)av_malloc(size);
    av_image_fill_arrays(pOutFrame -> data, pOutFrame -> linesize, out_buffer, pOutFrame -> format, pCodecCtx -> width,pCodecCtx -> height, 1);

    //av_init_packet(&packet);
    packet = (AVPacket *)av_malloc(sizeof(AVPacket));
    /**
    * 图像拉伸
    * #define SWS_FAST_BILINEAR     1
    * #define SWS_BILINEAR          2
    * #define SWS_BICUBIC           4
    * #define SWS_X                 8
    * #define SWS_POINT          0x10
    * #define SWS_AREA           0x20
    * #define SWS_BICUBLIN       0x40
    * #define SWS_GAUSS          0x80
    * #define SWS_SINC          0x100
    * #define SWS_LANCZOS       0x200
    * #define SWS_SPLINE        0x400
    **/
    img_convert_ctx = sws_getContext(pCodecCtx -> width, pCodecCtx -> height, pCodecCtx -> pix_fmt,
    pCodecCtx -> width, pCodecCtx -> height, pOutFrame -> format, SWS_BICUBIC, NULL, NULL, NULL);

    //audio_sws_init();
    init_info(env, thiz);
    audio_swr_init();
  }

/*
 * Class:     com_lmy_ffmpeg_codec_MediaDecoder
 * Method:    nextFrame
 * Signature: ()V
 */
JNIEXPORT jint JNICALL Java_com_lmy_ffmpeg_codec_MediaDecoder_nextFrame
  (JNIEnv *env, jobject thiz){
    clock_t start = clock();
    while(av_read_frame(pFormatCtx, packet) >= 0){
        if(packet -> stream_index == video_index){
            ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_frame, packet);
            if(ret < 0){
                LOGE("帧解码失败\n");
                return (jint)-1;
            }
            if(!got_frame)
                continue;
            LOGI("decode time = %d", (long)((clock() - start)/1000));
            //sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame -> data, pFrame -> linesize,
            //0, pCodecCtx -> height, pOutFrame -> data, pOutFrame -> linesize);
            LOGI("sws_scale time = %d", (long)((clock() - start)/1000));
            print_type(frame_cnt, pFrame -> pict_type);
            swap_frame(env, pFrame, avframe);
            frame_cnt++;
            av_free_packet(packet);
            return (jint)0;
        }else if(packet -> stream_index == audio_index && aCodecCtx != NULL){
            ret = avcodec_decode_audio4(aCodecCtx, pFrame, &got_frame, packet);
            if(ret < 0){
                LOGE("音频解码失败\n");
                return (jint)-1;
            }
            if(!got_frame)
                continue;
            LOGI("音频: %d %d %d %d", pFrame -> sample_rate, pFrame -> nb_samples, pFrame -> linesize[0], aCodecCtx -> sample_fmt);

            ret = av_samples_alloc(pAudioFrame -> data, pAudioFrame -> linesize, dst_nb_channels, dst_nb_samples, dst_sample_fmt, 0);
            //LOGE("av_samples_alloc: %d %d %d %d %d", pAudioFrame -> nb_samples, aCodecCtx -> sample_fmt, dst_nb_samples, dst_sample_fmt, AV_SAMPLE_FMT_S16);
            if (ret < 0){
                LOGE("AudioFrame分配失败");
            }
            ret = swr_convert(audio_convert_ctx, pAudioFrame -> data, dst_nb_samples, pFrame -> data, pFrame -> nb_samples);
            if (ret < 0){
                LOGE("sample转换失败");
            }
            pAudioFrame -> channels = aCodecCtx -> channels;
            pAudioFrame -> sample_rate = dst_rate;
            pAudioFrame -> nb_samples = dst_nb_samples;
            //LOGI("swr_convert %d %d %d %d", dst_rate, dst_nb_samples, pAudioFrame -> linesize[0], dst_sample_fmt);
            //LOGI("src samples: %d %d %d %d %d", pFrame -> data[0][0], pFrame -> data[0][1], pFrame -> data[1][0], pFrame -> data[1][1], pFrame -> data[0][2]);
            swap_audio_frame(env, pAudioFrame, avframe);
            return (jint)0;
        }
        av_free_packet(packet);
     }
    while(1){
        ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_frame, packet);
        if(ret < 0){
            LOGE("帧解码失败\n");
            break;
        }
        if(!got_frame)
            break;
        //sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame -> data, pFrame -> linesize,
        //0, pCodecCtx -> height, pOutFrame -> data, pOutFrame -> linesize);
        print_type(frame_cnt, pFrame -> pict_type);
        swap_frame(env, pFrame, avframe);
        frame_cnt++;
        return (jint)0;
    }
    LOGI("解码完成");
    //(*env)->DeleteGlobalRef(env, avframe);
    return (jint)1;
  }

/*
 * Class:     com_lmy_ffmpeg_codec_MediaDecoder
 * Method:    release
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_lmy_ffmpeg_codec_MediaDecoder_release
  (JNIEnv *env, jobject thiz){
    sws_freeContext(img_convert_ctx);
    av_frame_free(&pFrame);
    avcodec_close(pCodecCtx);
    avcodec_close(aCodecCtx);
    avformat_close_input(&pFormatCtx);
  }
