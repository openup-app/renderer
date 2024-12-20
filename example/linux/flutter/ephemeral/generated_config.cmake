# Generated code do not commit.
file(TO_CMAKE_PATH "/opt/flutter" FLUTTER_ROOT)
file(TO_CMAKE_PATH "/mnt/extra/dev/streamline/renderer/example" PROJECT_DIR)

set(FLUTTER_VERSION "1.0.0" PARENT_SCOPE)
set(FLUTTER_VERSION_MAJOR 1 PARENT_SCOPE)
set(FLUTTER_VERSION_MINOR 0 PARENT_SCOPE)
set(FLUTTER_VERSION_PATCH 0 PARENT_SCOPE)
set(FLUTTER_VERSION_BUILD 0 PARENT_SCOPE)

# Environment variables to pass to tool_backend.sh
list(APPEND FLUTTER_TOOL_ENVIRONMENT
  "FLUTTER_ROOT=/opt/flutter"
  "PROJECT_DIR=/mnt/extra/dev/streamline/renderer/example"
  "DART_OBFUSCATION=false"
  "TRACK_WIDGET_CREATION=true"
  "TREE_SHAKE_ICONS=false"
  "PACKAGE_CONFIG=/mnt/extra/dev/streamline/renderer/example/.dart_tool/package_config.json"
  "FLUTTER_TARGET=/mnt/extra/dev/streamline/renderer/example/lib/main.dart"
)
