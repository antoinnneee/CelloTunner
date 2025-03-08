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

struct Peak {
    double frequency;
    double amplitude;
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

public:
    explicit TunerEngine(QObject *parent = nullptr);
    ~TunerEngine();

    void start();
    void stop();

    // Property accessors
    QString currentNote() const { return m_currentNote; }
    double frequency() const { return m_frequency; }
    double cents() const { return m_cents; }
    double signalLevel() const { return m_signalLevel; }
    double dbThreshold() const { return m_dbThreshold; }
    QVariantList peaks() const { return m_peaks; }
    void setDbThreshold(double threshold);

signals:
    void noteChanged();
    void frequencyChanged();
    void centsChanged();
    void signalLevelChanged();
    void dbThresholdChanged();
    void peaksChanged();
    void noteDetected(const QString &note, double frequency, double cents);
    void signalLevel(double dbFS);

private slots:
    void processAudioInput();

private:
    static constexpr int SAMPLE_RATE = 48000;
    static constexpr int BUFFER_SIZE = 4096;
    static constexpr double A4_FREQUENCY = 440.0;
    static constexpr int HISTORY_SIZE = 1;
    static constexpr int OUTLIERS_TO_REMOVE = 0;
    static constexpr int MAX_PEAKS = 10;

    QAudioSource* m_audioSource;
    QIODevice* m_audioDevice;
    QByteArray m_buffer;
    QByteArray m_accumulationBuffer;

    // Property storage
    QString m_currentNote;
    double m_frequency = 0.0;
    double m_cents = 0.0;
    double m_signalLevel = -90.0;
    double m_dbThreshold = -50.0;
    QVariantList m_peaks;

    // Frequency history
    QQueue<double> m_frequencyHistory;
    double calculateMedianFrequency(double newFrequency);

    double detectFrequency(const QVector<double>& samples);
    QString frequencyToNote(double frequency, double& cents);
    void setupAudioInput();
    void processAccumulatedData();
    double calculateDBFS(const QVector<double>& samples);
    void updatePeaks(const QVector<Peak>& peaks);

    QVector<std::complex<double>> m_fftBuffer;
};

#endif // TUNERENGINE_H 
