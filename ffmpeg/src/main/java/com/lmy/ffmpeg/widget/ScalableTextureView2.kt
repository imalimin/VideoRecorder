package com.lmy.ffmpeg.widget

import android.content.Context
import android.graphics.Matrix
import android.util.AttributeSet
import android.util.Log
import android.view.TextureView
import android.widget.ImageView

/**
 * Created by limin on 2018/2/6.
 */
open class ScalableTextureView2 : TextureView {
    companion object {
        private val TAG = "ScalableTextureView2"
    }

    private var mScaleType: ImageView.ScaleType? = ImageView.ScaleType.CENTER
    private val mTransformMatrix = Matrix()

    constructor(context: Context) : super(context) {
        apply(null, 0)
    }

    constructor(context: Context, attrs: AttributeSet) : super(context, attrs) {
        apply(attrs, 0)
    }

    constructor(context: Context, attrs: AttributeSet?, defStyleAttr: Int) : super(context, attrs, defStyleAttr) {
        apply(attrs, 0)
    }

    private fun apply(attrs: AttributeSet?, defStyleAttr: Int) {

    }

    override fun onMeasure(widthMeasureSpec: Int, heightMeasureSpec: Int) {
        super.onMeasure(widthMeasureSpec, heightMeasureSpec)
        Log.v(TAG, "onMeasure")
    }

    fun updateViewSize(scaleType: ImageView.ScaleType, srcAspect: Float,
                       width: Int, height: Int) {
        Log.v(TAG, "updateViewSize")
        post { updateViewSizeProxy(scaleType, srcAspect, width, height) }
    }

    private fun updateViewSizeProxy(scaleType: ImageView.ScaleType, srcAspect: Float,
                                    width: Int, height: Int) {
        layout(left, top, right, top + (right - left) * height / width)
        mScaleType = scaleType
        mTransformMatrix.reset()
        var scaleX = 1f
        var scaleY = 1f
        when (scaleType) {
            ImageView.ScaleType.CENTER -> {
                scaleX = 1f
                scaleY = getWidth() / srcAspect / getHeight().toFloat()
            }
            else -> {
            }
        }
        mTransformMatrix.setScale(scaleX, scaleY, getWidth() / 2f, getHeight() / 2f)
//        mTransformMatrix.setRectToRect(RectF(0f, 0f, width.toFloat(), height.toFloat()),
//                RectF(0f, 0f, getWidth().toFloat(), getHeight().toFloat()),
//                Matrix.ScaleToFit.FILL)
        setTransform(mTransformMatrix)
    }
}
