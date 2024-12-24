#include "include/renderer/h265_decoder.h"
#include "include/renderer/renderer_plugin.h"

#include <flutter_linux/flutter_linux.h>
#include <gtk/gtk.h>

#include <cstring>
#include <iostream>
#include <memory>

#include <cstdio>
#include <vector>
#include <functional>

#include "include/renderer/fl_my_texture_gl.h"
#include "include/renderer/opengl_renderer.h"

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
                                       std::atomic<bool> &thread_run,
                                       std::function<void(const std::vector<uint8_t> &)> callback)
{
    auto pipes = popen2(command);
    if (!pipes.input || !pipes.output)
    {
        perror("Failed to open pipes");
        return {nullptr, {}};
    }

    FILE *input = pipes.input;
    std::thread t([output = pipes.output, callback, &thread_run]()
                  {
        thread_run = true;

        const size_t bufferSize = 4096;
        std::vector<uint8_t> buffer(bufferSize);
        std::vector<uint8_t> accumulatedData;

        while (thread_run) {
            size_t bytesRead = fread(buffer.data(), 1, bufferSize, output);
            if (bytesRead == 0) {
                break;
            }

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

H265Decoder::H265Decoder(GdkWindow *window, FlTextureRegistrar *texture_registrar)
{
    this->window = window;
    this->texture_registrar = texture_registrar;
}

H265Decoder::~H265Decoder()
{
    thread_run = false;
}

_FlTexture *H265Decoder::init(int width, int height)
{
    GError *error = NULL;
    GdkGLContext *context = gdk_window_create_gl_context(this->window, &error);
    gdk_gl_context_make_current(context);
    auto openglRenderer = std::make_shared<OpenGLRenderer>(context);
    int texture_name = openglRenderer->genTexture(width, height);
    FlMyTextureGL *t =
        fl_my_texture_gl_new(GL_TEXTURE_2D, texture_name, width, height);
    g_autoptr(FlTexture) texture = FL_TEXTURE(t);
    fl_texture_registrar_register_texture(texture_registrar, texture);
    fl_texture_registrar_mark_texture_frame_available(texture_registrar,
                                                      texture);

    if (width == 1)
    {
        auto ffmpeg_process = launchFFmpegWithCallback(
            "ffmpeg -hide_banner -probesize 4K -c:v hevc -hwaccel drm -hwaccel_device /dev/dri/renderD128 -i /home/openup/source.h265 -f rawvideo -y /dev/null",
            thread_run,
            [](const std::vector<uint8_t> &data)
            {
                // renderer->update_texture_with_frame(texture_name, data.data(), 1920, 1080);
                // fl_texture_registrar_mark_texture_frame_available(texture_registrar, texture);
            });
        this->ffmpeg_process = std::move(ffmpeg_process);
    }
    else if (width == 2)
    {
        auto ffmpeg_process = launchFFmpegWithCallback(
            "ffmpeg -hide_banner -probesize 4K -c:v hevc -hwaccel drm -hwaccel_device /dev/dri/renderD128 -i /home/openup/source.h265 -pix_fmt yuv420p -f rawvideo -y /dev/null",
            thread_run,
            [](const std::vector<uint8_t> &data)
            {
                // renderer->update_texture_with_frame(texture_name, data.data(), 1920, 1080);
                // fl_texture_registrar_mark_texture_frame_available(texture_registrar, texture);
            });
        this->ffmpeg_process = std::move(ffmpeg_process);
    }
    else if (width == 3)
    {
        auto ffmpeg_process = launchFFmpegWithCallback(
            "ffmpeg -hide_banner -probesize 4K -c:v hevc -hwaccel drm -hwaccel_device /dev/dri/renderD128 -i /home/openup/source.h265 -pix_fmt rgba -f rawvideo -y /dev/null",
            thread_run,
            [](const std::vector<uint8_t> &data)
            {
                // renderer->update_texture_with_frame(texture_name, data.data(), 1920, 1080);
                // fl_texture_registrar_mark_texture_frame_available(texture_registrar, texture);
            });
        this->ffmpeg_process = std::move(ffmpeg_process);
    }
    return texture;
}

void H265Decoder::addH265Nal(const uint8_t *nal, const size_t size)
{
    fwrite(nal, size, 1, ffmpeg_process.input);
}