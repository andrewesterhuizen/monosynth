#pragma once

#include <cmath>
#include <iostream>
#include "VAStateVariableFilter/VAStateVariableFilter.h"

namespace monosynth {

// http://www.martin-finke.de/blog/articles/audio-plugins-018-polyblep-oscillator/
class Oscillator {
    static constexpr float two_pi = 2 * M_PI;

    float sample_rate;
    float phase = 0;
    float phase_delta = 0;

    float poly_blep(float t) {
        float dt = phase_delta / two_pi;
        if (t < dt) {
            t /= dt;
            return t + t - t * t - 1.0;
        } else if (t > 1.0 - dt) {
            t = (t - 1.0) / dt;
            return t * t + t + t + 1.0;
        } else {
            return 0.0;
        }
    }

public:
    Oscillator(float sample_rate)
            : sample_rate(sample_rate) {
    }

    void next_sample(int waveform, float freq, float* out) {
        phase_delta = freq * two_pi / sample_rate;
        double t = phase / two_pi;

        switch (waveform) {
            case 0: // saw
                *out = 1.0 - (2.0 * phase / two_pi);
                *out -= poly_blep(t);

                break;

            case 1: // square
                if (phase <= M_PI) {
                    *out = 1.0;
                } else {
                    *out = -1.0;
                }

                *out += poly_blep(t);
                *out -= poly_blep(fmod(t + 0.5, 1.0));

                break;
        }

        phase += phase_delta;
        while (phase >= two_pi) {
            phase -= two_pi;
        }
    }
};

class EnvelopeGenerator {
    enum class State {
        Active,
        Off
    };

    State state = State::Active;
    float decay;
    float sample_rate_factor;
    float value;

public:
    void set_decay(float value) {
        float v = 1.0f - value * 0.5f;
        this->decay = (0.0003f * sample_rate_factor + sample_rate_factor * powf(v, 22.0f) * 2.0f) * 8.0f;
    }

    EnvelopeGenerator(float sample_rate) {
        state = State::Active;
        value = 1.0f;
        sample_rate_factor = 0.004f * 44100.0f / sample_rate;
        decay = 0.2f;
    }

    float next_sample() {
        switch (state) {
            case State::Active:
                value -= decay * (value + sample_rate_factor);
                if (value < 0.0f) {
                    value = 0.0f;
                    state = State::Off;
                }
                break;
            case State::Off:
                value = 0.0f;
                break;
        }
        return value;
    }

    void trigger() {
        value = 1.0f;
        state = State::Active;
    }
};

class Synth {
    Oscillator osc;
    EnvelopeGenerator eg;
    VAStateVariableFilter svf;

    float freq;
    float volume = 0.3;

    int prev_note = 0;
    bool prev_gate_high = false;
    float prev_decay;
    float prev_filter_cutoff;
    float prev_filter_res;

    float midi_note_to_freq(int note) {
        float tuning_freq = 440;
        float a4_note_number = 69;

        return tuning_freq * pow(2, ((float) note - a4_note_number) / 12);
    };

public:
    Synth(float sample_rate)
            : osc(sample_rate),
              eg(sample_rate) {
        svf.setSampleRate(sample_rate);
        svf.setFilterType(SVFLowpass);
        svf.setResonance(0.5);
    }


    void
    update_settings(int note, int octave_offset, bool gate_high, float eg_value, float filter_cutoff,
                    float filter_res,
                    float filter_env_amount, float decay) {
        int note_with_offset = note + (octave_offset * 12);
        if (note_with_offset != prev_note) {
            prev_note = note_with_offset;
            freq = midi_note_to_freq(note_with_offset);
            eg.trigger(); // retrigger on note change
        }

        if (gate_high && !prev_gate_high) {
            eg.trigger();
        }
        prev_gate_high = gate_high;

        if (decay != prev_decay) {
            eg.set_decay(decay);
        }
        prev_decay = decay;

        if (prev_filter_cutoff != filter_cutoff) {
            svf.setCutoffFreq(filter_cutoff);
        }
        prev_filter_cutoff = filter_cutoff;

        if (prev_filter_res != filter_res) {
            svf.setResonance(filter_res);
        }
        prev_filter_res = filter_res;

        float cutoff_with_modulation = filter_cutoff + (filter_cutoff * eg_value * filter_env_amount);
        svf.setCutoffFreq(cutoff_with_modulation);
    }

    void next_sample(float* out, int note, bool gate_high, int waveform, int octave_offset, float filter_cutoff,
                     float filter_res, float filter_env_amount, float decay, bool vca_use_env) {
        float eg_value = eg.next_sample();
        update_settings(note, octave_offset, gate_high, eg_value, filter_cutoff, filter_res, filter_env_amount,
                        decay);
        osc.next_sample(waveform, freq, out);

        if (vca_use_env) {
            *out *= eg_value;
        }

        *out = svf.processAudioSample(*out, 0);
        *out *= volume;
    }
};

}

