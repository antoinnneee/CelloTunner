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
        double detectedFrequency;
        
        // Use selected detection method
        if (m_detectionMethod == "FFT") {
            detectedFrequency = detectFrequencyFFT(samples);
        } else {
            detectedFrequency = detectFrequencyAutocorrelation(samples);
        }

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
    
    if (peaks.isEmpty()) {
        emit peaksChanged();
        return;
    }
    
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
        peak["amplitude"] = std::max(0.05, peaks[i].amplitude / maxAmplitude); // Minimum visible height
        peak["harmonicCount"] = peaks[i].harmonicCount;
        m_peaks.append(peak);
    }
    
    emit peaksChanged();
}

double TunerEngine::detectFrequencyAutocorrelation(const QVector<double>& samples)
{
    int maxPeriod = m_sampleRate / 50;  // Minimum frequency of 50 Hz
    int minPeriod = m_sampleRate / 1500; // Maximum frequency around 1500 Hz

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
            peaks.append({frequency, std::abs(lastCorrelation), 0});
            rising = false;
        } else if (correlation > lastCorrelation) {
            rising = true;
        }

        lastCorrelation = correlation;
    }

    if (peaks.isEmpty()) {
        updatePeaks(QVector<Peak>());
        return 0;
    }

    // Sort peaks by correlation strength initially
    std::sort(peaks.begin(), peaks.end(), 
              [](const Peak& a, const Peak& b) { return a.amplitude > b.amplitude; });

    // Take top peaks
    QVector<Peak> topPeaks;
    for (int i = 0; i < std::min(5, (int)peaks.size()); ++i) {
        topPeaks.append(peaks[i]);
    }

    // Sort by frequency to analyze harmonics
    std::sort(topPeaks.begin(), topPeaks.end(),
              [](const Peak& a, const Peak& b) { return a.frequency < b.frequency; });

    // Analyze harmonics for each peak
    for (Peak& fundamental : topPeaks) {
        analyzeHarmonics(fundamental, topPeaks);
    }

    // Update peaks for visualization
    updatePeaks(topPeaks);

    // Find the peak with the most harmonics
    // If multiple peaks have the same number of harmonics, take the lowest frequency
    Peak* bestPeak = nullptr;
    for (Peak& peak : topPeaks) {
        if (!bestPeak || 
            peak.harmonicCount > bestPeak->harmonicCount || 
            (peak.harmonicCount == bestPeak->harmonicCount && peak.frequency < bestPeak->frequency)) {
            bestPeak = &peak;
        }
    }

    return bestPeak ? bestPeak->frequency : 0;
}

