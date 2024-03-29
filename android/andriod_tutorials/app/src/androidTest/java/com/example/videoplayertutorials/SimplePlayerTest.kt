package com.example.videoplayertutorials

import android.util.Log
import androidx.test.platform.app.InstrumentationRegistry
import androidx.test.ext.junit.runners.AndroidJUnit4

import org.junit.Test
import org.junit.runner.RunWith

import org.junit.Assert.*
import org.junit.Before

/**
 * Instrumented test, which will execute on an Android device.
 *
 * See [testing documentation](http://d.android.com/tools/testing).
 */
@RunWith(AndroidJUnit4::class)
class SimplePlayerTest {
    private var videoPath = ""
    @Before
    fun setup(){
        val appContext = InstrumentationRegistry.getInstrumentation().targetContext
        videoPath = appContext.getExternalFilesDir(null)?.absolutePath + "/perf1080P.mp4"
    }
    @Test
    fun isValidAfterInit() {
        val player = SimplePlayer()
        assertTrue(player.isValid())
    }

    @Test
    fun isNotValidAfterDestroy(){
        val player = SimplePlayer()
        player.destroy()
        assertFalse(player.isValid())
    }

    @Test
    fun canOpenVideo(){
        val player = SimplePlayer()
        assertTrue(player.open(videoPath))
    }

    @Test
    fun canGetMediaFileWidthAfterOpen(){
        val player = SimplePlayer()
        val ok = player.open(videoPath)

        assertTrue(ok)
        assertTrue(player.getMediaFileWidth() > 0)
    }

    @Test
    fun canGetMediaFileHeightAfterOpen(){
        val player = SimplePlayer()
        val ok = player.open(videoPath)

        assertTrue(ok)
        assertTrue(player.getMediaFileHeight() > 0)
    }
}