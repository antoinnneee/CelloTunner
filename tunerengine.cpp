#include "tunerengine.h"
#include <QDebug>
#include <QtMath>
#include <QMediaDevices>
#include <QAudioDevice>
#include <QAudioSource>
#include <QIODevice>
#include <algorithm>
#include <stdlib.h>

TunerEngine::TunerEngine(QObject *parent)
    : QObject(parent)
    , m_audioSource(nullptr)
    , m_audioDevice(nullptr)
    , m_fftBuffer(BUFFER_SIZE)
{
    setupAudioInput();
}

TunerEngine::~TunerEngine()
{
    stop();
    delete m_audioSource;
}

void TunerEngine::setupAudioInput()
{
    QAudioFormat format;
    format.setSampleRate(SAMPLE_RATE);
    format.setChannelCount(1);
    format.setSampleFormat(QAudioFormat::Int16);

    // Get default audio input device
    QMediaDevices* devices = new QMediaDevices(this);
    QAudioDevice inputDevice = devices->defaultAudioInput();
    
    if (!inputDevice.isFormatSupported(format)) {
        qWarning() << "Default format not supported, trying to use nearest";
        format = inputDevice.preferredFormat();
    }

    m_audioSource = new QAudioSource(inputDevice, format, this);
}

void TunerEngine::start()
{
    if (m_audioSource && !m_audioDevice) {
        m_accumulationBuffer.clear(); // Clear the accumulation buffer when starting
        m_audioDevice = m_audioSource->start();
        connect(m_audioDevice, &QIODevice::readyRead, this, &TunerEngine::processAudioInput);
    }
}

void TunerEngine::stop()
{
    if (m_audioSource) {
        m_audioSource->stop();
        if (m_audioDevice) {
            disconnect(m_audioDevice, &QIODevice::readyRead, this, &TunerEngine::processAudioInput);
            m_audioDevice = nullptr;
        }
        m_accumulationBuffer.clear(); // Clear the accumulation buffer when stopping
    }
}

void TunerEngine::processAudioInput()
{
    if (!m_audioDevice) return;

    // Read available data and add it to accumulation buffer
    QByteArray buffer = m_audioDevice->readAll();
    m_accumulationBuffer.append(buffer);
//    qDebug() << "Accumulated buffer size:" << m_accumulationBuffer.size();

    // Process data when we have enough samples
    while (m_accumulationBuffer.size() >= BUFFER_SIZE * 2) {
        processAccumulatedData();
    }
}

double TunerEngine::calculateMedianFrequency(double newFrequency)
{
    // Add new frequency to history
    m_frequencyHistory.enqueue(newFrequency);
    
    // Keep only the last HISTORY_SIZE values
    while (m_frequencyHistory.size() > HISTORY_SIZE) {
        m_frequencyHistory.dequeue();
    }
    
    // If we don't have enough samples yet, return the new frequency
    if (m_frequencyHistory.size() < HISTORY_SIZE) {
        return newFrequency;
    }

    // Copy values to a vector for sorting
    QVector<double> sortedFreqs = m_frequencyHistory.toVector();
    std::sort(sortedFreqs.begin(), sortedFreqs.end());

    // Remove outliers (lowest and highest values)
  //  sortedFreqs.remove(0, OUTLIERS_TO_REMOVE);  // Remove lowest
    sortedFreqs.remove(sortedFreqs.size() - OUTLIERS_TO_REMOVE, OUTLIERS_TO_REMOVE);  // Remove highest

    // Calculate average of remaining values
    double sum = 0.0;
    for (double freq : sortedFreqs) {
        sum += freq;
    }
    
    return sum / sortedFreqs.size();
}

