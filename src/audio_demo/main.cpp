#include <audio/audio.h>

#include <boost/lockfree/spsc_queue.hpp>
#include <fmt/format.h>

#include <array>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <deque>
#include <iostream>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>

namespace
{
    using AudioQueue = boost::lockfree::spsc_queue<float>;

    static constexpr float SAMPLE_RATE = 44100.0f;

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
        int time_ms;
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
        if (freq <= 0)
        {
            // TODO : can this be an error or nullopt
            return {};
        }

        if (ms <= 0)
        {
            // TODO : can this be an error or nullopt
            return {};
        }


        auto const samples_per_value = (SAMPLE_RATE / freq) / 2.0f;
        std::vector<float> const positive_osc(static_cast<unsigned int>(samples_per_value), 1.0f);
        std::vector<float> const negative_osc(static_cast<unsigned int>(samples_per_value), -1.0f);

        std::vector<float> whole_sound;
        int const num_samples = static_cast<int>(std::ceil((SAMPLE_RATE / 1000.0f) * ms)); whole_sound.reserve(num_samples + 10);
        whole_sound.reserve(num_samples);
        for (int i = 0; i < freq; ++i)
        {
            whole_sound.insert(end(whole_sound), begin(positive_osc), end(positive_osc));
            whole_sound.insert(end(whole_sound), begin(negative_osc), end(negative_osc));
        }
        return whole_sound;
    }


    bool write_to_buffer(std::deque<float>& source, AudioQueue& dest)
    {
        auto const src_size = source.size();
        if (src_size == 0)
        {
            return false;
        }

        std::int64_t const writes_available = dest.write_available();
        std::int64_t const writes_difference = src_size - writes_available;
        std::int64_t const num_samples = writes_difference > 0 ? writes_available : src_size;

        for (int i = 0; i < num_samples; ++i)
        {
            if (!dest.push(source.front()))
            {
                return false;
            }
            source.pop_front();
        }

        return true;
    }
} // namespace

using namespace std::chrono_literals;
int main()
{
    audio::AudioQueue data{
      boost::lockfree::spsc_queue<float>{audio::BUFFER_SIZE},
      boost::lockfree::spsc_queue<float>{audio::BUFFER_SIZE},
      true,
      0.1f};

    auto portAudio = audio::initialise_backend(&data);
    if (!portAudio->stream->start())
    {
        return -1;
    }

    std::deque<float> whole_song_left;
    for (auto const& note : space_odyssey)
    {
        auto const note_wave = create_square_wave(note.time_ms, (*get_note_freq(note.id))/2);
        whole_song_left.insert(end(whole_song_left), begin(note_wave), end(note_wave));
    }
    std::deque<float> whole_song_right{begin(whole_song_left), end(whole_song_left)};

    bool should_continue = true;
    while (should_continue) // there is audio left to write
    {
        auto const left_written = write_to_buffer(whole_song_left, data.left);
        auto const right_written =  write_to_buffer(whole_song_right, data.right);

        auto const left_available = data.left.write_available() > 0;
        auto const right_available = data.right.write_available() > 0;

        // TODO : What am I trying to do?
        bool queue_ready_test = true;
        if (!data.queue_ready.compare_exchange_weak(queue_ready_test, left_available && right_available))
        {
            continue;
        }
        auto const stream_active = portAudio->stream->is_active();

        should_continue = stream_active &&
          left_written &&
          right_written && 
          (whole_song_left.size() > 0) &&
          (whole_song_right.size() > 0);
    }
    
    // wait for audio to finish
    auto const left_samples_read = data.left.read_available();
    auto const right_samples_read = data.right.read_available();
    auto const wait_for = std::max(left_samples_read, right_samples_read);
    auto const wait_for_ms = static_cast<int>((wait_for / SAMPLE_RATE) * 1000);
    std::this_thread::sleep_for(std::chrono::milliseconds(wait_for_ms + 10));

    if (!portAudio->stream->stop())
    {
        std::cout << fmt::format("failed to stop the stream!");
        return -1;
    }
}
