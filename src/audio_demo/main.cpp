#include <audio/audio.h>

#include <boost/lockfree/spsc_queue.hpp>
#include <fmt/format.h>

#include <array>
#include <chrono>
#include <iostream>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>

namespace
{
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
        float time_ms;
    };

    static constexpr std::array<Note, 5> space_odyssey{{
        {NoteId::C1, 1500},
        {NoteId::G1, 1500},
        {NoteId::C2, 1500},
        {NoteId::E2, 600},
        {NoteId::Eb2, 1200}
    }};

    inline std::optional<float> get_note_freq(NoteId note_id)
    {
        // NodeId -> Frequence map
        static std::unordered_map<NoteId, float> const note_frequencies{{
            {NoteId::C1, 523.25f},
            {NoteId::G1, 783.99f},
            {NoteId::C2, 1046.50f},
            {NoteId::E2, 1318.51f},
            {NoteId::Eb2, 1244.51f}
        }};

        auto const found = note_frequencies.find(note_id);
        if (found == end(note_frequencies))
        {
            return std::nullopt;
        }

        return found->second;
    }

} // namespace

int main()
{
    boost::lockfree::spsc_queue<float, boost::lockfree::capacity<512>> queue;


    // What happens when portAudio is no longer referenced?
    audio::PaStreamData data{-1.0f, -1.0f, 0.00f};
    auto portAudio = audio::initialise_backend(&data);
    
    if (!portAudio->stream->start())
    {
        return -1;
    }

    for (auto const note : space_odyssey)
    {
        auto const freq = get_note_freq(note.id);
        if (!freq)
        {
            std::cout << "invalid note given\n";
            continue;
        }
        
        float const step = 3.0f / (44100 / *freq);
        data.step = step;
        std::this_thread::sleep_for(std::chrono::microseconds(static_cast<int>(note.time_ms * 1000)));
    }

    // stop stream here
    if (!portAudio->stream->stop())
    {
        std::cout << fmt::format("failed to stop the stream!");
        return -1;
    }
}
