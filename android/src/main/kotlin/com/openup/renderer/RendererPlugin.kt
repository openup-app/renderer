package com.openup.renderer

import android.content.Context
import androidx.annotation.NonNull

import io.flutter.embedding.engine.plugins.FlutterPlugin
import io.flutter.plugin.common.MethodCall
import io.flutter.plugin.common.MethodChannel
import io.flutter.plugin.common.MethodChannel.MethodCallHandler
import io.flutter.plugin.common.MethodChannel.Result

class RendererPlugin: FlutterPlugin, MethodCallHandler {
    private lateinit var channel : MethodChannel
    private lateinit var h265Decoder : H265Decoder
    private var context: Context? = null

    override fun onAttachedToEngine(flutterPluginBinding: FlutterPlugin.FlutterPluginBinding) {
        context = flutterPluginBinding.applicationContext
        h265Decoder = H265Decoder(flutterPluginBinding.getTextureRegistry())
        channel = MethodChannel(flutterPluginBinding.binaryMessenger, "com.openup.streamline/renderer")
        channel.setMethodCallHandler(this)
    }

    override fun onMethodCall(call: MethodCall, result: Result) {
        when (call.method) {
            "init" -> {
                val width = call.argument<Int>("width")!!
                val height = call.argument<Int>("height")!!
                val csd = call.argument<ByteArray>("csd")!!
                val textureId = h265Decoder.init(width, height, csd)
                result.success(textureId)
            }
            "addH265Nal" -> {
                val nal = call.arguments<ByteArray>()!!
                h265Decoder.addH265Nal(nal)
                result.success(null)
            }
            "dispose" -> {
              h265Decoder.cleanup()
              result.success(null)
            }
            "needsTransformation" -> {
              val needsTransformation = h265Decoder.needsTransformation();
              result.success(needsTransformation)
            }
            "sensorOrientation" -> {
                val localContext = context
                localContext?.let {
                    val sensorOrientation = h265Decoder.sensorOrientation(localContext);
                    result.success(sensorOrientation)
                } ?: run {
                    result.success(null)
                }
            }
            else -> result.notImplemented()
        }
    }

    override fun onDetachedFromEngine(binding: FlutterPlugin.FlutterPluginBinding) {
        context = null
        channel.setMethodCallHandler(null)
        h265Decoder.cleanup()
    }
}
