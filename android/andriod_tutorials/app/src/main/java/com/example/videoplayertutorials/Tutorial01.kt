package com.example.videoplayertutorials

import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.widget.TextView

class Tutorial01 : AppCompatActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_tutorial01)

        val textView = findViewById<TextView>(R.id.sample_text)
        textView.text = stringFromFFMPEG()
    }
    
    private external fun stringFromFFMPEG(): String
}