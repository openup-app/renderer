import 'package:flutter_test/flutter_test.dart';
import 'package:renderer/renderer.dart';
import 'package:renderer/renderer_platform_interface.dart';
import 'package:renderer/renderer_method_channel.dart';
import 'package:plugin_platform_interface/plugin_platform_interface.dart';

class MockRendererPlatform
    with MockPlatformInterfaceMixin
    implements RendererPlatform {

  @override
  Future<String?> getPlatformVersion() => Future.value('42');
}

void main() {
  final RendererPlatform initialPlatform = RendererPlatform.instance;

  test('$MethodChannelRenderer is the default instance', () {
    expect(initialPlatform, isInstanceOf<MethodChannelRenderer>());
  });

  test('getPlatformVersion', () async {
    Renderer rendererPlugin = Renderer();
    MockRendererPlatform fakePlatform = MockRendererPlatform();
    RendererPlatform.instance = fakePlatform;

    expect(await rendererPlugin.getPlatformVersion(), '42');
  });
}
