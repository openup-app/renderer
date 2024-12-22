#include "include/renderer/renderer_plugin.h"

#include <flutter_linux/flutter_linux.h>
#include <gtk/gtk.h>

#include <cstring>
#include <iostream>
#include <memory>

#include <cstdio>
#include <vector>
#include <functional>
#include <thread>

#include "include/renderer/fl_my_texture_gl.h"
#include "include/renderer/opengl_renderer.h"

struct FFmpegProcess
{
  FILE *input;
  std::thread thread;
};

std::unique_ptr<OpenGLRenderer> openglRenderer;
FFmpegProcess my_ffmpeg_process;

#define RENDERER_PLUGIN(obj)                                     \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), renderer_plugin_get_type(), \
                              RendererPlugin))

struct _RendererPlugin
{
  GObject parent_instance;
  FlTextureRegistrar *texture_registrar;
  FlView *fl_view;
};

G_DEFINE_TYPE(RendererPlugin,
              renderer_plugin,
              g_object_get_type())

struct ProcessPipes
{
  FILE *input;
  FILE *output;
};

ProcessPipes popen2(const char *command)
{
  std::array<int, 2> in_pipe{};
  std::array<int, 2> out_pipe{};

  if (pipe(in_pipe.data()) < 0 || pipe(out_pipe.data()) < 0)
  {
    return {nullptr, nullptr};
  }

  pid_t pid = fork();
  if (pid < 0)
  {
    return {nullptr, nullptr};
  }

  if (pid == 0)
  { // Child process
    dup2(in_pipe[0], STDIN_FILENO);
    dup2(out_pipe[1], STDOUT_FILENO);

    close(in_pipe[0]);
    close(in_pipe[1]);
    close(out_pipe[0]);
    close(out_pipe[1]);

    execl("/bin/sh", "sh", "-c", command, nullptr);
    exit(1);
  }

  // Parent process
  close(in_pipe[0]);
  close(out_pipe[1]);

  return {
      fdopen(in_pipe[1], "w"),
      fdopen(out_pipe[0], "r")};
}

void postToMainThread(std::function<void()> callback)
{
  // Allocate a heap object to ensure the callback survives across threads
  std::function<void()> *cb = new std::function<void()>(std::move(callback));

  // Post the callback to the main thread using g_idle_add
  g_idle_add([](gpointer user_data) -> gboolean
             {
               // Cast the user data to the function pointer
               auto cb = static_cast<std::function<void()> *>(user_data);
               (*cb)();                // Execute the callback
               delete cb;              // Cleanup memory after execution
               return G_SOURCE_REMOVE; // Remove from idle list (it runs only once)
             },
             cb); // Pass callback pointer to main thread
}

FFmpegProcess launchFFmpegWithCallback(const char *command,
                                       std::function<void(const std::vector<uint8_t> &)> callback)
{
  auto pipes = popen2(command);
  if (!pipes.input || !pipes.output)
  {
    perror("Failed to open pipes");
    return {nullptr, {}};
  }

  FILE *input = pipes.input;
  std::thread t([output = pipes.output, callback]()
                {
        const size_t bufferSize = 4096;
        std::vector<uint8_t> buffer(bufferSize);
        std::vector<uint8_t> accumulatedData;

        while (true) {
            size_t bytesRead = fread(buffer.data(), 1, bufferSize, output);
            if (bytesRead == 0) break;

            accumulatedData.insert(accumulatedData.end(), 
                                 buffer.begin(), 
                                 buffer.begin() + bytesRead);

            const size_t frameSize = 1920 * 1080 * 4;
            if (accumulatedData.size() >= frameSize) {
                postToMainThread([accumulatedData, callback]() {
                    callback(accumulatedData);
                });
                accumulatedData.clear();
            }
        }

        if (!accumulatedData.empty()) {
            callback(accumulatedData);
        }

        fclose(output); });

  return {input, std::move(t)};
}

