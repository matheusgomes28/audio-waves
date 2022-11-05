#include <fmt/format.h>
extern "C"
{
#include <portaudio.h>
}

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
        data->left_phase += 0.01f;
        /* When signal reaches top, drop back down. */
        if( data->left_phase >= 1.0f ) data->left_phase -= 2.0f;
        /* higher pitch so we can distinguish left and right. */
        data->right_phase += 0.03f;
        if( data->right_phase >= 1.0f ) data->right_phase -= 2.0f;
    }
    return 0;
}

int main()
{
    auto const init_error = Pa_Initialize();
    if (init_error != paNoError ) 
    {
        std::cout << fmt::format("PortAudioError : %s\n", Pa_GetErrorText(init_error));
        return -1;
    }


    // This where we play the audio stuff
    PaTestData data{-1.0f, -1.0f};
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
      &data ); /*This is a pointer that will be passed to
      your callback*/

    if(stream_error != paNoError )
    {
        std::string const error_message = Pa_GetErrorText(stream_error);
        std::cout << fmt::format("error opening the stream: %s\n", Pa_GetErrorText(stream_error));
        return -1;
    }

    std::this_thread::sleep_for(std::chrono::seconds(5));

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