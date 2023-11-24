package com.example.videoplayertutorials

import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.util.Log
import android.view.SurfaceHolder
import android.widget.Button
import android.widget.SeekBar
import java.io.File

class DisplayVideoActivity : AppCompatActivity() {
    private lateinit var mSurfaceView: MySurfaceView
    private var mPlayer = SimplePlayer()
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_display_video)

        val videoPath = File(getExternalFilesDir(null), "perf1080P.mp4")

        mSurfaceView = findViewById(R.id.surfaceView_display_video)
        mSurfaceView.holder.addCallback(object : SurfaceHolder.Callback {
            override fun surfaceCreated(holder: SurfaceHolder) {
                mPlayer.attachSurface(mSurfaceView.holder.surface)
            }

            override fun surfaceChanged(p0: SurfaceHolder, format: Int, width: Int, height: Int) {
                Log.d(
                    "DisplayVideoActivity",
                    "surfaceChanged: format: $format, width: $width, height: $height"
                )
            }

            override fun surfaceDestroyed(p0: SurfaceHolder) {
            }

        })


        val playButton = findViewById<Button>(R.id.btn_play)
        playButton.setOnClickListener {
            if(mPlayer.isStopped()){
                val ret = mPlayer.open(videoPath.absolutePath)
                if(!ret){
                    Log.e("DisplayVideoActivity", "open video failed")
                    return@setOnClickListener
                }
            }

            if(!mPlayer.isPrepared()){
                val width = mPlayer.getMediaFileWidth()
                val height = mPlayer.getMediaFileHeight()
                mSurfaceView.setAspectRation(width, height)

                mPlayer.attachSurface(mSurfaceView.holder.surface)
                mPlayer.prepareForOutput()
            }

            mPlayer.play()
        }

        val stopButton = findViewById<Button>(R.id.btn_stop)
        stopButton.setOnClickListener {
            mPlayer.stop()
        }

        val pauseButton = findViewById<Button>(R.id.btn_pause)
        pauseButton.setOnClickListener {
            mPlayer.pause()
        }

        val seekBar = findViewById<SeekBar>(R.id.seek_bar)
        seekBar.setOnSeekBarChangeListener(object : SeekBar.OnSeekBarChangeListener{
            override fun onProgressChanged(p0: SeekBar?, progress: Int, fromUser: Boolean) {
                val p = progress.toDouble() / 100
                Log.e("DisplayVideoActivity", "progress: $p")
                if(mPlayer.isStopped()){
                    val ret = mPlayer.open(videoPath.absolutePath)
                    if(!ret){
                        Log.e("DisplayVideoActivity", "open video failed")
                        return
                    }
                }
                mPlayer.seek(p)
            }

            override fun onStartTrackingTouch(p0: SeekBar?) {
                mPlayer.pause()
            }

            override fun onStopTrackingTouch(p0: SeekBar?) {
            }

        })
    }

    override fun onPause() {
        super.onPause()
        mPlayer.stop()
    }

    override fun onDestroy() {
        super.onDestroy()
        mPlayer.stop()
        mPlayer.destroy()
    }
}