#include "audio/audio.h"

#include <boost/lockfree/spsc_queue.hpp>
#include <fmt/format.h>
#include <spdlog/spdlog.h>
extern "C"
{
    #include <portaudio.h>
}

#include <array>
#include <memory>
#include <string>

namespace
{
    void pa_stream_destroy(PaStream* instance)
    {
        if (instance)
        {
            auto const error = Pa_CloseStream(instance);
            if (error != paNoError)
            {
                std::string const message = Pa_GetErrorText(error);
                spdlog::error("error closing the stream %s", message);
            }
        }
    }

    audio::PaStreamPtr create_default_stream(int n_in_channels,
        int n_out_channels,
        int sample_rate,
        audio::PaStreamData* data,
        audio::Callback callback)
    {
        PaStream* stream = nullptr;
        auto const stream_error = Pa_OpenDefaultStream( &stream,
        0,          /* no input channels */
        2,          /* stereo output */
        paFloat32,  /* 32 bit floating point output */
        44100,
        256,        /* frames per buffer, i.e. the number
        of sample frames that PortAudio will
        request from the callback. Many apps
        may want to use
        paFramesPerBufferUnspecified, which
        tells PortAudio to pick the best,
        possibly changing, buffer size.*/
        callback, /* this is your callback function */
        data); /*This is a pointer that will be passed to
        your callback*/

        if(stream_error != paNoError )
        {
            std::string const error_message = Pa_GetErrorText(stream_error);
            spdlog::error("error opening the stream: %s", error_message);
            return audio::PaStreamPtr{nullptr, pa_stream_destroy};
        }
        
        return audio::PaStreamPtr{stream, pa_stream_destroy};
    }

    /* This routine will be called by the PortAudio engine when audio is needed.
    * It may called at interrupt level on some machines so don't do anything
    * that could mess up the system like calling malloc() or free().
    */
    static int default_callback( const void *inputBuffer, void *outputBuffer,
                            unsigned long framesPerBuffer,
                            const PaStreamCallbackTimeInfo* timeInfo,
                            unsigned long statusFlags,
                            void *userData )
    {
        /* Cast data passed through stream to our structure. */
        auto *data = (audio::PaStreamData*)userData;
        float *out = (float*)outputBuffer;
        (void) inputBuffer; /* Prevent unused variable warning. */

        std::array<float, 256> left;
        std::array<float, 256> right;

        if ((data->left.read_available() >= 256) && (data->right.read_available() >= 256))
        {
            for (int i = 0; i < framesPerBuffer; ++i)
            {

                float left_sample = 0.0f;
                if (!data->left.pop(left_sample))
                {
                    return 0;
                }
                *(out++) = left_sample;
                left[i] = left_sample;

                float right_sample = 0.0;
                if (!data->right.pop(right_sample))
                {
                    return 0;
                }
                *(out++) = right_sample;
                right[i] = right_sample;

            }
        }
        return 0;
    }
} // namespace

namespace audio
{
    Stream::Stream(PaStreamData* data, Callback callback)
        : _pa_stream{create_default_stream(0, 2, 44100, data, callback)}
    {
    }

    bool Stream::is_valid()
    {
        return _pa_stream.get() != nullptr;
    }

    bool Stream::start()
    {
        if (!_pa_stream)
        {
            return false;
        }

        auto const error = Pa_StartStream(_pa_stream.get());
        if (error != paNoError)
        {
            std::string const error_message = Pa_GetErrorText(error);
            spdlog::error("error starting the stream: %s", error_message);
            return false;
        }

        return true;
    }

    bool Stream::stop()
    {
        if (!_pa_stream)
        {
            return false;
        }

        auto const error = Pa_StopStream(_pa_stream.get());
        if (error != paNoError)
        {
            std::string const error_message = Pa_GetErrorText(error);
            spdlog::error("error stopping the stream: %s", error_message);
            return false;
        }

        return true;
    }

    AudioBackendPtr initialise_backend(PaStreamData* data)
    {
        auto const init_error = Pa_Initialize();
        if (init_error != paNoError ) 
        {
            std::string const error_message = Pa_GetErrorText(init_error);
            spdlog::error("error initialising PortAudio: %s", error_message);
            return AudioBackendPtr{nullptr, [](AudioBackend* instance){}};
        }

        return AudioBackendPtr{new AudioBackend{std::make_unique<Stream>(data, default_callback)},
          [](AudioBackend* backend){
            backend->stream = nullptr;

            auto const error = Pa_Terminate();
            if(error != paNoError )
            {
                std::string const error_message = Pa_GetErrorText(error);
                spdlog::error("error deiniting PortAudio: %s", error_message);
            }
        }};
    }
} // namespace audio