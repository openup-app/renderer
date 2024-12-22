import 'dart:async';
import 'dart:math';
import 'dart:typed_data';

import 'package:flutter/widgets.dart';
import 'package:native_device_orientation/native_device_orientation.dart';

import 'renderer_platform_interface.dart';

class Renderer {
  Future<int?> init(int width, int height, ParameterSets parameterSets) {
    return RendererPlatform.instance.init(width, height, parameterSets);
  }

  Future<void> dispose() {
    return RendererPlatform.instance.dispose();
  }

  Future<void> addH265Nal(Uint8List nal) {
    return RendererPlatform.instance.addH265Nal(nal);
  }

  Future<bool?> needsTransformation() {
    return RendererPlatform.instance.needsTransformation();
  }

  Future<int?> sensorOrientation() {
    return RendererPlatform.instance.sensorOrientation();
  }

  Future<int> get sensorQuaterTurns async =>
      ((await sensorOrientation() ?? 0) + 360) % 360 ~/ 90;

  Future<void> addFrame(Uint8List frame) {
    return RendererPlatform.instance.addFrame(frame);
  }

  Future<void> test() {
    return RendererPlatform.instance.test();
  }
}

class TextureDisplay extends StatelessWidget {
  final int textureId;
  final int textureWidth;
  final int textureHeight;

  const TextureDisplay({
    super.key,
    required this.textureId,
    required this.textureWidth,
    required this.textureHeight,
  });

  @override
  Widget build(BuildContext context) {
    return LayoutBuilder(
      builder: (context, constraints) {
        final viewportSize = constraints.biggest;
        final scaleX = viewportSize.width / textureWidth;
        final scaleY = viewportSize.height / textureHeight;
        final scale = max(scaleX, scaleY);
        return Transform.scale(
          scale: scale,
          child: Center(
            child: OverflowBox(
              maxWidth: textureWidth.toDouble(),
              maxHeight: textureHeight.toDouble(),
              child: Texture(
                textureId: textureId,
              ),
            ),
          ),
        );
      },
    );
  }
}

class SensorOrientationBuilder extends StatefulWidget {
  final Widget Function(BuildContext context, int sensorOrientationDegrees)
      builder;

  const SensorOrientationBuilder({super.key, required this.builder});

  @override
  State<SensorOrientationBuilder> createState() =>
      _SensorOrientationBuilderState();
}

class _SensorOrientationBuilderState extends State<SensorOrientationBuilder> {
  late final Timer _timer;
  int _sensorOrientationDegrees = 0;
  final renderer = Renderer();

  @override
  void initState() {
    super.initState();
    _timer = Timer.periodic(
      const Duration(seconds: 2),
      (_) => _onTimer(),
    );
    _onTimer();
  }

  void _onTimer() async {
    final degrees = await renderer.sensorOrientation();
    if (mounted && degrees != _sensorOrientationDegrees) {
      setState(() => _sensorOrientationDegrees = degrees ?? 0);
    }
  }

  @override
  void dispose() {
    _timer.cancel();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return widget.builder(context, _sensorOrientationDegrees);
  }
}

class RotateTexture extends StatefulWidget {
  final bool enabled;
  final bool frontCamera;
  final int sensorOrientationDegrees;
  final Widget child;

  const RotateTexture({
    super.key,
    this.enabled = true,
    this.frontCamera = false,
    this.sensorOrientationDegrees = 0,
    required this.child,
  });

  @override
  State<RotateTexture> createState() => _RotateTextureState();
}

class _RotateTextureState extends State<RotateTexture> {
  StreamSubscription? _subscription;
  double _deviceOrientationDegrees = 0.0;

  @override
  void initState() {
    super.initState();
    _subscription = NativeDeviceOrientationCommunicator()
        .onOrientationChanged()
        .listen(_onOrientationChanged);
  }

  @override
  void dispose() {
    _subscription?.cancel();
    super.dispose();
  }

  void _onOrientationChanged(NativeDeviceOrientation orientation) {
    final degrees = switch (orientation) {
      NativeDeviceOrientation.portraitUp => 0.0,
      NativeDeviceOrientation.landscapeRight => 90.0,
      NativeDeviceOrientation.portraitDown => 180.0,
      NativeDeviceOrientation.landscapeLeft => 270.0,
      _ => 0.0,
    };
    setState(() => _deviceOrientationDegrees = degrees);
  }

  @override
  Widget build(BuildContext context) {
    if (!widget.enabled) {
      return widget.child;
    }

    final sign = widget.frontCamera ? 1 : -1;
    final degrees = (widget.sensorOrientationDegrees -
            _deviceOrientationDegrees * sign +
            360) %
        360;
    final quaterTurns = degrees.toInt() ~/ 90;
    print(
        '### Sensor: ${widget.sensorOrientationDegrees}, device $_deviceOrientationDegrees, output $degrees, quater turns $quaterTurns');
    return RotatedBox(
      quarterTurns: quaterTurns,
      child: widget.child,
    );
  }
}
