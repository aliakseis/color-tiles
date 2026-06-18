#include "retro_synth.h"
#include <QtMath>
#include <QAudioFormat>
#include <chrono>
#include <cmath>

RetroSynth::RetroSynth(QObject* parent)
    : QIODevice(parent)
{
    QAudioFormat fmt;
    fmt.setSampleRate(44100);
    fmt.setChannelCount(1);

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    // -------------------------
    // Qt5 API
    // -------------------------
    fmt.setSampleSize(16);
    fmt.setSampleType(QAudioFormat::SignedInt);
    fmt.setByteOrder(QAudioFormat::LittleEndian);
    fmt.setCodec("audio/pcm");

    QAudioDeviceInfo dev = QAudioDeviceInfo::defaultOutputDevice();

    if (!dev.isFormatSupported(fmt)) {
        fmt = dev.nearestFormat(fmt);
    }
#else
    // -------------------------
    // Qt6 API
    // -------------------------
    fmt.setSampleFormat(QAudioFormat::Int16);

    QAudioDevice dev = QMediaDevices::defaultAudioOutput();
#endif
    audio = new QAudioOutput(dev, fmt, this);

    // C minor scale
    scale = { 0, 2, 3, 5, 7, 8, 10 };

    rng.seed(std::chrono::high_resolution_clock::now()
        .time_since_epoch().count());

    connect(&tickTimer, &QTimer::timeout, this, &RetroSynth::sequencerStep);
}

void RetroSynth::start() {
    open(QIODevice::ReadOnly);
    audio->start(this);
    tickTimer.start(100);
}

void RetroSynth::stop() {
    tickTimer.stop();
    audio->stop();
    close();
}

double RetroSynth::midiToFreq(int note) {
    return 440.0 * qPow(2.0, (note - 69) / 12.0);
}

void RetroSynth::triggerNote(int v, int midiNote) {
    voices[v].active = true;
    voices[v].freq = midiToFreq(midiNote);
    voices[v].t = 0;
    voices[v].env = 0;
    voices[v].releasing = false;
    voices[v].masterPhase = 0;
    voices[v].slavePhase = 0;
}

void RetroSynth::releaseNote(int v) {
    voices[v].releasing = true;
}

void RetroSynth::initMarkov() {
    int n = scale.size();
    markov.assign(n, std::vector<double>(n, 0.0));

    for (int i = 0; i < n; ++i) {
        double sum = 0.0;
        for (int j = 0; j < n; ++j) {
            int d = qAbs(j - i);
            double w = 0.0;

            if (d == 0) w = 0.2;
            else if (d == 1) w = 0.4;
            else if (d == 2) w = 0.2;
            else if (d == 3) w = 0.1 + metalIntensity * 0.3; // tritone
            else w = 0.05;

            markov[i][j] = w;
            sum += w;
        }
        for (int j = 0; j < n; ++j)
            markov[i][j] /= sum;
    }
}

int RetroSynth::nextStep(int currentStep) {
    int n = scale.size();
    if (currentStep < 0 || currentStep >= n)
        currentStep = (int)(uni(rng) * n);

    double r = uni(rng);
    double acc = 0.0;
    for (int j = 0; j < n; ++j) {
        acc += markov[currentStep][j];
        if (r <= acc)
            return j;
    }
    return n - 1;
}

int RetroSynth::stepToMidi(int root, int stepIndex, int octave) {
    stepIndex = qBound(0, stepIndex, (int)scale.size() - 1);
    return root + scale[stepIndex] + 12 * octave;
}

void RetroSynth::generateMotif(int length) {
    currentMotif.clear();
    currentMotif.reserve(length);

    int n = scale.size();
    int step = (int)(uni(rng) * n);

    for (int i = 0; i < length; ++i) {
        currentMotif.push_back(step);
        step = nextStep(step);
    }

    motifPos = 0;
    motifOctave = (int)(uni(rng) * 2);
}

void RetroSynth::varyMotif() {
    if (currentMotif.empty()) return;

    for (int& s : currentMotif) {
        if (uni(rng) < metalIntensity * 0.5) {
            int delta = (uni(rng) < 0.5 ? -2 : 2);
            s = qBound(0, s + delta, (int)scale.size() - 1);
        }
    }

    if (metalIntensity > 0.6 && uni(rng) < 0.3)
        std::reverse(currentMotif.begin(), currentMotif.end());

    if (uni(rng) < metalIntensity * 0.4)
        motifOctave = (int)(uni(rng) * 3);

    motifPos = 0;
}

