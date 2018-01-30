package com.lmy.video.ui

import android.content.Intent
import android.view.View
import com.lmy.video.R
import kotlinx.android.synthetic.main.activity_main.*

class MainActivity : BaseActivity(), View.OnClickListener {

    override fun getLayoutView(): Int {
        return R.layout.activity_main
    }

    override fun initView() {
        infoBtn.setOnClickListener(this)
        playBtn.setOnClickListener(this)
        recordBtn.setOnClickListener(this)
    }

    override fun onClick(view: View) {
        when (view.id) {
            R.id.infoBtn -> startActivity(Intent(this, InfoActivity::class.java))
            R.id.playBtn -> startActivity(Intent(this, PlayActivity::class.java))
            R.id.recordBtn -> startActivity(Intent(this, RecordActivity::class.java))
        }
    }

    companion object {
        private val TAG = "MainActivity"
    }
}
