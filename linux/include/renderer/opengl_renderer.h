#ifndef OPENGL_RENDERER_FLUTTER_H
#define OPENGL_RENDERER_FLUTTER_H
#include <gtk/gtk.h>
#include <GL/gl.h>
#include <vector>

class OpenGLRenderer
{
    GdkGLContext *context;

public:
    OpenGLRenderer(GdkGLContext *context)
    {
        this->context = context;
    }

    int genTexture(int width, int height)
    {
        std::vector<uint8_t> buffer;
        for (int i = 0; i < width * height * 4; i++)
        {
            buffer.push_back(i % 255);
        }
        unsigned int texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer.data());

        return texture;
    }

    void update_texture_with_frame(int texture_name, const uint8_t *frame_data, int width, int height)
    {
        glBindTexture(GL_TEXTURE_2D, texture_name);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, frame_data);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
};

#endif // OPENGL_RENDERER_FLUTTER_H