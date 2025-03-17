#ifndef TUNERENGINE_H
#define TUNERENGINE_H

#include <QObject>
#include <QAudioInput>
#include <QMediaDevices>
#include <QAudioSource>
#include <QByteArray>
#include <QVector>
#include <QQueue>
#include <complex>
#include <QVariantList>

class QAudioSource;
class QIODevice;

struct FrequencyHistory {
    double frequency;
    int count;
    double confidence;
};

struct Peak {
    double frequency;
    double amplitude;
    int harmonicCount;
    double harmonicStrength;
};

class TunerEngine : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString currentNote READ currentNote NOTIFY noteChanged)
    Q_PROPERTY(double frequency READ frequency NOTIFY frequencyChanged)
    Q_PROPERTY(double cents READ cents NOTIFY centsChanged)
    Q_PROPERTY(double signalLevel READ signalLevel NOTIFY signalLevelChanged)
    Q_PROPERTY(double dbThreshold READ dbThreshold WRITE setDbThreshold NOTIFY dbThresholdChanged)
    Q_PROPERTY(QVariantList peaks READ peaks NOTIFY peaksChanged)
    Q_PROPERTY(int sampleRate READ sampleRate WRITE setSampleRate NOTIFY sampleRateChanged)
    Q_PROPERTY(int bufferSize READ bufferSize WRITE setBufferSize NOTIFY bufferSizeChanged)
    Q_PROPERTY(int maximumSampleRate READ maximumSampleRate NOTIFY maximumSampleRateChanged)
    Q_PROPERTY(int maxPeaks READ maxPeaks WRITE setMaxPeaks NOTIFY maxPeaksChanged)
    Q_PROPERTY(double referenceA READ referenceA WRITE setReferenceA NOTIFY referenceAChanged)
    Q_PROPERTY(QString detectionMethod READ detectionMethod WRITE setDetectionMethod NOTIFY detectionMethodChanged)
    Q_PROPERTY(int fftPadding READ fftPadding WRITE setFftPadding NOTIFY fftPaddingChanged)

public:
    explicit TunerEngine(QObject *parent = nullptr);
    ~TunerEngine();

    void start();
    void stop();

    // Add reload function
    Q_INVOKABLE void reload() {
        stop();
        setupAudioInput();
        start();
    }

    // Property accessors
    QString currentNote() const { return m_currentNote; }
    double frequency() const { return m_frequency; }
    double cents() const { return m_cents; }
    double signalLevel() const { return m_signalLevel; }
    double dbThreshold() const { return m_dbThreshold; }
    void setDbThreshold(double threshold);
    QVariantList peaks() const { return m_peaks; }
    int sampleRate() const { return m_sampleRate; }
    void setSampleRate(int rate);
    int bufferSize() const { return m_bufferSize; }
    void setBufferSize(int size);
    int maximumSampleRate() const { return m_maximumSampleRate; }
    int maxPeaks() const { return m_maxPeaks; }
    void setMaxPeaks(int peaks);
    double referenceA() const { return m_referenceA; }
    void setReferenceA(double freq);
    QString detectionMethod() const { return m_detectionMethod; }
    void setDetectionMethod(const QString &method);
    int fftPadding() const { return m_fftPadding; }
    void setFftPadding(int padding);

signals:
    void noteChanged();
    void frequencyChanged();
    void centsChanged();
    void signalLevelChanged();
    void dbThresholdChanged();
    void peaksChanged();
    void noteDetected(const QString &note, double frequency, double cents);
    void signalLevel(double dbFS);
    void sampleRateChanged();
    void bufferSizeChanged();
    void maximumSampleRateChanged();
    void maxPeaksChanged();
    void referenceAChanged();
    void detectionMethodChanged();
    void fftPaddingChanged();

private slots:
    void processAudioInput();

private:
    static constexpr int DEFAULT_SAMPLE_RATE = 48000;
    static constexpr int DEFAULT_BUFFER_SIZE = 8112;
    static constexpr double DEFAULT_A4_FREQUENCY = 440.0;
    static constexpr int DEFAULT_MAX_PEAKS = 10;
    static constexpr int DEFAULT_FFT_PADDING = 2;  // Default 2x padding

    QAudioSource* m_audioSource;
    QIODevice* m_audioDevice;
    QByteArray m_buffer;
    QByteArray m_accumulationBuffer;

    // Property storage
    QString m_currentNote;
    double m_frequency = 0.0;
    double m_cents = 0.0;
    double m_signalLevel = -90.0;
    double m_dbThreshold = -70.0;
    QVariantList m_peaks;
    int m_sampleRate = DEFAULT_SAMPLE_RATE;
    int m_bufferSize = DEFAULT_BUFFER_SIZE;
    int m_maximumSampleRate = DEFAULT_SAMPLE_RATE;
    int m_maxPeaks = DEFAULT_MAX_PEAKS;
    double m_referenceA = DEFAULT_A4_FREQUENCY;
    QString m_detectionMethod = "FFT";
    int m_fftPadding = DEFAULT_FFT_PADDING;

    double detectFrequencyAutocorrelation(const QVector<double>& samples);
    double detectFrequencyFFT(const QVector<double>& samples);
    QString frequencyToNote(double frequency, double& cents);
    void setupAudioInput();
    void processAccumulatedData();
    double calculateDBFS(const QVector<double>& samples);
    void updatePeaks(const QVector<Peak>& peaks);

    QVector<std::complex<double>> m_fftBuffer;
    void applyHannWindow(QVector<double>& samples);
    void performFFT(QVector<std::complex<double>>& data);

    void updateMaximumSampleRate();

    void analyzeHarmonics(Peak& fundamental, const QVector<Peak>& peaks);
    double calculateNoteProbability(const Peak& peak) const;
    Peak* selectBestPeak(QVector<Peak>& peaks);
    double getStableFrequency(double newFreq, double confidence);
    double getNearestNoteFrequency(double frequency) const;
    
    QVector<FrequencyHistory> m_frequencyHistory;
    static constexpr int HISTORY_SIZE = 5;

};

#endif // TUNERENGINE_H 
