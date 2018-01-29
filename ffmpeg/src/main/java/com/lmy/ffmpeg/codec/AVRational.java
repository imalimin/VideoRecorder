package com.lmy.ffmpeg.codec;

/**
 * Created by lmy on 2017/4/20.
 */

public class AVRational {
    private int num; ///< numerator
    private int den; ///< denominator

    public AVRational() {
    }

    public AVRational(int num, int den) {
        this.num = num;
        this.den = den;
    }

    public int getNum() {
        return num;
    }

    public void setNum(int num) {
        this.num = num;
    }

    public int getDen() {
        return den;
    }

    public void setDen(int den) {
        this.den = den;
    }
}
