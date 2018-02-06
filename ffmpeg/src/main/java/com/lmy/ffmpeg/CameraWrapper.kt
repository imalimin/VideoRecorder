package com.lmy.ffmpeg

import android.graphics.PixelFormat
import android.graphics.Rect
import android.graphics.SurfaceTexture
import android.hardware.Camera
import android.util.Log
import android.view.SurfaceHolder
import java.io.IOException
import java.util.*


/**
 * Created by limin on 2018/1/29.
 */
class CameraWrapper private constructor(expectWidth: Int, expectHeight: Int, facing: Int) {
    companion object {
        private val TAG = "CameraWrapper"
        private val RATE = 24
        fun build(expectWidth: Int, expectHeight: Int): CameraWrapper {
            return CameraWrapper(expectWidth, expectHeight, Camera.CameraInfo.CAMERA_FACING_BACK)
        }
    }

    private var mCamera: Camera? = null
    private var mExpectWidth: Int = expectWidth
    private var mExpectHeight: Int = expectHeight
    private var mCameraSize: Camera.Size? = null
    private var mFacing: Int = facing
    private var mDefaultCameraID: Int = 0
    private var previewing = false

    init {
        val cameras = Camera.getNumberOfCameras()
        val cameraInfo = Camera.CameraInfo()
        for (i in 0 until cameras) {
            Camera.getCameraInfo(i, cameraInfo)
            if (cameraInfo.facing == facing) {
                mDefaultCameraID = i
            }
        }
        stopPreview()//初始化之前暂停预览
        if (mCamera != null) {//如果摄像头不为空则先释放摄像头
            mCamera?.release()
            mCamera = null
        }
        try {

            if (mDefaultCameraID >= 0)
                mCamera = Camera.open(mDefaultCameraID)
            else
                mCamera = Camera.open()
        } catch (e: Exception) {
            e.printStackTrace()
            mCamera = null
        }

        if (mCamera != null) {
            try {
                initCamera()
            } catch (e: Exception) {
                e.printStackTrace()
                mCamera?.release()
                mCamera = null
            }
        }
    }

    private fun initCamera() {
        if (null == mCamera) return
        val params = mCamera!!.parameters
        setRate(params)
        setSize(params)
        params.focusMode = Camera.Parameters.FOCUS_MODE_AUTO
        Log.v(TAG, "format: ${params.previewFormat}, ${PixelFormat.YCbCr_420_SP}")
        mCamera!!.parameters = params
        mCamera!!.setDisplayOrientation(90)
    }

    //设置相机预览帧率
    private fun setRate(params: Camera.Parameters) {
        val frameRatesRang = IntArray(2)
        params.getPreviewFpsRange(frameRatesRang)
        var rate = RATE * 1000
        if (rate < frameRatesRang[0])
            rate = frameRatesRang[0]
        if (rate > frameRatesRang[1])
            rate = frameRatesRang[1]
        params.setPreviewFpsRange(rate, rate)
        Log.v(TAG, "rate: $rate, ${frameRatesRang[0]}, ${frameRatesRang[1]}")
    }

    //设置最佳分辨率，大于或等于
    private fun setSize(params: Camera.Parameters) {
        mCameraSize = getBestSize(params.supportedPreviewSizes)
        Log.v(TAG, "${mCameraSize?.width}, ${mCameraSize?.height}")
        if (null != mCameraSize) {
            params.setPictureSize(mCameraSize!!.width, mCameraSize!!.height)
            params.setPreviewSize(mCameraSize!!.width, mCameraSize!!.height)
        }
    }

    private fun getBestSize(sizes: List<Camera.Size>): Camera.Size? {
        sizes.forEach {
            Log.v(TAG, "${it?.width}, ${it?.height}")
        }
        Collections.sort(sizes, { lhs, rhs ->
            val w = lhs.height - rhs.height
            if (w == 0) lhs.width - rhs.width else w
        })

        return sizes.firstOrNull { it.width >= mExpectWidth && it.height >= mExpectHeight }
    }

    fun setPreviewCallback(callback: Camera.PreviewCallback) {
        if (previewing) throw  RuntimeException("Must call before startPreview")
//        mCamera!!.addCallbackBuffer(mBuffer)
        mCamera!!.setPreviewCallback(callback)
    }

    @Synchronized
    fun startPreview(texture: SurfaceTexture) {
        Log.i(TAG, "Camera startPreview...")
        if (previewing) {
            Log.e(TAG, "Err: camera is previewing...")
            return
        }

        if (mCamera != null) {
            try {
                mCamera?.setPreviewTexture(texture)
            } catch (e: IOException) {
                e.printStackTrace()
            }

            mCamera?.startPreview()
            previewing = true
        }
    }

    @Synchronized
    fun startPreview(holder: SurfaceHolder) {
        Log.i(TAG, "Camera startPreview...")
        if (previewing) {
            Log.e(TAG, "Err: camera is previewing...")
            return
        }

        if (mCamera != null) {
            try {
                mCamera?.setPreviewDisplay(holder)
            } catch (e: IOException) {
                e.printStackTrace()
            }

            mCamera?.startPreview()
            previewing = true
        }
    }

    @Synchronized
    fun stopPreview() {
        if (previewing && mCamera != null) {
            previewing = false
            mCamera?.stopPreview()
        }
    }

    @Synchronized
    fun stopCamera() {
        if (mCamera != null) {
            stopPreview()
            previewing = false
            mCamera?.setPreviewCallback(null)
            mCamera?.release()
            mCamera = null
        }
    }

    fun getCameraSize(): Camera.Size {
        return mCameraSize!!
    }

    fun focus(x: Float, y: Float, callback: Camera.AutoFocusCallback) {
        focusAtPoint(x, y, 0.2f, callback)
    }

    @Synchronized
    private fun focusAtPoint(x: Float, y: Float, radius: Float, callback: Camera.AutoFocusCallback) {
        if (mCamera == null) {
            Log.e(TAG, "Error: focus after release.")
            return
        }
        val params = mCamera!!.parameters
        if (params.maxNumMeteringAreas > 0) {

            val focusRadius = (radius * 1000.0f).toInt()
            val left = (x * 2000.0f - 1000.0f).toInt() - focusRadius
            val top = (y * 2000.0f - 1000.0f).toInt() - focusRadius

            val focusArea = Rect()
            focusArea.left = Math.max(left, -1000)
            focusArea.top = Math.max(top, -1000)
            focusArea.right = Math.min(left + focusRadius, 1000)
            focusArea.bottom = Math.min(top + focusRadius, 1000)
            val meteringAreas = ArrayList<Camera.Area>()
            meteringAreas.add(Camera.Area(focusArea, 800))

            try {
                mCamera!!.cancelAutoFocus()
                params.focusMode = Camera.Parameters.FOCUS_MODE_AUTO
                params.focusAreas = meteringAreas
                mCamera!!.parameters = params
                mCamera!!.autoFocus(callback)
            } catch (e: Exception) {
                Log.e(TAG, "Error: focusAtPoint failed: " + e.toString())
            }

        } else {
            Log.i(TAG, "The device does not support metering areas...")
            try {
                mCamera!!.autoFocus(callback)
            } catch (e: Exception) {
                Log.e(TAG, "Error: focusAtPoint failed: " + e.toString())
            }

        }
    }
}