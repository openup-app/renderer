#include "include/renderer/renderer_plugin.h"
#include "include/renderer/h265_decoder.h"

#include <flutter_linux/flutter_linux.h>
#include <gtk/gtk.h>

#include <cstring>

H265Decoder *decoder;

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
    GdkWindow *window = gtk_widget_get_parent_window(GTK_WIDGET(self->fl_view));
    decoder = new H265Decoder(window, self->texture_registrar);
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
    auto texture = decoder->init(width, height);
    g_autoptr(FlValue) result =
        fl_value_new_int(reinterpret_cast<int64_t>(texture));
    response = FL_METHOD_RESPONSE(fl_method_success_response_new(result));
  }
  else if (strcmp(method, "addH265Nal") == 0)
  {
    FlValue *args = fl_method_call_get_args(method_call);
    const uint8_t *nal = fl_value_get_uint8_list(args);
    const size_t size = fl_value_get_length(args);
    if (nal == NULL)
    {
      g_autoptr(FlValue) error_message = fl_value_new_string("Missing h265 data argument");
      response = FL_METHOD_RESPONSE(fl_method_error_response_new(
          "INVALID_ARGUMENT", "Missing h265 data argument", error_message));
    }
    else
    {
      if (decoder == nullptr)
      {
        g_autoptr(FlValue) error_message = fl_value_new_string("Decoder has not been initialized");
        response = FL_METHOD_RESPONSE(fl_method_error_response_new(
            "BAD_STATE", "Decoder has not been initialized", error_message));
      }
      else
      {
        decoder->addH265Nal(nal, size);
        response = FL_METHOD_RESPONSE(fl_method_success_response_new(nullptr));
      }
    }
  }
  else if (strcmp(method, "dispose") == 0)
  {
    decoder = nullptr;
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