void TunerEngine::processAccumulatedData()
{
    QVector<double> samples;
    samples.reserve(BUFFER_SIZE);

    // Convert bytes to doubles
    const qint16* data = reinterpret_cast<const qint16*>(m_accumulationBuffer.constData());
    for (int i = 0; i < BUFFER_SIZE; ++i) {
        samples.append(data[i] / 32768.0); // Normalize to [-1, 1]
    }

    // Remove the processed data from the accumulation buffer
    m_accumulationBuffer.remove(0, BUFFER_SIZE * 2);

    // Calculate and emit signal level
    double dbLevel = calculateDBFS(samples);
    if (m_signalLevel != dbLevel) {
        m_signalLevel = dbLevel;
        emit signalLevelChanged();
        emit signalLevel(dbLevel);
    }

    // Clear history and keep last note display when signal is too weak
    if (dbLevel <= m_dbThreshold) {
        m_frequencyHistory.clear();
        return;
    }

    // Only process frequency if signal is above threshold
    double rawFrequency = detectFrequency(samples);
    if (rawFrequency > 0) {
        // Calculate median frequency from history
        double smoothedFrequency = calculateMedianFrequency(rawFrequency);
        
        double cents;
        QString note = frequencyToNote(smoothedFrequency, cents);
        
        // Update properties
        bool changed = false;
        if (m_currentNote != note) {
            m_currentNote = note;
            emit noteChanged();
            changed = true;
        }
        if (m_frequency != smoothedFrequency) {
            m_frequency = smoothedFrequency;
            emit frequencyChanged();
            changed = true;
        }
        if (m_cents != cents) {
            m_cents = cents;
            emit centsChanged();
            changed = true;
        }
        
        if (changed) {
            emit noteDetected(note, smoothedFrequency, cents);
        }
        
        // Add detailed debug output
        qDebug() << "♪ Note detected:";
        qDebug() << "  - Raw frequency:" << QString::number(rawFrequency, 'f', 2) << "Hz";
        qDebug() << "  - Smoothed frequency:" << QString::number(smoothedFrequency, 'f', 2) << "Hz";
        qDebug() << "  - History size:" << m_frequencyHistory.size();
        qDebug() << "  - Note:" << note;
        qDebug() << "  - Cents deviation:" << QString::number(cents, 'f', 1);
        qDebug() << "  - Signal level:" << QString::number(dbLevel, 'f', 1) << "dBFS";
        
        // Add tuning guidance
        if (qAbs(cents) < 5) {
            qDebug() << "  ✓ In tune!";
        } else if (cents > 0) {
            qDebug() << "  ↓ Pitch is sharp - lower the pitch";
        } else {
            qDebug() << "  ↑ Pitch is flat - raise the pitch";
        }
    }
}

double TunerEngine::calculateDBFS(const QVector<double>& samples)
{
    if (samples.isEmpty()) return -90.0; // Return minimal level if no samples

    // Calculate RMS (Root Mean Square)
    double sum = 0.0;
    for (double sample : samples) {
        sum += sample * sample;
    }
    double rms = std::sqrt(sum / samples.size());

    // Convert to dBFS (0 dBFS = maximum level = 1.0)
    double dbFS = 20 * std::log10(rms);
    
    // Clamp the lower bound to -90 dB
    return std::max(dbFS, -90.0);
}

void TunerEngine::updatePeaks(const QVector<Peak>& peaks)
{
    m_peaks.clear();
    qsizetype count = std::min(peaks.size(), static_cast<qsizetype>(MAX_PEAKS));
    
    // Find maximum amplitude for normalization
    double maxAmplitude = 0.0;
    for (const Peak& peak : peaks) {
        maxAmplitude = std::max(maxAmplitude, peak.amplitude);
    }
    
    // Avoid division by zero
    if (maxAmplitude <= 0.0) maxAmplitude = 1.0;
    
    // Add normalized peaks to the list
    for (qsizetype i = 0; i < count; ++i) {
        QVariantMap peak;
        peak["frequency"] = peaks[i].frequency;
        // Normalize amplitude to [0,1] range
        peak["amplitude"] = peaks[i].amplitude / maxAmplitude;
        m_peaks.append(peak);
    }
    
    emit peaksChanged();
}

