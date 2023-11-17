package com.example.usesurfaceview

import android.content.Intent
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.widget.Button
import androidx.core.app.ActivityCompat

class MainActivity : AppCompatActivity() {

    companion object {
        init {
            System.loadLibrary("usesurfaceview")
        }
    }
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        requestPermissions()

        val displayImageButton = findViewById<Button>(R.id.go_display_image)
        displayImageButton.setOnClickListener {
            val intent = Intent(this, DisplayImageActivity::class.java)
            startActivity(intent)
        }

        val displayVideoButoon = findViewById<Button>(R.id.go_display_video)
        displayVideoButoon.setOnClickListener {
            val intent = Intent(this, DisplayVideoActivity::class.java)
            startActivity(intent)
        }

    }

    private fun requestPermissions() {
        ActivityCompat.requestPermissions(
            this@MainActivity, arrayOf(
                android.Manifest.permission.WRITE_EXTERNAL_STORAGE, android.Manifest.permission.RECORD_AUDIO,
                android.Manifest.permission.CAMERA, android.Manifest.permission.READ_EXTERNAL_STORAGE
            ),
            1
        )
    }
}