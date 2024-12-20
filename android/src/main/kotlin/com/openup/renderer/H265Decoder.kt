package com.openup.renderer

import android.content.Context
import android.hardware.camera2.CameraCharacteristics
import android.hardware.camera2.CameraManager
import android.media.MediaCodec
import android.media.MediaFormat    
import android.view.Surface
import android.opengl.GLES20
import io.flutter.view.TextureRegistry;
import io.flutter.view.TextureRegistry.SurfaceProducer;
import java.nio.ByteBuffer
import android.util.Log

class H265Decoder(private val textureRegistry: TextureRegistry) {
    private var mediaCodec: MediaCodec? = null
    private var surface: Surface? = null
    private var producer: SurfaceProducer? = null
    private var textureId: Long? = null
    private var frameCounter = 0L

    fun init(width: Int, height: Int, csd: ByteArray): Long? {
        try {
            val format = MediaFormat.createVideoFormat(MediaFormat.MIMETYPE_VIDEO_HEVC, width, height)
            format.setByteBuffer("csd-0", ByteBuffer.wrap(csd))
            val mediaCodec = MediaCodec.createDecoderByType(MediaFormat.MIMETYPE_VIDEO_HEVC)
            this.mediaCodec = mediaCodec
            
            val producer = textureRegistry.createSurfaceProducer()
            this.producer = producer
            textureId = producer.id()
            val surface = producer.getSurface()
            this.surface = surface
            
            mediaCodec.configure(format, surface, null, 0)
            mediaCodec.start()
            Log.i("H265Decoder", "Started H265Decoder: " + mediaCodec.getName());
            return textureId
        } catch (e: Exception) {
            Log.e("H265Decoder", "Error during init", e)
            return null;
        }
    }

    fun addH265Nal(nal: ByteArray) {
        val codec = mediaCodec ?: run {
            Log.w("H265Decoder", "Attemted to add H265 data without initing")
            return;
        }

        val inputBufferIndex = codec.dequeueInputBuffer(10000)

        if (inputBufferIndex >= 0) {
            codec.getInputBuffer(inputBufferIndex)?.apply {
                clear()
                put(nal)

                val presentationTimeUs = frameCounter * (1_000_000L / 30)
                frameCounter++
                codec.queueInputBuffer(inputBufferIndex, 0, nal.size, presentationTimeUs, 0)
            }
        }

        val bufferInfo = MediaCodec.BufferInfo()
        var outputBufferIndex: Int

        while (true) {
            outputBufferIndex = codec.dequeueOutputBuffer(bufferInfo, 10000)
            
            if (outputBufferIndex >= 0) {
                // Release output buffer (render it to surface)
                codec.releaseOutputBuffer(outputBufferIndex, true)
                break
            } else if (outputBufferIndex == MediaCodec.INFO_TRY_AGAIN_LATER) {
                break
            }
        }
    }

    fun needsTransformation(): Boolean? {
        return producer?.let { !it.handlesCropAndRotation() }
    }

    fun sensorOrientation(context: Context): Int? {
        val cameraManager = context.getSystemService(Context.CAMERA_SERVICE) as CameraManager
        val cameraId = "0"  // Use the correct camera ID (e.g., "0" for the rear camera)
        val characteristics = cameraManager.getCameraCharacteristics(cameraId)
        val sensorOrientation = characteristics.get(CameraCharacteristics.SENSOR_ORIENTATION)
        return sensorOrientation
    }

    fun cleanup() {
        mediaCodec?.stop();
        mediaCodec = null;
        producer?.release()
        textureId = null
    }
}