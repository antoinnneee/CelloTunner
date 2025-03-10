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
    , m_fftBuffer(m_bufferSize)
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
    updateMaximumSampleRate();

    QAudioFormat format;
    format.setSampleRate(m_sampleRate);
    format.setChannelCount(1);
    format.setSampleFormat(QAudioFormat::Int16);

    // Get default audio input device
    QMediaDevices* devices = new QMediaDevices(this);
    QAudioDevice inputDevice = devices->defaultAudioInput();
    
    if (!inputDevice.isFormatSupported(format)) {
        qWarning() << "Default format not supported, trying to use nearest";
        format = inputDevice.preferredFormat();
        m_sampleRate = format.sampleRate();
        emit sampleRateChanged();
    }

    m_audioSource = new QAudioSource(inputDevice, format, this);
}

void TunerEngine::updateMaximumSampleRate()
{
    QMediaDevices* devices = new QMediaDevices(this);
    QAudioDevice inputDevice = devices->defaultAudioInput();
    QAudioFormat format = inputDevice.preferredFormat();
    
    int maxRate = format.sampleRate();
    if (m_maximumSampleRate != maxRate) {
        m_maximumSampleRate = maxRate;
        emit maximumSampleRateChanged();
    }
    qDebug() << "Maximum sample rate:" << m_maximumSampleRate;

    delete devices;
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

    // Process data when we have enough samples
    while (m_accumulationBuffer.size() >= m_bufferSize * 2) {
        processAccumulatedData();
    }
}

void TunerEngine::processAccumulatedData()
{
    QVector<double> samples;
    samples.reserve(m_bufferSize);

    // Convert bytes to doubles
    const qint16* data = reinterpret_cast<const qint16*>(m_accumulationBuffer.constData());
    for (int i = 0; i < m_bufferSize; ++i) {
        samples.append(data[i] / 32768.0); // Normalize to [-1, 1]
    }

    // Remove the processed data from the accumulation buffer
    m_accumulationBuffer.remove(0, m_bufferSize * 2);

    // Calculate and emit signal level
    double dbLevel = calculateDBFS(samples);
    if (m_signalLevel != dbLevel) {
        m_signalLevel = dbLevel;
        emit signalLevelChanged();
        emit signalLevel(dbLevel);
    }

    // Only process frequency if signal is above threshold
    if (dbLevel > m_dbThreshold) {
        double detectedFrequency = detectFrequency(samples);
        if (detectedFrequency > 0) {
            double cents;
            QString note = frequencyToNote(detectedFrequency, cents);
            
            // Update properties
            bool changed = false;
            if (m_currentNote != note) {
                m_currentNote = note;
                emit noteChanged();
                changed = true;
            }
            if (m_frequency != detectedFrequency) {
                m_frequency = detectedFrequency;
                emit frequencyChanged();
                changed = true;
            }
            if (m_cents != cents) {
                m_cents = cents;
                emit centsChanged();
                changed = true;
            }
            
            if (changed) {
                emit noteDetected(note, detectedFrequency, cents);
            }
            
            // Add detailed debug output
            qDebug() << "♪ Note detected:";
            qDebug() << "  - Frequency:" << QString::number(detectedFrequency, 'f', 2) << "Hz";
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
    qsizetype count = std::min(peaks.size(), static_cast<qsizetype>(m_maxPeaks));
    
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
    int maxPeriod = m_sampleRate / 50;  // Minimum frequency of 50 Hz (lower than C2)
    int minPeriod = m_sampleRate / 1100; // Maximum frequency around 1100 Hz

    QVector<Peak> peaks;

    // Find correlation peaks
    double lastCorrelation = 0;
    bool rising = false;
    
    for (int period = minPeriod; period <= maxPeriod; ++period) {
        double correlation = 0;
        int validSamples = 0;

        // Normalized autocorrelation
        for (int i = 0; i < m_bufferSize - period; ++i) {
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
            double frequency = static_cast<double>(m_sampleRate) / (period - 1);
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
    
    // Calculate number of half steps from A4 using the reference frequency
    double halfSteps = 12 * log2(frequency / m_referenceA);
    
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

void TunerEngine::setSampleRate(int rate)
{
    if (m_sampleRate != rate) {
        m_sampleRate = rate;
        // Reconfigure audio input with new sample rate
        stop();
        setupAudioInput();
        start();
        emit sampleRateChanged();
    }
}

void TunerEngine::setBufferSize(int size)
{
    if (m_bufferSize != size) {
        m_bufferSize = size;
        // Resize FFT buffer
        m_fftBuffer.resize(size);
        // Clear accumulation buffer to avoid processing with wrong size
        m_accumulationBuffer.clear();
        emit bufferSizeChanged();
    }
}

void TunerEngine::setMaxPeaks(int peaks)
{
    if (m_maxPeaks != peaks) {
        m_maxPeaks = peaks;
        emit maxPeaksChanged();
    }
}

void TunerEngine::setReferenceA(double freq)
{
    if (m_referenceA != freq) {
        m_referenceA = freq;
        emit referenceAChanged();
    }
} 
