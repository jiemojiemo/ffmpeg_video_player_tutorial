package com.example.videoplayertutorials

import android.content.Intent
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.widget.Button

class Tutorial02 : AppCompatActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_tutorial02)

        val displayImageButton = findViewById<Button>(R.id.go_display_image)
        displayImageButton.setOnClickListener {
            val intent = Intent(this, T02DisplayImageActivity::class.java)
            startActivity(intent)
        }

        val displayVideoButoon = findViewById<Button>(R.id.go_display_video)
        displayVideoButoon.setOnClickListener {
            val intent = Intent(this, DisplayVideoActivity::class.java)
            startActivity(intent)
        }
    }
}