package com.example.usesurfaceview

import android.view.Surface

class SimplePlayer {
    private var _handel: Long = nativeCreatePlayer()

    companion object{
        init {
            System.loadLibrary("usesurfaceview")
        }
    }
    fun isValid(): Boolean {
        return _handel != 0L
    }

    fun destroy() {
        nativeDestroy(_handel)
        _handel = 0
    }


    fun open(videoPath: String): Boolean {
        return nativeOpen(_handel, videoPath)
    }

    fun getMediaFileWidth(): Int {
        return nativeGetMediaFileWidth(_handel)
    }

    fun getMediaFileHeight(): Int {
        return nativeGetMediaFileHeight(_handel)
    }

    fun attachSurface(surface: Surface) {
        return nativeAttachSurface(_handel, surface)
    }

    fun prepare(): Int{
        return nativePrepare(_handel)
    }

    fun play(): Int{
        return nativePlay(_handel)
    }

    fun stop(): Int{
        return nativeStop(_handel)
    }

    fun seek(progress: Double):Int{
        return nativeSeek(_handel, progress)
    }

    fun pause(): Int{
        return nativePause(_handel)
    }


    private external fun nativeCreatePlayer(): Long
    private external fun nativeDestroy(_handel: Long)
    private external fun nativeOpen(player: Long, videoPath: String): Boolean
    private external fun nativeGetMediaFileWidth(player: Long): Int
    private external fun nativeGetMediaFileHeight(player: Long): Int
    private external fun nativeAttachSurface(_handel: Long, surface: Surface)
    private external fun nativePrepare(_handel: Long): Int
    private external fun nativePlay(_handel: Long): Int
    private external fun nativeStop(_handel: Long): Int
    private external fun nativeSeek(_handel: Long, progress: Double): Int
    private external fun nativePause(_handel: Long): Int
}