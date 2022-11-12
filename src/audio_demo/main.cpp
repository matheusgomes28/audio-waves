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

    std::vector<float> create_square_wave(int ms, float freq)
    {
        std::vector<float> sound_wave;
        float const num_samples = (44100.0f / 1000.0f) * ms;
        float const step = 3.0f / (44100 / freq);

        sound_wave.reserve(num_samples);
        float current = 0.0f;
        for (int i = 0; i < num_samples; ++i)
        {
            if (current >= 1.0f)
            {
                current = -2.0f;
            }
            sound_wave.push_back(current);
            current += step;
        }

        return sound_wave;
    }

} // namespace

using namespace std::chrono_literals;
int main()
{

    // What happens when portAudio is no longer referenced?
    audio::PaStreamData data{-1.0f, -1.0f, 0.00f};
    auto portAudio = audio::initialise_backend(&data);
    
    if (!portAudio->stream->start())
    {
        return -1;
    }


    std::vector<float> whole_song;
    for (auto const& note : space_odyssey)
    {
        auto const note_wave = create_square_wave(note.time_ms, (*get_note_freq(note.id))/2);
        whole_song.insert(end(whole_song), begin(note_wave), end(note_wave));
    }

    if ((data.left.write_available() >= 512) & (data.right.write_available() >= 512))
    {
        int const total_chunks = whole_song.size() / 512;
        int n_chunks_left = total_chunks;
        while (n_chunks_left > 0) 
        {
            if ((data.left.write_available() >= 512) && (data.left.write_available() >= 512))
            {
                for (int i = 0; i < 512; ++i)
                {
                    float const sample =whole_song.at((total_chunks - n_chunks_left)*512 + i) ;
                    data.left.push(sample);
                    data.right.push(sample);
                }
                n_chunks_left--;
            }
        }
    }

    // stop stream here
    if (!portAudio->stream->stop())
    {
        std::cout << fmt::format("failed to stop the stream!");
        return -1;
    }
}
