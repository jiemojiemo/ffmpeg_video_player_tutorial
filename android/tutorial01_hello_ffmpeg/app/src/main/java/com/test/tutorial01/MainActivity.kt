package com.test.tutorial01

import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import com.test.tutorial01.databinding.ActivityMainBinding

class MainActivity : AppCompatActivity() {

    private lateinit var binding: ActivityMainBinding

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        // Example of a call to a native method
        binding.sampleText.text = stringFromFFMPEG()
    }

    /**
     * A native method that is implemented by the 'tutorial01' native library,
     * which is packaged with this application.
     */
    external fun stringFromFFMPEG(): String

    companion object {
        // Used to load the 'tutorial01' library on application startup.
        init {
            System.loadLibrary("tutorial01")
        }
    }
}