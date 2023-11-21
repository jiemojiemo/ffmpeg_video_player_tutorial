package com.example.videoplayertutorials

import android.content.Context
import android.util.AttributeSet
import android.view.SurfaceView

class MySurfaceView(context: Context?, attrs: AttributeSet?) : SurfaceView(context, attrs) {
    private var mWidth = 0
    private var mHeight = 0

    constructor(context: Context?) : this(context, null)

    override fun onMeasure(widthMeasureSpec: Int, heightMeasureSpec: Int) {
        super.onMeasure(widthMeasureSpec, heightMeasureSpec)
        val width = MeasureSpec.getSize(widthMeasureSpec)
        val height = MeasureSpec.getSize(heightMeasureSpec)
        if(mWidth == 0){
            setMeasuredDimension(width, height)
            return
        }

        // calculate expected width by ratio
        val expectedWidth = height * mWidth / mHeight

        // if expected width is too big, set max width to expected width
        if(expectedWidth >= width){
            // to maintain aspect ratio, calculate expected height
            val expectedHeight = width * mHeight / mWidth
            setMeasuredDimension(width, expectedHeight)
        }else{
            // or the expected width can fit in the parent, set the expected width
            setMeasuredDimension(expectedWidth, height)
        }
    }

    fun setAspectRation(width: Int, height: Int) {
        if (width < 0 || height < 0) throw IllegalArgumentException("width and height must be positive")
        mWidth = width
        mHeight = height
        requestLayout()
    }
}