double TunerEngine::detectFrequency(const QVector<double>& samples)
{
    // Adjust frequency range for cello (C2 ~65Hz to C6 ~1047Hz)
    int maxPeriod = SAMPLE_RATE / 50;  // Minimum frequency of 50 Hz (lower than C2)
    int minPeriod = SAMPLE_RATE / 1100; // Maximum frequency around 1100 Hz

    QVector<Peak> peaks;

    // Find correlation peaks
    double lastCorrelation = 0;
    bool rising = false;
    
    for (int period = minPeriod; period <= maxPeriod; ++period) {
        double correlation = 0;
        int validSamples = 0;

        // Normalized autocorrelation
        for (int i = 0; i < BUFFER_SIZE - period; ++i) {
            correlation += samples[i] * samples[i + period];
            validSamples++;
        }

        // Normalize by the number of samples
        if (validSamples > 0) {
            correlation /= validSamples;
        }

        // Detect peaks
        if (rising && correlation < lastCorrelation) {
            // We just passed a peak
            double frequency = static_cast<double>(SAMPLE_RATE) / (period - 1);
            peaks.append({frequency, std::abs(lastCorrelation)});
            rising = false;
        } else if (correlation > lastCorrelation) {
            rising = true;
        }

        lastCorrelation = correlation;
    }

    // No peaks found
    if (peaks.isEmpty()) {
        updatePeaks(QVector<Peak>());
        return 0;
    }

    // Sort peaks by correlation strength (amplitude)
    std::sort(peaks.begin(), peaks.end(), 
              [](const Peak& a, const Peak& b) { return a.amplitude > b.amplitude; });

    // Update the peaks list for visualization
    updatePeaks(peaks);

    // Take only the top peaks
    QVector<Peak> topPeaks;
    for (int i = 0; i < std::min(5, (int)peaks.size()); ++i) {
        topPeaks.append(peaks[i]);
    }

    // Sort top peaks by frequency (ascending)
    std::sort(topPeaks.begin(), topPeaks.end(),
              [](const Peak& a, const Peak& b) { return a.frequency < b.frequency; });

    // Find fundamental frequencies with harmonics
    struct FundamentalCandidate {
        double frequency;
        double amplitude;
        int harmonicCount;
    };
    QVector<FundamentalCandidate> candidates;

    // Check each peak as a potential fundamental
    for (const Peak& fundamental : topPeaks) {
        int harmonicCount = 0;
        bool hasFirstHarmonic = false;

        // Look for harmonics
        for (const Peak& peak : topPeaks) {
            if (peak.frequency > fundamental.frequency) {
                double ratio = peak.frequency / fundamental.frequency;
                double roundedRatio = std::round(ratio);
                
                // Check if this is a harmonic (within 5% tolerance)
                if (std::abs(ratio - roundedRatio) < 0.05) {
                    harmonicCount++;
                    // Check specifically for the first harmonic (2x frequency)
                    if (std::abs(ratio - 2.0) < 0.05) {
                        hasFirstHarmonic = true;
                    }
                }
            }
        }

        // Only consider frequencies that have the first harmonic
        if (hasFirstHarmonic) {
            candidates.append({fundamental.frequency, fundamental.amplitude, harmonicCount});
        }
    }

    // If we found valid candidates with harmonics
    if (!candidates.isEmpty()) {
        // Sort candidates by number of harmonics, then by amplitude
        std::sort(candidates.begin(), candidates.end(),
                 [](const FundamentalCandidate& a, const FundamentalCandidate& b) {
                     if (a.harmonicCount == b.harmonicCount) {
                         return a.amplitude > b.amplitude;
                     }
                     return a.harmonicCount > b.harmonicCount;
                 });
        
        // Debug output for harmonics
        qDebug() << "Found fundamental:" << candidates[0].frequency 
                 << "Hz with" << candidates[0].harmonicCount << "harmonics";
        
        return candidates[0].frequency;
    }

    // If no valid fundamental with harmonics was found, return 0
    return 0;
}

QString TunerEngine::frequencyToNote(double frequency, double& cents)
{
    static const QStringList noteNames = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    
    // Calculate number of half steps from A4
    double halfSteps = 12 * log2(frequency / A4_FREQUENCY);
    
    // Round to nearest note
    int roundedHalfSteps = qRound(halfSteps);
    
    // Calculate cents deviation
    cents = 100 * (halfSteps - roundedHalfSteps);
    
    // Calculate note index and octave
    int noteIndex = (roundedHalfSteps + 9) % 12; // +9 because A is at index 9
    if (noteIndex < 0) noteIndex += 12;
    
    int octave = 4 + (roundedHalfSteps + 9) / 12;
    
    return noteNames[noteIndex] + QString::number(octave);
}

void TunerEngine::setDbThreshold(double threshold)
{
    if (m_dbThreshold != threshold) {
        m_dbThreshold = threshold;
        emit dbThresholdChanged();
    }
} 
