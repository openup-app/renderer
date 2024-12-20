import Flutter
import VideoToolbox
import CoreMedia

class H265Decoder {
    private var textureRegistry: FlutterTextureRegistry
    private var textureId: Int64 = 0
    private var texture: DecodedTextureProvider?
    private var session: VTDecompressionSession?
    private var formatDescription: CMVideoFormatDescription?
    private var frameCounter: Int32 = 0

    init(textureRegistry: FlutterTextureRegistry) {
        self.textureRegistry = textureRegistry
    }
    
    func initialize(width: Int32, height: Int32, vps: Data, sps: Data, pps: Data) -> Int64? {
        var videoFormat: CMVideoFormatDescription?
        let formatStatus = try vps.withUnsafeBytes { vpsPtr in 
            return sps.withUnsafeBytes { spsPtr in 
                return pps.withUnsafeBytes { ppsPtr in 
                    return CMVideoFormatDescriptionCreateFromHEVCParameterSets(
                        allocator: kCFAllocatorDefault, 
                        parameterSetCount: 3,
                        parameterSetPointers: [
                            vpsPtr.bindMemory(to: UInt8.self).baseAddress!,
                            spsPtr.bindMemory(to: UInt8.self).baseAddress!,
                            ppsPtr.bindMemory(to: UInt8.self).baseAddress!,
                        ],
                        parameterSetSizes: [
                            vps.count,
                            sps.count,
                            pps.count,
                        ],
                        nalUnitHeaderLength: 4,
                        extensions: [:] as CFDictionary?,
                        formatDescriptionOut: &videoFormat
                    )
                }
            }
        }

        guard formatStatus == noErr, let formatDesc = videoFormat else {
            print("Failed to create video format description: \(formatStatus)")
            return nil
        }


        var decompressionSession: VTDecompressionSession?
        let status = VTDecompressionSessionCreate(
            allocator: kCFAllocatorDefault,
            formatDescription: formatDesc,
            decoderSpecification: [:] as CFDictionary,
            imageBufferAttributes: [:] as CFDictionary,
            outputCallback: nil,
            decompressionSessionOut: &decompressionSession
        )

        guard status == noErr, let session = decompressionSession else {
            print("Failed to create decompression session: \(status)")
            return nil
        }
        
        self.formatDescription = formatDesc
        self.session = session
        guard let texture = createInitialTexture(width, height) else {
          print("Failed to create textue");
          return nil;
        }

        self.texture = texture
        textureId = self.textureRegistry.register(texture)

        print("Decoder initialized")
        return textureId
    }
    
    func addH265Nal(_ nalData: Data) {
        guard let session = session, let formatDescription = formatDescription else {
            print("No decoder initialized")
            return
        }
        
        var nalBuffer: CMBlockBuffer?
        let status = nalData.withUnsafeBytes { rawBufferPointer in
            return CMBlockBufferCreateWithMemoryBlock(
                allocator: kCFAllocatorDefault,
                memoryBlock: UnsafeMutableRawPointer(mutating: rawBufferPointer.baseAddress!),
                blockLength: nalData.count,
                blockAllocator: kCFAllocatorNull,
                customBlockSource: nil,
                offsetToData: 0,
                dataLength: nalData.count,
                flags: 0,
                blockBufferOut: &nalBuffer
            )
        }
        guard status == noErr, let buffer = nalBuffer else {
            print("Failed to create block buffer")
            return
        }

        let frameDuration = CMTime(value: 1, timescale: 30);
        var timing = CMSampleTimingInfo(
            duration: frameDuration,
            presentationTimeStamp: CMTimeMultiply(frameDuration, multiplier: frameCounter),
            decodeTimeStamp: CMTime.invalid
        )
        self.frameCounter += 1

        var sampleBuffer: CMSampleBuffer?            
        CMSampleBufferCreate(
            allocator: kCFAllocatorDefault,
            dataBuffer: buffer,
            dataReady: true,
            makeDataReadyCallback: nil,
            refcon: nil,
            formatDescription: formatDescription,
            sampleCount: 1,
            sampleTimingEntryCount: 1,
            sampleTimingArray: &timing,
            sampleSizeEntryCount: 0,
            sampleSizeArray: nil,
            sampleBufferOut: &sampleBuffer
        )
        guard let sampBuffer = sampleBuffer else {
            print("Failed to create sample buffer")
            return
        }

        if sampBuffer == nil || CMSampleBufferIsValid(sampBuffer) == false {
            print("Invalid sample buffer!")
        }
        
        let decodeStatus = VTDecompressionSessionDecodeFrame(
            session,
            sampleBuffer: sampBuffer,
            flags: [],
            infoFlagsOut: nil,
            outputHandler: { status, infoFlags, imageBuffer, presentationTimeStamp, duration in
                if status == noErr, let imageBuffer = imageBuffer {
                    self.texture?.setBuffer(imageBuffer);
                    self.textureRegistry.textureFrameAvailable(self.textureId);
                } else {
                    print("Failed to decode frame: \(status)")
                }
            }
        )
        
        guard decodeStatus == noErr else {
            print("Decoding failed with status: \(decodeStatus)")
            return
        }
    }
    
    func cleanup() {
        // session?.invalidate()
        session = nil
        formatDescription = nil
        textureRegistry.unregisterTexture(textureId);
        textureId = 0
    }

    private func createInitialTexture(_ width: Int32, _ height: Int32) -> DecodedTextureProvider? {
      var pixelBuffer : CVPixelBuffer?
      let status = CVPixelBufferCreate(
        kCFAllocatorDefault,
        Int(width),
        Int(height),
        kCVPixelFormatType_32BGRA,
        nil,
        &pixelBuffer
      )
      guard status == kCVReturnSuccess else {
        print("Failed creating CVPixelBuffer \(status)")
        return nil
      }
      return DecodedTextureProvider(pixelBuffer: pixelBuffer)
    }
}

class DecodedTextureProvider: NSObject, FlutterTexture {
    private var pixelBuffer: CVPixelBuffer?
    
    init(pixelBuffer: CVPixelBuffer?) {
        self.pixelBuffer = pixelBuffer
        super.init()
    }
    
    func setBuffer(_ pixelBuffer: CVPixelBuffer) {
        self.pixelBuffer = pixelBuffer
    }

    func copyPixelBuffer() -> Unmanaged<CVPixelBuffer>? {
        guard let buffer = pixelBuffer else {
            return nil
        }
        return Unmanaged.passRetained(buffer)
    }
}