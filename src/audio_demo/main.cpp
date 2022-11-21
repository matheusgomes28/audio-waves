#include <audio/audio.h>

#include <boost/lockfree/spsc_queue.hpp>
#include <fmt/format.h>

#include <array>
#include <chrono>
#include <condition_variable>
#include <deque>
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
        float const step = 2.0f / (44100 / freq);

        sound_wave.reserve(num_samples);
        float current = 0.0f;
        for (int i = 0; i < num_samples; ++i)
        {
            if (current >= 1.0f)
            {
                current = -1.0f;
            }
            sound_wave.push_back(current);
            current += step;
        }

        return sound_wave;
    }

    static std::size_t constexpr audio_buffer_size = 16384;
    using AudioQueue = boost::lockfree::spsc_queue<float, boost::lockfree::capacity<audio_buffer_size>>;

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

    // What happens when portAudio is no longer referenced?
    audio::PaStreamData data{-1.0f, -1.0f, 0.00f, {}, {}, 0.1f, true};
    data.multiplier = 0.1;
    auto portAudio = audio::initialise_backend(&data);

    auto const is_lock_left = data.left.is_lock_free();
    auto const is_lock_right = data.right.is_lock_free();
    
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
        bool queue_ready_test = true;
        
        auto const left_available = data.left.write_available() > 0;
        auto const right_available = data.right.write_available() > 0;

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

        // if the full flag is no longer set
    }
    
    // wait for audio to finish
    auto const left_samples_read = data.left.read_available();
    auto const right_samples_read = data.right.read_available();
    auto const wait_for = std::max(left_samples_read, right_samples_read);
    auto const wait_for_ms = static_cast<int>((wait_for / 44100.0f) * 1000);
    std::this_thread::sleep_for(std::chrono::milliseconds(wait_for_ms + 10));

    // stop stream here
    if (!portAudio->stream->stop())
    {
        std::cout << fmt::format("failed to stop the stream!");
        return -1;
    }
}
