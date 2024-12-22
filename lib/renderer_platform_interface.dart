import 'dart:typed_data';

import 'package:plugin_platform_interface/plugin_platform_interface.dart';

import 'renderer_method_channel.dart';

abstract class RendererPlatform extends PlatformInterface {
  /// Constructs a RendererPlatform.
  RendererPlatform() : super(token: _token);

  static final Object _token = Object();

  static RendererPlatform _instance = MethodChannelRenderer();

  /// The default instance of [RendererPlatform] to use.
  ///
  /// Defaults to [MethodChannelRenderer].
  static RendererPlatform get instance => _instance;

  /// Platform-specific implementations should set this with their own
  /// platform-specific class that extends [RendererPlatform] when
  /// they register themselves.
  static set instance(RendererPlatform instance) {
    PlatformInterface.verifyToken(instance, _token);
    _instance = instance;
  }

  Future<int?> init(int width, int height, ParameterSets parameterSets) {
    throw UnimplementedError('init() has not been implemented.');
  }

  Future<void> dispose() {
    throw UnimplementedError('dispose() has not been implemented.');
  }

  Future<void> addH265Nal(Uint8List nal) {
    throw UnimplementedError('addH265Nal() has not been implemented.');
  }

  Future<bool?> needsTransformation() {
    throw UnimplementedError('needsTransformation() has not been implemented.');
  }

  Future<int?> sensorOrientation() {
    throw UnimplementedError('sensorOrientation() has not been implemented.');
  }

  Future<void> addFrame(Uint8List frame) {
    throw UnimplementedError('addFrame() has not been implemented');
  }

  Future<void> test() {
    throw UnimplementedError('test() has not been implemented');
  }
}

class ParameterSets {
  final Uint8List vps;
  final Uint8List sps;
  final Uint8List pps;

  ParameterSets({
    required this.vps,
    required this.sps,
    required this.pps,
  });
}
