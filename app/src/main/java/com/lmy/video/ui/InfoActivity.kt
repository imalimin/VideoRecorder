package com.lmy.video.ui


import com.lmy.ffmpeg.Version
import com.lmy.video.R
import kotlinx.android.synthetic.main.activity_info.*

/**
 * Created by lmy on 2017/4/20.
 */

class InfoActivity : BaseActivity() {

    override fun getLayoutView(): Int {
        return R.layout.activity_info
    }

    override fun initView() {
        val version = Version()
        var str = ""
        //        str = String.format("Version:\n%s\n", version.version());
        str += String.format("Protocol:\n%s\n", version.protocol())
        str += String.format("Format:\n%s\n", version.format())
        str += String.format("Codec:\n%s\n", version.codec())
        //        str += String.format("Filter:\n%s\n", version.filter());
        str += String.format("Configure:\n%s\n", version.configure())
        infoView.text = str
    }
}
