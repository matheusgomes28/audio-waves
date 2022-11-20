#ifndef AUDIO_H

#include <boost/lockfree/spsc_queue.hpp>
#include <memory>

struct PaStreamCallbackTimeInfo;


namespace audio
{
    struct PaStreamData
    {
        float left_phase;
        float right_phase;
        float step;
        boost::lockfree::spsc_queue<float, boost::lockfree::capacity<16384>> left;
        boost::lockfree::spsc_queue<float, boost::lockfree::capacity<16384>> right;
        float multiplier;
    };

    // yes, PaStream* = void* ...
    using PaStreamCallbackFlags = unsigned long;
    using PaStreamDestroyer = void (*)(void*);
    using PaStreamPtr = std::unique_ptr<void, PaStreamDestroyer>;
    using Callback = int (*)(const void*,
    void *,
    unsigned long,
    const PaStreamCallbackTimeInfo*,
    PaStreamCallbackFlags,
    void *);

    class Stream
    {
    public:
        Stream(PaStreamData* data, Callback callback);

        bool start();
        bool stop();
        bool is_active() const;

    private:
        // Maybe take a look at unique_ptr<void, deleter>
        // may not be safe
        PaStreamPtr _pa_stream;
    };

    // Make the default stream -> Stream

    struct AudioBackend
    {
        // only supports one stream for now
        std::unique_ptr<Stream> stream;   
    };

    using BackendDestroyer = void (*)(AudioBackend*);
    using AudioBackendPtr = std::unique_ptr<AudioBackend, BackendDestroyer>;
    AudioBackendPtr initialise_backend(PaStreamData* data);
} // namespace audio

#endif // AUDIO_H
