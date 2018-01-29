package com.lmy.video.widget;

import android.content.Context;
import android.util.AttributeSet;

import com.lmy.video.gl.ICanvasGL;
import com.lmy.video.gl.view.texture.GLContinuousTextureView;

/**
 * Created by lmy on 2017/5/4.
 */

public class GLVideoView extends GLContinuousTextureView {
    public GLVideoView(Context context) {
        super(context);
    }

    public GLVideoView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public GLVideoView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
    }

    @Override
    protected void onGLDraw(ICanvasGL canvas) {

    }
}
