#include "include/renderer/renderer_plugin.h"

#include <flutter_linux/flutter_linux.h>
#include <gtk/gtk.h>

#include <cstring>
#include <iostream>
#include <memory>

#include "include/renderer/fl_my_texture_gl.h"
#include "include/renderer/opengl_renderer.h"
std::unique_ptr<OpenGLRenderer> openglRenderer;
FlTexture *fl_texture;
int my_texture_name;

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
    fl_texture = texture;
    my_texture_name = texture_name;
    FlTextureRegistrar *texture_registrar = self->texture_registrar;
    fl_texture_registrar_register_texture(texture_registrar, texture);
    fl_texture_registrar_mark_texture_frame_available(texture_registrar,
                                                      texture);
    g_autoptr(FlValue) result =
        fl_value_new_int(reinterpret_cast<int64_t>(texture));
    response = FL_METHOD_RESPONSE(fl_method_success_response_new(result));
  }
  else if (strcmp(method, "addFrame") == 0)
  {
    FlValue *args = fl_method_call_get_args(method_call);
    FlValue *frame_value = fl_value_lookup_string(args, "frame");
    if (frame_value == NULL)
    {
      g_autoptr(FlValue) error_message = fl_value_new_string("Missing frame parameter");
      response = FL_METHOD_RESPONSE(fl_method_error_response_new(
          "INVALID_ARGUMENT", "Missing frame parameter", error_message));
    }
    const uint8_t *frame = fl_value_get_uint8_list(frame_value);
    // size_t frame_length = fl_value_get_length(frame_value);
    // std::clog << "Frame length is " << frame_length << std::endl;
    openglRenderer->update_texture_with_frame(my_texture_name, frame, 1920, 1080);
    FlTextureRegistrar *texture_registrar = self->texture_registrar;
    fl_texture_registrar_mark_texture_frame_available(texture_registrar,
                                                      fl_texture);
    response = FL_METHOD_RESPONSE(fl_method_success_response_new(nullptr));
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