void RetroSynth::updateMetalIntensity() {
    double target = uni(rng);
    metalIntensity += (target - metalIntensity) * 0.01;
}

void RetroSynth::sequencerStep() {
    static bool initDone = false;
    if (!initDone) {
        initMarkov();
        generateMotif(8);
        initDone = true;
    }

    updateMetalIntensity();

    // Bass
    if (tick % 8 == 0) {
        int stepIndex = currentMotif[0];
        int n = stepToMidi(36, stepIndex, 0);
        triggerNote(0, n);
    }

    // Arp
    int arpRate = (metalIntensity < 0.5 ? 2 : 1);
    if (tick % arpRate == 0) {
        int stepIndex = currentMotif[motifPos];
        int n = stepToMidi(motifRoot, stepIndex, motifOctave);

        bool accent = (uni(rng) < metalIntensity * 0.4);
        if (accent) n += 12;

        triggerNote(1, n);

        motifPos++;
        if (motifPos >= (int)currentMotif.size()) {
            motifPos = 0;
            if (uni(rng) < 0.5)
                varyMotif();
            else
                generateMotif(8 + (int)(uni(rng) * 4));
        }
    }

    // Drums
    if (tick % 4 == 0) {
        if (uni(rng) < 0.7 + metalIntensity * 0.3) {
            int n = 70 + (int)(uni(rng) * 40);
            triggerNote(2, n);
        }
    }

    tick++;
}

qint64 RetroSynth::readData(char* data, qint64 maxlen) {
    qint16* out = reinterpret_cast<qint16*>(data);
    int samples = maxlen / 2;

    for (int i = 0; i < samples; i++) {
        double mix = 0.0;

        for (auto& v : voices) {
            if (!v.active) continue;

            // ADSR
            double atk = 0.01 - metalIntensity * 0.008;
            double dec = 0.1 - metalIntensity * 0.05;
            double sus = 0.6 + metalIntensity * 0.3;

            if (!v.releasing) {
                if (v.t < atk)
                    v.env = v.t / atk;
                else if (v.t < atk + dec)
                    v.env = 1.0 - (1.0 - sus) * ((v.t - atk) / dec);
                else
                    v.env = sus;
            }
            else {
                v.env -= 1.0 / (v.release * sampleRate);
                if (v.env <= 0) {
                    v.active = false;
                    v.env = 0;
                }
            }

            // Hard-sync + FM
            double masterFreq = v.freq;
            double slaveFreq = v.freq * (1.0 + metalIntensity * 3.0);

            v.masterPhase += masterFreq / sampleRate;
            v.slavePhase += slaveFreq / sampleRate;

            if (v.masterPhase >= 1.0) {
                v.masterPhase -= 1.0;
                v.slavePhase = 0.0;
            }

            double fmMod = sin(2.0 * M_PI * v.slavePhase * (1.0 + metalIntensity * 4.0));
            double fmIndex = 0.5 + metalIntensity * 3.0;

            double phase = v.masterPhase + fmMod * fmIndex;
            phase -= floor(phase);

            double s = (2.0 * phase - 1.0);
            if (metalIntensity > 0.5)
                s = (phase < 0.5 ? 1.0 : -1.0) * 0.7 + s * 0.3;

            // Bitcrush
            int crushBits = 16 - (int)(metalIntensity * 10.0);
            double levels = (1 << crushBits);
            s = floor(s * levels) / levels;

            // Downsample
            static double hold = 0.0;
            static int downCount = 0;
            int downRate = 1 + (int)(metalIntensity * 6.0);

            if (downCount == 0) {
                hold = s;
                downCount = downRate;
            }
            downCount--;
            s = hold;

            // Overdrive
            s = tanh(s * (1.0 + metalIntensity * 3.0));

            mix += s * v.env * 0.3;

            v.t += 1.0 / sampleRate;
        }

        mix = qBound(-1.0, mix, 1.0);
        out[i] = (qint16)(mix * 32767);
    }

    return maxlen;
}
