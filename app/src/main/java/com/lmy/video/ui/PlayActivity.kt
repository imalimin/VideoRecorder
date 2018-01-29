package com.lmy.video.ui

import android.view.View
import com.lmy.ffmpeg.codec.MediaDecoder
import com.lmy.video.R
import kotlinx.android.synthetic.main.activity_play.*

/**
 * Created by limin on 2018/1/29.
 */
class PlayActivity : BaseActivity(), View.OnClickListener {
    private val mDecoder: MediaDecoder? = null

    override fun getLayoutView(): Int {
        return R.layout.activity_play
    }

    override fun initView() {
        enterBtn.setOnClickListener(this)
        videoView.setDataSource("/storage/emulated/0/清湾延时-1080.mp4")
    }

    override fun onClick(view: View) {
        when (view.id) {
            R.id.enterBtn -> videoView.start()
        }//                new AsyncTask<Void, Void, YuvImage>() {
        //                    @Override
        //                    protected YuvImage doInBackground(Void... voids) {
        //                        long time = System.currentTimeMillis();
        //                        if (mDecoder == null) {
        //                            mDecoder = new MediaDecoder();
        //                            mDecoder.setDataSource("/storage/emulated/0/test.mp4");
        //                        }
        //                        mDecoder.nextFrame();
        //                        Log.v(TAG, String.format("decode: %d", System.currentTimeMillis() - time));
        //                        if (mDecoder.getFrame().getData() == null || mDecoder.getFrame().getData().length == 0)
        //                            return null;
        //                        YuvImage image = new YuvImage(mDecoder.getFrame().getData(), ImageFormat.NV21,
        //                                mDecoder.getFrame().getWidth(), mDecoder.getFrame().getHeight(), null);
        //                        Log.v(TAG, String.format("to YuvImage: %d", System.currentTimeMillis() - time));
        ////                        MediaPlayer mediaPlayer;
        ////                        mediaPlayer.setDataSource();
        //                        try {
        //                            image.compressToJpeg(new Rect(0, 0, mDecoder.getFrame().getWidth(), mDecoder.getFrame().getHeight()),
        //                                    80, new FileOutputStream("/storage/emulated/0/aaaaaaaa.jpg"));
        //                        } catch (FileNotFoundException e) {
        //                            e.printStackTrace();
        //                        }
        //                        return image;
        //                    }
        //
        //                    @Override
        //                    protected void onPostExecute(YuvImage yuvImage) {
        //                        super.onPostExecute(yuvImage);
        //                    }
        //                }.execute();
    }

    override fun onDestroy() {
        super.onDestroy()
        videoView.release()
        //        mDecoder.release();
    }

    companion object {
        private val TAG = "MainActivity"
    }
}