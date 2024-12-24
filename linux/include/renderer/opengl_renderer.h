#ifndef OPENGL_RENDERER_FLUTTER_H
#define OPENGL_RENDERER_FLUTTER_H
#include <gtk/gtk.h>
#include <GL/glew.h>
#include <GL/gl.h>
#include <vector>

class OpenGLRenderer
{
    GdkGLContext *context;
    GLuint pbo;

public:
    OpenGLRenderer(GdkGLContext *context)
    {
        this->context = context;
        glGenBuffers(1, &pbo);
    }

    ~OpenGLRenderer()
    {
        glDeleteBuffers(1, &pbo);
    }

    int genTexture(int width, int height)
    {
        std::vector<uint8_t> buffer(width * height * 4, 0);

        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer.data());
        glBindTexture(GL_TEXTURE_2D, 0);

        // Initialize PBO with texture size
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);
        glBufferData(GL_PIXEL_UNPACK_BUFFER, width * height * 4, nullptr, GL_STREAM_DRAW);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

        return texture;
    }

    void update_texture_with_frame(int texture_name, const uint8_t *frame_data, int width, int height)
    {
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo);

        // Map PBO memory and copy the frame data
        void *ptr = glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
        if (ptr)
        {
            memcpy(ptr, frame_data, width * height * 4);
            glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
        }

        // Update the texture using PBO
        glBindTexture(GL_TEXTURE_2D, texture_name);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

        // Unbind
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    }
};

#endif // OPENGL_RENDERER_FLUTTER_H
