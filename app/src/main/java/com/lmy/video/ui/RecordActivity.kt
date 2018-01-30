package com.lmy.video.ui

import android.content.Context
import android.hardware.Camera
import android.util.Log
import android.view.SurfaceHolder
import android.view.SurfaceView
import android.view.View
import com.lmy.ffmpeg.CameraWrapper
import com.lmy.video.R
import kotlinx.android.synthetic.main.activity_record.*

/**
 * Created by limin on 2018/1/29.
 */
class RecordActivity : BaseActivity(), View.OnClickListener {
    override fun getLayoutView(): Int {
        return R.layout.activity_record
    }

    override fun initView() {
        val surfaceView = MySurfaceView(this)
        contentLayout.addView(surfaceView)
    }

    override fun onClick(p0: View?) {

    }

    private class MySurfaceView(context: Context) : SurfaceView(context), SurfaceHolder.Callback,
            Camera.PreviewCallback {

        private var mCameraWrapper: CameraWrapper? = null
        private var count = 0

        init {
            holder.addCallback(this)
        }

        override fun onPreviewFrame(p0: ByteArray?, p1: Camera?) {
            ++count
//            Log.v("000", "onPreviewFrame: $count")
        }

        override fun surfaceChanged(p0: SurfaceHolder?, p1: Int, p2: Int, p3: Int) {
            Log.v("000", "surfaceChanged")
            mCameraWrapper = CameraWrapper.build(750, 480)
            mCameraWrapper?.setPreviewCallback(this)
            if (null != p0)
                mCameraWrapper?.startPreview(p0)
        }

        override fun surfaceDestroyed(p0: SurfaceHolder?) {
            Log.v("000", "surfaceDestroyed")
            mCameraWrapper?.stopCamera()
        }

        override fun surfaceCreated(p0: SurfaceHolder?) {
            Log.v("000", "surfaceCreated")
        }
    }
}