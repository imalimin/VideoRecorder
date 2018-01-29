//
// Created by lmy on 2017/4/20.
//
#include <com_lmy_ffmpeg_player_Player.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/log.h"
//Log
#ifdef ANDROID
#include <jni.h>
#include <android/log.h>
#define LOGE(format, ...)  __android_log_print(ANDROID_LOG_ERROR, "JNI", format, ##__VA_ARGS__)
#define LOGI(format, ...)  __android_log_print(ANDROID_LOG_INFO,  "JNI", format, ##__VA_ARGS__)
#else
#define LOGE(format, ...)  printf("JNI" format "\n", ##__VA_ARGS__)
#define LOGI(format, ...)  printf("JNI" format "\n", ##__VA_ARGS__)
#endif

//Output FFmpeg's av_log()
void custom_log(void *ptr, int level, const char* fmt, va_list vl){
    FILE *fp=fopen("/storage/emulated/0/av_log.txt","a+");
    if(fp){
        vfprintf(fp,fmt,vl);
        fflush(fp);
        fclose(fp);
    }
}

/*
 * Class:     com_lmy_ffmpeg_player_Player
 * Method:    decode
 * Signature: (Ljava/lang/String;Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_com_lmy_ffmpeg_player_Player_decode
  (JNIEnv *env, jobject thiz, jstring in, jstring out){

    char input_str[500] = {0};
    char output_str[500] = {0};
    char info[1000] = {0};
    sprintf(input_str, "%s", (*env)->GetStringUTFChars(env, in, NULL));
    sprintf(output_str, "%s", (*env)->GetStringUTFChars(env, out, NULL));

    AVFormatContext *pFormatCtx;
    int i, videoindex;
    AVCodecContext *pCodecCtx;
    AVCodec *pCodec;
    AVFrame *pFrame, *pFrameYUV;
    uint8_t *out_buffer;
    AVPacket *packet;
    int ret, got_picture;
    struct SwsContext *img_convert_ctx;
    FILE *fp_yuv;
    int frame_cnt;
    clock_t time_start;

    av_log_set_callback(custom_log);

    av_register_all();
    avformat_network_init();
    pFormatCtx = avformat_alloc_context();

    if(avformat_open_input(&pFormatCtx, input_str, NULL, NULL) != 0){
        LOGE("无法打开输入文件\n");
        return;
    }
    if(avformat_find_stream_info(pFormatCtx, NULL) < 0){
        LOGE("无法获取输入流信息\n");
        return;
    }
    videoindex = -1;
    for(i = 0; i<pFormatCtx -> nb_streams; i++){
        if(pFormatCtx -> streams[i] -> codec -> codec_type == AVMEDIA_TYPE_VIDEO){
            videoindex = i;
            break;
        }
    }
    if(videoindex == -1){
        LOGE("无法找到视频流信息\n");
        return;
    }
    pCodecCtx = pFormatCtx ->streams[videoindex] -> codec;
    pCodec = avcodec_find_decoder(pCodecCtx -> codec_id);
    if(pCodec == NULL){
        LOGE("没有合适的解码器\n");
        return;
    }
    if(avcodec_open2(pCodecCtx, pCodec, NULL) < 0){
        LOGE("无法打开解码器\n");
        return;
    }
    pFrame = av_frame_alloc();
    pFrameYUV = av_frame_alloc();
    out_buffer = (unsigned char *)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_YUV420P, pCodecCtx -> width, pCodecCtx -> height, 1));
    av_image_fill_arrays(pFrameYUV -> data, pFrameYUV -> linesize, out_buffer, AV_PIX_FMT_YUV420P, pCodecCtx -> width,pCodecCtx -> height, 1);

    packet = (AVPacket *)av_malloc(sizeof(AVPacket));

    img_convert_ctx = sws_getContext(pCodecCtx -> width, pCodecCtx -> height, pCodecCtx -> pix_fmt,
    pCodecCtx -> width, pCodecCtx -> height,AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);

    sprintf(info, "[Input]%s\n", input_str);
    sprintf(info, "%s[Output]%s\n", info, output_str);
    sprintf(info, "%s[Format]%s\n", info, pFormatCtx -> iformat -> name);
    sprintf(info, "%s[Codec]%s\n", info, pCodecCtx -> codec -> name);
    sprintf(info, "%s[Resolution]%dx%d\n", info, pCodecCtx -> width, pCodecCtx -> height);

    fp_yuv = fopen(output_str, "wb+");
    if(fp_yuv == NULL){
        LOGE("无法打开输出文件\n");
        return;
    }
    frame_cnt = 0;
    time_start = clock();

    while(av_read_frame(pFormatCtx, packet) >= 0){
        if(packet -> stream_index == videoindex){
            ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
            if(ret < 0){
                LOGE("帧解码失败\n");
                return;
            }
            if(got_picture){
                sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame -> data, pFrame -> linesize,
                0, pCodecCtx -> height, pFrameYUV -> data, pFrameYUV -> linesize);
                int y_size = pCodecCtx ->width * pCodecCtx -> height;
                fwrite(pFrameYUV -> data[0], 1, y_size, fp_yuv);
                fwrite(pFrameYUV -> data[1], 1, y_size/4, fp_yuv);
                fwrite(pFrameYUV -> data[2], 1, y_size/4, fp_yuv);
                //print info
                char pict_type[10] = {0};
                switch(pFrame -> pict_type){
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
                LOGI("第%5d帧，类型：%s",frame_cnt, pict_type);
                frame_cnt++;
            }
        }
        av_free_packet(packet);
    }
    //flush decoder
    while(1){
        ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
        if(ret < 0)
            break;
        if(!got_picture)
            break;
        sws_scale(img_convert_ctx, (const uint8_t* const*) pFrame -> data, pFrame -> linesize, 0,
        pCodecCtx -> height, pFrameYUV -> data, pFrameYUV -> linesize);
        int y_size = pCodecCtx -> width * pCodecCtx -> height;
        fwrite(pFrameYUV -> data, 1, y_size, fp_yuv);
        fwrite(pFrameYUV -> data, 1, y_size/4, fp_yuv);
        fwrite(pFrameYUV -> data, 1, y_size/4, fp_yuv);
        //print info
        char pict_type[10] = {0};
        switch(pFrame -> pict_type){
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
        LOGI("第%5d帧，类型：%s",frame_cnt, pict_type);
        frame_cnt++;
    }

    sprintf(info, "%s[Time]%fms\n", info, (double)(clock() - time_start));
    sprintf(info, "%s[Count]%d\n", info,frame_cnt);

    sws_freeContext(img_convert_ctx);
    fclose(fp_yuv);
    av_frame_free(&pFrame);
    av_frame_free(&pFrameYUV);
    avcodec_close(pCodecCtx);
    avformat_close_input(&pFormatCtx);
  }

/*命令行转码
 * Class:     com_lmy_ffmpeg_player_Player
 * Method:    codec
 * Signature: (Ljava/lang/String;Ljava/lang/String;)V
 */
/*JNIEXPORT void JNICALL Java_com_lmy_ffmpeg_player_Player_codec
  (JNIEnv *env, jobject thiz, jint num, jobjectArray cmds){
    av_log_set_callback(custom_log);
    int argc = num;
    char** argv = (char**)malloc(sizeof(char*) * argc);
    int i = 0;
    for(i = 0; i < argc; i++){
        jstring str = (*env) -> GetObjectArrayElement(env, cmds, i);
        const char* tmp = (*env) -> GetStringUTFChars(env, str, NULL);
        argv[i] = (char*)malloc(sizeof(char*) * 1024);
        strcpy(argv[i], tmp);
    }
    ffmpegmain(argc, argv);
    for(i = 0; i < argc; i++){
        free(argv[i]);
    }
    free(argv);
  }*/