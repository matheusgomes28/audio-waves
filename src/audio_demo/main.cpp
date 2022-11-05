#include <fmt/format.h>
extern "C"
{
#include <portaudio.h>
}

#include <array>
#include <chrono>
#include <iostream>
#include <string>
#include <thread>


using PsStreamCallback = int (*)(const void *input,
  void *output,
  unsigned long frameCount,
  const PaStreamCallbackTimeInfo* timeInfo,
  PaStreamCallbackFlags statusFlags,
  void *userData);

struct PaTestData
{
    float left_phase;
    float right_phase;
    float step;
};

/* This routine will be called by the PortAudio engine when audio is needed.
 * It may called at interrupt level on some machines so don't do anything
 * that could mess up the system like calling malloc() or free().
*/ 
static int patestCallback( const void *inputBuffer, void *outputBuffer,
                           unsigned long framesPerBuffer,
                           const PaStreamCallbackTimeInfo* timeInfo,
                           PaStreamCallbackFlags statusFlags,
                           void *userData )
{
    /* Cast data passed through stream to our structure. */
    PaTestData *data = (PaTestData*)userData; 
    float *out = (float*)outputBuffer;
    unsigned int i;
    (void) inputBuffer; /* Prevent unused variable warning. */
    
    for( i=0; i<framesPerBuffer; i++ )
    {
        *(out++) = data->left_phase;  /* left */
        *(out++) = data->right_phase;  /* right */
        /* Generate simple sawtooth phaser that ranges between -1.0 and 1.0. */
        data->left_phase += data->step;
        /* When signal reaches top, drop back down. */
        if( data->left_phase >= 1.0f ) data->left_phase -= 2.0f;
        /* higher pitch so we can distinguish left and right. */
        data->right_phase += data->step;
        if( data->right_phase >= 1.0f ) data->right_phase -= 2.0f;
    }
    return 0;
}

enum class NoteId
{
    C1,
    G1,
    C2,
    E2,
    Eb2
};

struct Note
{
    NoteId id;
    float freq;
    float ms;
};

static constexpr std::array<Note, 7> space_odyssey{{
    {NoteId::C1, 523.25f, 1500},
    {NoteId::G1, 783.99f, 1500},
    {NoteId::C2, 1046.50, 1500},
    {NoteId::E2, 1318.51, 600},
    {NoteId::Eb2, 1244.51, 1200}
}};

int main()
{
    auto const init_error = Pa_Initialize();
    if (init_error != paNoError ) 
    {
        std::cout << fmt::format("PortAudioError : %s\n", Pa_GetErrorText(init_error));
        return -1;
    }


    // This where we play the audio stuff
    PaTestData data{-1.0f, -1.0f, 0.00f};
    PaStream *stream = nullptr;

    /* Open an audio I/O stream. */
    auto const stream_error = Pa_OpenDefaultStream( &stream,
      0,          /* no input channels */
      2,          /* stereo output */
      paFloat32,  /* 32 bit floating point output */
      4410,
      256,        /* frames per buffer, i.e. the number
      of sample frames that PortAudio will
      request from the callback. Many apps
      may want to use
      paFramesPerBufferUnspecified, which
      tells PortAudio to pick the best,
      possibly changing, buffer size.*/
      patestCallback, /* this is your callback function */
      &data); /*This is a pointer that will be passed to
      your callback*/

    if(stream_error != paNoError )
    {
        std::string const error_message = Pa_GetErrorText(stream_error);
        std::cout << fmt::format("error opening the stream: %s\n", Pa_GetErrorText(stream_error));
        return -1;
    }

    auto const start_stream_error = Pa_StartStream( stream );
    if (start_stream_error != paNoError)
    {
        std::string const error_message = Pa_GetErrorText(start_stream_error);
        std::cout << fmt::format("error starting the stream: %s\n", error_message);
    }

    for (auto const note : space_odyssey)
    {
        float const freq = note.freq;
        float const step = 3.0f / (44100 / freq);
        data.step = step;
        std::this_thread::sleep_for(std::chrono::microseconds(static_cast<int>(note.ms * 1000)));
    }

    auto const stop_stream_error = Pa_StopStream( stream );
    if (stop_stream_error != paNoError)
    {
        std::cout << fmt::format("error stopping the stream: %s\n", Pa_GetErrorText(stop_stream_error));
    }

    auto const stream_close_error = Pa_CloseStream( stream );
    if (stream_close_error != paNoError)
    {
        std::cout << fmt::format("error closing the stream: %s\n", Pa_GetErrorText(stream_close_error));
    }

    auto const deinit_error = Pa_Terminate();
    if(deinit_error != paNoError )
    {
        std::cout << fmt::format("PortAudio error: %s\n", Pa_GetErrorText(deinit_error));
        return -1;
    }
}