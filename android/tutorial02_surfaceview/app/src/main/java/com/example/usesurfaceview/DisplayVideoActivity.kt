package com.example.usesurfaceview

import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle

class DisplayVideoActivity : AppCompatActivity() {
    private lateinit var mSurfaceView: MySurfaceView
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_display_video)

        mSurfaceView = findViewById(R.id.surfaceView_display_video)

    }
}