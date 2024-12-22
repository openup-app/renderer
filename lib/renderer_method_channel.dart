import 'dart:io';
import 'dart:typed_data';

import 'package:flutter/foundation.dart';
import 'package:flutter/services.dart';

import 'renderer_platform_interface.dart';

/// An implementation of [RendererPlatform] that uses method channels.
class MethodChannelRenderer extends RendererPlatform {
  /// The method channel used to interact with the native platform.
  @visibleForTesting
  final methodChannel = const MethodChannel('com.openup.streamline/renderer');

  @override
  Future<int?> init(int width, int height, ParameterSets parameterSets) async {
    if (Platform.isIOS) {
      final textureId = await methodChannel.invokeMethod<int>(
        'init',
        {
          'width': width,
          'height': height,
          'vps': parameterSets.vps,
          'sps': parameterSets.sps,
          'pps': parameterSets.pps,
        },
      );
      return textureId;
    } else if (Platform.isAndroid) {
      final csdBuilder = BytesBuilder();
      csdBuilder
        ..add([0, 0, 0, 1])
        ..add(parameterSets.vps)
        ..add([0, 0, 0, 1])
        ..add(parameterSets.sps)
        ..add([0, 0, 0, 1])
        ..add(parameterSets.pps);
      final textureId = await methodChannel.invokeMethod<int>(
        'init',
        {
          'width': width,
          'height': height,
          'csd': csdBuilder.toBytes(),
        },
      );
      return textureId;
    } else if (Platform.isLinux) {
      final textureId = await methodChannel.invokeMethod<int>(
        'init',
        {
          'width': width,
          'height': height,
        },
      );
      return textureId;
    } else {
      throw 'Unsupported platform';
    }
  }

  @override
  Future<void> dispose() async {
    await methodChannel.invokeMethod<void>('dispose');
  }

  @override
  Future<void> addH265Nal(Uint8List nal) async {
    await methodChannel.invokeMethod<void>('addH265Nal', nal);
  }

  @override
  Future<bool?> needsTransformation() async {
    if (Platform.isIOS) {
      return Future.value(true);
    } else {
      final result =
          await methodChannel.invokeMethod<bool>('needsTransformation');
      return result;
    }
  }

  @override
  Future<int?> sensorOrientation() async {
    if (Platform.isIOS) {
      return Future.value(0);
    } else {
      final result = await methodChannel.invokeMethod<int>('sensorOrientation');
      return result;
    }
  }

  @override
  Future<void> addFrame(Uint8List frame) async {
    await methodChannel.invokeMethod<void>('addFrame', {'frame': frame});
  }

  @override
  Future<void> test() async {
    await methodChannel.invokeMethod<void>('test');
  }
}
