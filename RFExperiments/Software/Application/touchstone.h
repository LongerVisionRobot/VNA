#ifndef TOUCHSTONE_H
#define TOUCHSTONE_H

#include <complex>
#include <vector>
#include <string>

class Touchstone
{
public:
    enum class Unit {
        Hz,
        kHz,
        MHz,
        GHz,
    };

    enum class Format {
        DBAngle,
        MagnitudeAngle,
        RealImaginary,
    };

    class Datapoint {
    public:
        double frequency;
        std::vector<std::complex<double>> S;
    };

    Touchstone(unsigned int m_ports);
    void AddDatapoint(Datapoint p);
    void toFile(std::string filename, Unit unit = Unit::GHz, Format format = Format::RealImaginary);
    static Touchstone fromFile(std::string filename);
    double minFreq();
    double maxFreq();
    unsigned int points() { return m_datapoints.size(); };
    // remove all paramaters except the ones regarding port1 and port2 (port cnt starts at 0)
    void reduceTo2Port(unsigned int port1, unsigned int port2);
    // remove all paramaters except the ones from port (port cnt starts at 0)
    void reduceTo1Port(unsigned int port);
    unsigned int ports() { return m_ports; }
private:
    unsigned int m_ports;
    std::vector<Datapoint> m_datapoints;
};

#endif // TOUCHSTONE_H
