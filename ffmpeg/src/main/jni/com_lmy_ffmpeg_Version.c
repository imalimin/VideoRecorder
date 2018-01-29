//
// Created by lmy on 2017/4/20.
//
#include <com_lmy_ffmpeg_Version.h>
#include <string.h>
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavfilter/avfilter.h"
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
/* ffmpeg版本信息
 * Class:     com_lmy_ffmpeg_Version
 * Method:    version
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_lmy_ffmpeg_Version_version
  (JNIEnv *env, jobject thiz){
    char info[10000] = { 0 };
    sprintf(info, "%s\n", avcodec_version());
    return (*env)->NewStringUTF(env, info);
  }

/* ffmpeg支持的协议
 * Class:     com_lmy_ffmpeg_Version
 * Method:    protocol
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_lmy_ffmpeg_Version_protocol
  (JNIEnv *env, jobject thiz){
    char info[40000]={0};
    av_register_all();

    struct URLProtocol *pup = NULL;
    //Input
    struct URLProtocol **p_temp = &pup;
    avio_enum_protocols((void **)p_temp, 0);
    while ((*p_temp) != NULL){
        sprintf(info, "%s[In ][%10s]\n", info, avio_enum_protocols((void **)p_temp, 0));
    }
    pup = NULL;
    //Output
    avio_enum_protocols((void **)p_temp, 1);
    while ((*p_temp) != NULL){
        sprintf(info, "%s[Out][%10s]\n", info, avio_enum_protocols((void **)p_temp, 1));
    }

    return (*env)->NewStringUTF(env, info);
  }

/* ffmpeg支持的封装格式
 * Class:     com_lmy_ffmpeg_Version
 * Method:    format
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_lmy_ffmpeg_Version_format
  (JNIEnv *env, jobject thiz){
    char info[40000] = { 0 };

    av_register_all();

    AVInputFormat *if_temp = av_iformat_next(NULL);
    AVOutputFormat *of_temp = av_oformat_next(NULL);
    //Input
    while(if_temp!=NULL){
        sprintf(info, "%s[In ][%10s]\n", info, if_temp->name);
        if_temp=if_temp->next;
    }
    //Output
    while (of_temp != NULL){
        sprintf(info, "%s[Out][%10s]\n", info, of_temp->name);
        of_temp = of_temp->next;
    }
    return (*env)->NewStringUTF(env, info);
  }

/* ffmpeg支持的编解码器
 * Class:     com_lmy_ffmpeg_Version
 * Method:    codec
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_lmy_ffmpeg_Version_codec
  (JNIEnv *env, jobject thiz){
    char info[40000] = { 0 };
    av_register_all();

    AVCodec *c_temp = av_codec_next(NULL);

    while(c_temp!=NULL){
        if (c_temp->decode!=NULL){
            sprintf(info, "%s[Dec]", info);
        }
        else{
            sprintf(info, "%s[Enc]", info);
        }
        switch (c_temp->type){
        case AVMEDIA_TYPE_VIDEO:
            sprintf(info, "%s[Video]", info);
            break;
        case AVMEDIA_TYPE_AUDIO:
            sprintf(info, "%s[Audio]", info);
            break;
        default:
            sprintf(info, "%s[Other]", info);
            break;
        }
        sprintf(info, "%s[%10s]\n", info, c_temp->name);

        c_temp=c_temp->next;
    }

    return (*env)->NewStringUTF(env, info);
  }

/* ffmpeg类库支持的滤镜
 * Class:     com_lmy_ffmpeg_Version
 * Method:    filter
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_lmy_ffmpeg_Version_filter
  (JNIEnv *env, jobject thiz){
    char info[40000] = { 0 };
    avfilter_register_all();
    AVFilter *f_temp = (AVFilter *)avfilter_next(NULL);
    while (f_temp != NULL){
        sprintf(info, "%s[%10s]\n", info, f_temp->name);
    }

    return (*env)->NewStringUTF(env, info);
  }

/* ffmpeg的配置信息
 * Class:     com_lmy_ffmpeg_Version
 * Method:    configure
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_lmy_ffmpeg_Version_configure
  (JNIEnv *env, jobject thiz){
    char info[10000] = { 0 };
    av_register_all();

    sprintf(info, "%s\n", avcodec_configuration());

    return (*env)->NewStringUTF(env, info);
  }
