#ifndef H265_DECODER_H
#define H265_DECODER_H
#include <atomic>
#include <cstdint>
#include <thread>

#include <flutter_linux/flutter_linux.h>
#include <gtk/gtk.h>

struct FFmpegProcess
{
    FILE *input;
    std::thread thread;
};

class H265Decoder
{
public:
    H265Decoder(GdkWindow *window, FlTextureRegistrar *texture_registrar);
    ~H265Decoder();
    _FlTexture *init(int width, int height);
    void addH265Nal(const uint8_t *nal, const size_t size);

private:
    GdkWindow *window;
    FlTextureRegistrar *texture_registrar;
    FFmpegProcess ffmpeg_process;
    std::atomic<bool> thread_run{false};
};

#endif // H265_DECODER_H