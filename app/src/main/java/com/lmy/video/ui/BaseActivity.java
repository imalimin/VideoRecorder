package com.lmy.video.ui;

import android.os.Build;
import android.os.Bundle;
import android.support.v4.app.ActivityCompat;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.util.TypedValue;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;

import com.lmy.video.R;

/**
 * Created by 李明艺 on 2016/3/12.
 *
 * @author lrlmy@foxmail.com
 */
public abstract class BaseActivity extends AppCompatActivity {
    protected abstract int getLayoutView();

    protected abstract void initView();

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(getLayoutView());
        //控件注解框架注入
//        ButterKnife.bind(this);
        initView();
    }

    public void fullLayout() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            getWindow().getDecorView().setSystemUiVisibility(
                    View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                            | View.SYSTEM_UI_FLAG_LAYOUT_STABLE
            );
        }
    }

    public void initToolbar(Toolbar toolbar, int tile, int navIcon) {
        initToolbar(toolbar, getResources().getString(tile), navIcon);
    }

    public void initToolbar(Toolbar toolbar, int tile) {
        if (tile == 0)
            initToolbar(toolbar, "");
        else
            initToolbar(toolbar, getResources().getString(tile));
    }

    public void initToolbar(Toolbar toolbar, String tile) {
        initToolbar(toolbar, tile, R.mipmap.ic_launcher);
    }

    public void initToolbar(Toolbar toolbar, String tile, int navIcon) {
        toolbar.setTitle(tile);
        setSupportActionBar(toolbar);
        toolbar.setNavigationIcon(navIcon);
        toolbar.setNavigationOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                ActivityCompat.finishAfterTransition(BaseActivity.this);
            }
        });
    }

    protected void setToolbarTitle(String title) {
        getSupportActionBar().setTitle(title);
    }

    protected int getBaseColor(int res) {
        if (res <= 0)
            throw new IllegalArgumentException("resource id can not be less 0");
        return getResources().getColor(res);
    }

    public int getStatusBarColor() {
        return getColorPrimary();
    }

    public void setStatusBarColor(int colorRes) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            Window window = getWindow();
            window.clearFlags(WindowManager.LayoutParams.FLAG_TRANSLUCENT_STATUS);
            window.setStatusBarColor(getResources().getColor(colorRes));
        }
    }

    public int getColorPrimary() {
        TypedValue typedValue = new TypedValue();
        getTheme().resolveAttribute(R.attr.colorPrimaryDark, typedValue, true);
        return typedValue.data;
    }

    public float getDimenPixelSize(int id) {
        return getResources().getDimensionPixelSize(id);
    }
}
