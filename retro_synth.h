#pragma once
#include <QIODevice>
#include <QAudioOutput>
#include <QTimer>
#include <vector>
#include <random>

class RetroSynth : public QIODevice {
    Q_OBJECT
public:
    explicit RetroSynth(QObject* parent = nullptr);
    void start();
    void stop();

protected:
    qint64 readData(char* data, qint64 maxlen) override;
    qint64 writeData(const char*, qint64) override { return 0; }

private:
    double sampleRate = 44100.0;

    struct Voice {
        bool active = false;
        double freq = 0;
        double t = 0;
        double env = 0;
        double attack = 0.01;
        double decay = 0.1;
        double sustain = 0.6;
        double release = 0.2;
        bool releasing = false;

        double masterPhase = 0.0;
        double slavePhase = 0.0;
    };

    Voice voices[8];

    QAudioOutput* audio;
    QTimer tickTimer;

    int tick = 0;

    // Metal intensity (0.0–1.0)
    double metalIntensity = 0.0;

    // Scale + Markov
    std::vector<int> scale;
    std::vector<std::vector<double>> markov;

    // Motif
    std::vector<int> currentMotif;
    int motifPos = 0;
    int motifRoot = 60;
    int motifOctave = 0;

    // RNG
    std::mt19937 rng;
    std::uniform_real_distribution<double> uni{ 0.0, 1.0 };

    // Methods
    void triggerNote(int voiceIndex, int midiNote);
    void releaseNote(int voiceIndex);
    double midiToFreq(int note);

    void initMarkov();
    int nextStep(int currentStep);
    int stepToMidi(int root, int stepIndex, int octave);

    void generateMotif(int length);
    void varyMotif();

    void sequencerStep();
    void updateMetalIntensity();
};
