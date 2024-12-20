#include "include/renderer/fl_my_texture_gl.h"
#include "include/renderer/opengl_renderer.h"

G_DEFINE_TYPE(FlMyTextureGL,
              fl_my_texture_gl,
              fl_texture_gl_get_type())

static gboolean fl_my_texture_gl_populate(FlTextureGL *texture,
                                          uint32_t *target,
                                          uint32_t *name,
                                          uint32_t *width,
                                          uint32_t *height,
                                          GError **error)
{
    FlMyTextureGL *f = (FlMyTextureGL *)texture;
    *target = f->target;
    *name = f->name;
    *width = f->width;
    *height = f->height;
    return true;
}

FlMyTextureGL *fl_my_texture_gl_new(uint32_t target,
                                    uint32_t name,
                                    uint32_t width,
                                    uint32_t height)
{
    auto r = FL_MY_TEXTURE_GL(g_object_new(fl_my_texture_gl_get_type(), nullptr));
    r->target = target;
    r->name = name;
    r->width = width;
    r->height = height;
    return r;
}

static void fl_my_texture_gl_class_init(
    FlMyTextureGLClass *klass)
{
    FL_TEXTURE_GL_CLASS(klass)->populate =
        fl_my_texture_gl_populate;
}

static void fl_my_texture_gl_init(FlMyTextureGL *self)
{
}