// Called when a method call is received from Flutter.
static void renderer_plugin_handle_method_call(
    RendererPlugin *self,
    FlMethodCall *method_call)
{
  g_autoptr(FlMethodResponse) response = nullptr;

  const gchar *method = fl_method_call_get_name(method_call);

  if (strcmp(method, "init") == 0)
  {
    FlValue *args = fl_method_call_get_args(method_call);
    FlValue *width_value = fl_value_lookup_string(args, "width");
    FlValue *height_value = fl_value_lookup_string(args, "height");
    if (width_value == NULL || height_value == NULL)
    {
      g_autoptr(FlValue) error_message = fl_value_new_string("Missing width or height parameter");
      response = FL_METHOD_RESPONSE(fl_method_error_response_new(
          "INVALID_ARGUMENT", "Missing width or height parameter", error_message));
    }
    int width = fl_value_get_int(width_value);
    int height = fl_value_get_int(height_value);
    std::clog << "Args " << width << "x" << height << std::endl;
    GdkWindow *window = gtk_widget_get_parent_window(GTK_WIDGET(self->fl_view));
    GError *error = NULL;
    GdkGLContext *context = gdk_window_create_gl_context(window, &error);
    gdk_gl_context_make_current(context);
    openglRenderer = std::make_unique<OpenGLRenderer>(context);
    int texture_name = openglRenderer->genTexture(width, height);
    FlMyTextureGL *t =
        fl_my_texture_gl_new(GL_TEXTURE_2D, texture_name, width, height);
    g_autoptr(FlTexture) texture = FL_TEXTURE(t);
    FlTextureRegistrar *texture_registrar = self->texture_registrar;
    fl_texture_registrar_register_texture(texture_registrar, texture);
    fl_texture_registrar_mark_texture_frame_available(texture_registrar,
                                                      texture);

    auto ffmpeg_process = launchFFmpegWithCallback(
        "ffmpeg -hide_banner -probesize 4K -c:v hevc -hwaccel drm -hwaccel_device /dev/dri/renderD128 -i pipe:0 -pix_fmt rgba -f rawvideo -y -",
        [texture_registrar = self->texture_registrar, texture_name, texture](const std::vector<uint8_t> &data)
        {
          openglRenderer->update_texture_with_frame(texture_name, data.data(), 1920, 1080);
          fl_texture_registrar_mark_texture_frame_available(texture_registrar,
                                                            texture);
        });
    my_ffmpeg_process = std::move(ffmpeg_process);
    response = FL_METHOD_RESPONSE(fl_method_success_response_new(nullptr));

    g_autoptr(FlValue) result =
        fl_value_new_int(reinterpret_cast<int64_t>(texture));
    response = FL_METHOD_RESPONSE(fl_method_success_response_new(result));
  }
  else if (strcmp(method, "addH265Nal") == 0)
  {
    FlValue *args = fl_method_call_get_args(method_call);
    const uint8_t *frame = fl_value_get_uint8_list(args);
    const size_t frame_length = fl_value_get_length(args);
    if (frame == NULL)
    {
      g_autoptr(FlValue) error_message = fl_value_new_string("Missing h265 data argument");
      response = FL_METHOD_RESPONSE(fl_method_error_response_new(
          "INVALID_ARGUMENT", "Missing h265 data argument", error_message));
    }
    else
    {
      fwrite(frame, frame_length, 1, my_ffmpeg_process.input);
      response = FL_METHOD_RESPONSE(fl_method_success_response_new(nullptr));
    }
  }
  else
  {
    response = FL_METHOD_RESPONSE(fl_method_not_implemented_response_new());
  }

  fl_method_call_respond(method_call, response, nullptr);
}

static void renderer_plugin_dispose(GObject *object)
{
  G_OBJECT_CLASS(renderer_plugin_parent_class)->dispose(object);
}

static void renderer_plugin_class_init(
    RendererPluginClass *klass)
{
  G_OBJECT_CLASS(klass)->dispose = renderer_plugin_dispose;
}

static void renderer_plugin_init(RendererPlugin *self) {}

static void method_call_cb(FlMethodChannel *channel,
                           FlMethodCall *method_call,
                           gpointer user_data)
{
  RendererPlugin *plugin = RENDERER_PLUGIN(user_data);
  renderer_plugin_handle_method_call(plugin, method_call);
}

void renderer_plugin_register_with_registrar(
    FlPluginRegistrar *registrar)
{
  RendererPlugin *plugin = RENDERER_PLUGIN(
      g_object_new(renderer_plugin_get_type(), nullptr));
  FlView *fl_view = fl_plugin_registrar_get_view(registrar);
  plugin->fl_view = fl_view;
  plugin->texture_registrar =
      fl_plugin_registrar_get_texture_registrar(registrar);
  g_autoptr(FlStandardMethodCodec) codec = fl_standard_method_codec_new();
  g_autoptr(FlMethodChannel) channel =
      fl_method_channel_new(fl_plugin_registrar_get_messenger(registrar),
                            "com.openup.streamline/renderer", FL_METHOD_CODEC(codec));
  fl_method_channel_set_method_call_handler(
      channel, method_call_cb, g_object_ref(plugin), g_object_unref);

  g_object_unref(plugin);
}