void TunerEngine::analyzeHarmonics(Peak& fundamental, const QVector<Peak>& peaks) {
    int harmonicCount = 0;
    double harmonicStrength = 0;
    
    // Expected harmonic ratios for string instruments
    const double expectedHarmonics[] = {2.0, 3.0, 4.0, 5.0, 6.0};
    
    for (const Peak& peak : peaks) {
        if (peak.frequency > fundamental.frequency) {
            double ratio = peak.frequency / fundamental.frequency;
            
            // Check against expected harmonics
            for (double expected : expectedHarmonics) {
                if (std::abs(ratio - expected) < 0.03) { // 3% tolerance
                    harmonicCount++;
                    // Higher harmonics contribute less
                    harmonicStrength += peak.amplitude / expected;
                    break;
                }
            }
        }
    }
    
    fundamental.harmonicCount = harmonicCount;
    fundamental.harmonicStrength = harmonicStrength;
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

void TunerEngine::applyHannWindow(QVector<double>& samples)
{
    for (int i = 0; i < samples.size(); i++) {
        double window = 0.5 * (1 - cos(2 * M_PI * i / (samples.size() - 1)));
        samples[i] *= window;
    }
}

void TunerEngine::performFFT(QVector<std::complex<double>>& data)
{
    int n = data.size();
    if (n <= 1) return;

    // Separate even and odd elements
    QVector<std::complex<double>> even(n/2);
    QVector<std::complex<double>> odd(n/2);
    for (int i = 0; i < n/2; i++) {
        even[i] = data[2*i];
        odd[i] = data[2*i+1];
    }

    // Recursive FFT on even and odd parts
    performFFT(even);
    performFFT(odd);

    // Combine results
    for (int k = 0; k < n/2; k++) {
        double angle = -2 * M_PI * k / n;
        std::complex<double> t = std::polar(1.0, angle) * odd[k];
        data[k] = even[k] + t;
        data[k + n/2] = even[k] - t;
    }
}

double TunerEngine::detectFrequencyFFT(const QVector<double>& samples)
{
    // Create a copy for windowing
    QVector<double> windowed = samples;
    applyHannWindow(windowed);

    // Zero padding for better frequency resolution
    int paddedSize = m_bufferSize * m_fftPadding;  // Use padding factor
    m_fftBuffer.resize(paddedSize);
    
    // Copy samples and pad with zeros
    for (int i = 0; i < paddedSize; i++) {
        m_fftBuffer[i] = i < samples.size() ? 
            std::complex<double>(windowed[i], 0) : 
            std::complex<double>(0, 0);
    }

    // Perform FFT
    performFFT(m_fftBuffer);

    // Calculate frequency step size
    double freqStep = static_cast<double>(m_sampleRate) / paddedSize;
    qDebug() << "FFT frequency resolution:" << freqStep << "Hz";

    // Calculate magnitude spectrum and find peaks
    QVector<Peak> peaks;
    double maxMagnitude = 0;
    
    // Only look at the meaningful part of the spectrum
    for (int i = 1; i < paddedSize/2 - 1; i++) {
        double magnitude = std::abs(m_fftBuffer[i]);
        double frequency = i * freqStep;
        
        // Only consider frequencies in our range of interest (50Hz to 1500Hz)
        if (frequency >= 50 && frequency <= 1500) {
            // Look for peaks in the spectrum
            if (magnitude > std::abs(m_fftBuffer[i-1]) && 
                magnitude > std::abs(m_fftBuffer[i+1])) {
                
                // Quadratic interpolation for better frequency precision
                double alpha = std::abs(m_fftBuffer[i-1]);
                double beta = magnitude;
                double gamma = std::abs(m_fftBuffer[i+1]);
                double p = 0.5 * (alpha - gamma) / (alpha - 2*beta + gamma);
                
                // Refined frequency
                double refinedFreq = (i + p) * freqStep;
                
                peaks.append({refinedFreq, magnitude, 0, 0}); // Initialize harmonicStrength to 0
                maxMagnitude = std::max(maxMagnitude, magnitude);
            }
        }
    }

    if (peaks.isEmpty()) {
        updatePeaks(QVector<Peak>());
        return 0;
    }

    // Sort peaks by magnitude
    std::sort(peaks.begin(), peaks.end(),
              [](const Peak& a, const Peak& b) { return a.amplitude > b.amplitude; });

    // Take top peaks
    QVector<Peak> topPeaks;
    for (int i = 0; i < std::min(5, (int)peaks.size()); i++) {
        Peak normalizedPeak = peaks[i];
        normalizedPeak.amplitude /= maxMagnitude; // Normalize amplitude
        topPeaks.append(normalizedPeak);
    }

    // Sort by frequency to analyze harmonics
    std::sort(topPeaks.begin(), topPeaks.end(),
              [](const Peak& a, const Peak& b) { return a.frequency < b.frequency; });

    // Analyze harmonics for each peak
    for (Peak& fundamental : topPeaks) {
        analyzeHarmonics(fundamental, topPeaks);
    }

    // Update peaks for visualization
    updatePeaks(topPeaks);

    // Use the new peak selection method
    Peak* bestPeak = selectBestPeak(topPeaks);
    if (bestPeak) {
        // Apply frequency stability check
        return getStableFrequency(bestPeak->frequency, calculateNoteProbability(*bestPeak));
    }

    return 0;
}

double TunerEngine::getNearestNoteFrequency(double frequency) const {
    // A4 = 440Hz is our reference
    double halfSteps = 12 * std::log2(frequency / m_referenceA);
    int roundedHalfSteps = qRound(halfSteps);
    return m_referenceA * std::pow(2, roundedHalfSteps / 12.0);
}

double TunerEngine::calculateNoteProbability(const Peak& peak) const {
    double probability = 0.0;
    
    // Base probability from harmonic count
    probability += peak.harmonicCount * 0.2;
    
    // Add harmonic strength contribution
    probability += std::min(peak.harmonicStrength, 0.3);
    
    // Check proximity to standard note frequencies
    double noteFreq = getNearestNoteFrequency(peak.frequency);
    double centsDiff = std::abs(1200 * std::log2(peak.frequency / noteFreq));
    if (centsDiff < 50) { // Within 50 cents
        probability += 0.3 * (1.0 - centsDiff / 50.0);
    }
    
    return std::min(probability, 1.0);
}

Peak* TunerEngine::selectBestPeak(QVector<Peak>& peaks) {
    Peak* bestPeak = nullptr;
    double bestScore = 0;
    
    for (Peak& peak : peaks) {
        // Calculate base score from harmonics
        double score = peak.harmonicCount * 2.0;
        
        // Add probability score
        score += calculateNoteProbability(peak) * 3.0;
        
        // Prefer lower frequencies (fundamental over harmonics)
        score += 1.0 / (1.0 + peak.frequency / 440.0);
        
        // Amplitude contribution (smaller weight)
        score += peak.amplitude * 0.5;
        
        if (!bestPeak || score > bestScore) {
            bestPeak = &peak;
            bestScore = score;
        }
    }
    
    // Only return if we're confident enough
    return (bestScore > 2.0) ? bestPeak : nullptr;
}

double TunerEngine::getStableFrequency(double newFreq, double confidence) {
    if (newFreq == 0) return 0;
    
    // Find if we have this frequency in history
    for (FrequencyHistory& hist : m_frequencyHistory) {
        double centsDiff = std::abs(1200 * std::log2(newFreq / hist.frequency));
        if (centsDiff < 15) { // Within 15 cents
            hist.count++;
            hist.confidence = std::max(hist.confidence, confidence);
            if (hist.count >= 3) { // Need 3 consecutive detections
                return hist.frequency;
            }
            return 0;
        }
    }
    
    // Add new frequency to history
    m_frequencyHistory.append({newFreq, 1, confidence});
    if (m_frequencyHistory.size() > HISTORY_SIZE) {
        m_frequencyHistory.removeFirst();
    }
    return 0;
}

void TunerEngine::setDetectionMethod(const QString &method)
{
    if (m_detectionMethod != method) {
        m_detectionMethod = method;
        emit detectionMethodChanged();
    }
}

void TunerEngine::setFftPadding(int padding)
{
    // Ensure padding is at least 1 and not too large
    padding = std::max(1, std::min(padding, 8));
    
    if (m_fftPadding != padding) {
        m_fftPadding = padding;
        // Resize FFT buffer
        m_fftBuffer.resize(m_bufferSize * m_fftPadding);
        emit fftPaddingChanged();
        
        qDebug() << "FFT padding set to" << padding << "x";
        qDebug() << "New frequency resolution:" 
                 << static_cast<double>(m_sampleRate) / (m_bufferSize * padding) 
                 << "Hz";
    }
} 
