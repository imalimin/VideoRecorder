package com.lmy.video.ui

import android.content.Context
import android.hardware.Camera
import android.util.Log
import android.view.MotionEvent
import android.view.SurfaceHolder
import android.view.SurfaceView
import android.view.View
import com.lmy.ffmpeg.CameraWrapper
import com.lmy.ffmpeg.codec.VideoEncoder
import com.lmy.video.R
import kotlinx.android.synthetic.main.activity_record.*

/**
 * Created by limin on 2018/1/29.
 */
class RecordActivity : BaseActivity(), View.OnClickListener {
    private var mSurfaceView: MySurfaceView? = null
    override fun getLayoutView(): Int {
        return R.layout.activity_record
    }

    override fun initView() {
        mSurfaceView = MySurfaceView(this)
        contentLayout.addView(mSurfaceView)
        recordBtn.setOnTouchListener { _, motionEvent ->
            when (motionEvent?.action) {
                MotionEvent.ACTION_DOWN -> mSurfaceView?.start()
                MotionEvent.ACTION_UP -> mSurfaceView?.pause()
            }
            true
        }
    }

    override fun onClick(p0: View?) {

    }

    private class MySurfaceView(context: Context) : SurfaceView(context), SurfaceHolder.Callback,
            Camera.PreviewCallback {

        private val mVideoWidth = 720
        private val mVideoHeight = 480
        private var mCameraWrapper: CameraWrapper? = null
        private var mVideoEncoder: VideoEncoder? = null
        private var count = 0
        private var start = false

        init {
            holder.addCallback(this)
        }

        override fun onPreviewFrame(data: ByteArray?, p1: Camera?) {
            if (!start) return
            ++count
            mVideoEncoder?.encode2(data)
            Log.v("000", "onPreviewFrame: ${data?.size}")
        }

        override fun surfaceChanged(p0: SurfaceHolder?, p1: Int, p2: Int, p3: Int) {
            Log.v("000", "surfaceChanged")
            mCameraWrapper?.stopPreview()
            if (null != p0)
                mCameraWrapper?.startPreview(p0)
        }

        override fun surfaceDestroyed(p0: SurfaceHolder?) {
            Log.v("000", "surfaceDestroyed")
            mCameraWrapper?.stopCamera()
            mVideoEncoder?.flush()
        }

        override fun surfaceCreated(p0: SurfaceHolder?) {
            Log.v("000", "surfaceCreated")
            mCameraWrapper = CameraWrapper.build(mVideoHeight, mVideoWidth)
            mVideoEncoder = VideoEncoder.build("/storage/emulated/0/out.mp4",
                    mCameraWrapper!!.getCameraSize().width, mCameraWrapper!!.getCameraSize().height,
                    mVideoWidth, mVideoHeight)
            mCameraWrapper?.setPreviewCallback(this)
        }

        fun start() {
            start = true
        }

        fun pause() {
            start = false
        }
    }
}