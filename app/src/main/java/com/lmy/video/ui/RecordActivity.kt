package com.lmy.video.ui

import android.view.MotionEvent
import android.view.View
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
        mRecordView.setVideoInfo("/storage/emulated/0/out.mp4", 720, 480)
        recordBtn.setOnTouchListener { _, motionEvent ->
            when (motionEvent?.action) {
                MotionEvent.ACTION_DOWN -> mRecordView.start()
                MotionEvent.ACTION_UP -> mRecordView.pause()
            }
            true
        }
    }

    override fun onClick(p0: View?) {

    }
}