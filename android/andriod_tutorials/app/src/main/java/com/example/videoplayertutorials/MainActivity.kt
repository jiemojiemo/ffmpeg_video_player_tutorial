package com.example.videoplayertutorials

import android.content.Intent
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.widget.Button
import androidx.core.app.ActivityCompat

class MainActivity : AppCompatActivity() {
    companion object{
        init {
            System.loadLibrary("native_video_player")
        }
    }
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        requestPermissions()

        val tutorial01Btn = findViewById<Button>(R.id.tutorial01)
        tutorial01Btn.setOnClickListener {
            val intent = Intent(this, Tutorial01::class.java)
            startActivity(intent)
        }

        val tutorial02Btn = findViewById<Button>(R.id.tutorial02)
        tutorial02Btn.setOnClickListener {
            val intent = Intent(this, Tutorial02::class.java)
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