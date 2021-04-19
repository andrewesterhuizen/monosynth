#include "amh/audio_midi_helper.h"
#include "Synth.h"

int main() {
    using namespace audio_midi_helper;

    AudioMidiHelper::Config config;
    config.sample_rate = 48000;
    config.buffer_size = 256;
    config.stay_open = true;

    // settings
    int note = 0;
    bool gate_high = false;
    int waveform = 1;
    int octave_offset = -1;
    float decay = 0.3;
    float filter_cutoff = 1000;
    float filter_res = 0.8;
    float filter_env_amount = 1.0;
    bool vca_use_env = true;

    monosynth::Synth synth(config.sample_rate);

    AudioMidiHelper::audio_callback_t audio_callback = [&](float* input, float* output, int nFrames) {
        for (int i = 0; i < nFrames * 2; i += 2) {
            synth.next_sample(&output[i], note, gate_high, waveform, octave_offset, filter_cutoff, filter_res,
                              filter_env_amount, decay,
                              vca_use_env);
            output[i + 1] = output[i];
        }
    };

    AudioMidiHelper::midi_callback_t midi_callback = [&](MidiMessage message) {
        switch (message.messageType) {
            case MidiMessageNoteOn:
                gate_high = true;
                note = message.note;
                break;
            case MidiMessageNoteOff:
                gate_high = false;
                break;
        }
    };

    config.audio_callback = &audio_callback;
    config.midi_callback = &midi_callback;

    AudioMidiHelper amh(config);

    return 0;
}
