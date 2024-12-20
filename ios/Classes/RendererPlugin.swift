import Flutter
import UIKit

public class RendererPlugin: NSObject, FlutterPlugin {
    private var channel: FlutterMethodChannel?
    private var h265Decoder: H265Decoder?

    public static func register(with registrar: FlutterPluginRegistrar) {
        let channel = FlutterMethodChannel(name: "com.openup.streamline/renderer", binaryMessenger: registrar.messenger())
        let instance = RendererPlugin()
        instance.channel = channel
        instance.h265Decoder = H265Decoder(textureRegistry: registrar.textures())
        registrar.addMethodCallDelegate(instance, channel: channel)
    }
    
    public func handle(_ call: FlutterMethodCall, result: @escaping FlutterResult) {
        switch call.method {
        case "init":
            guard let args = call.arguments as? [String: Any],
                  let width = args["width"] as? Int32,
                  let height = args["height"] as? Int32,
                  let vps = args["vps"] as? FlutterStandardTypedData,
                  let sps = args["sps"] as? FlutterStandardTypedData,
                  let pps = args["pps"] as? FlutterStandardTypedData  else {
                result(FlutterError(code: "INVALID_ARGUMENTS", message: "Invalid initialization parameters", details: nil))
                return
            }
            let textureId = h265Decoder?.initialize(width: width, height: height, vps: vps.data, sps: sps.data, pps: pps.data)
            result(textureId)
            
        case "addH265Nal":
            guard let nal = call.arguments as? FlutterStandardTypedData else {
                result(FlutterError(code: "INVALID_ARGUMENTS", message: "Invalid NAL data", details: nil))
                return
            }
            
            h265Decoder?.addH265Nal(nal.data)
            result(nil)
        case "dispose":
            h265Decoder?.cleanup();
        default:
            result(FlutterMethodNotImplemented)
        }
    }
    
    public func detach(from engine: FlutterEngine) {
        h265Decoder?.cleanup()
        channel?.setMethodCallHandler(nil)
    }
}