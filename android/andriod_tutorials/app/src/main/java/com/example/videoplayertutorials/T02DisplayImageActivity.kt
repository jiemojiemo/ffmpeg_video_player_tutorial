package com.example.videoplayertutorials

import android.graphics.Bitmap
import android.graphics.BitmapFactory
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.view.Surface
import android.view.SurfaceHolder

class T02DisplayImageActivity : AppCompatActivity() {
    private lateinit var mSurfaceView: MySurfaceView

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_display_image)

        // get image bitmap
        val options = BitmapFactory.Options()
        options.inScaled = false
        val bitmap = BitmapFactory.decodeResource(resources, R.drawable.test, options)

        mSurfaceView = findViewById(R.id.surfaceView_display_image)
        mSurfaceView.setAspectRation(bitmap.width, bitmap.height)

        mSurfaceView.holder.addCallback(object: SurfaceHolder.Callback{
            override fun surfaceCreated(holder: SurfaceHolder) {
                val surface = holder.surface
                renderImage(surface, bitmap)
            }

            override fun surfaceChanged(p0: SurfaceHolder, format: Int, width: Int, height: Int) {
            }

            override fun surfaceDestroyed(p0: SurfaceHolder) {
            }

        })
    }

    external fun renderImage(surface: Surface, bitmap: Bitmap);
}