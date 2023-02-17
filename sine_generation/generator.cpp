#include "../waveform.hpp"
#include <cmath>
#include <numbers>

const wav::WAVHeader mono_16bit = {
    { 'R', 'I', 'F', 'F' },
    0,
    { 'W', 'A', 'V', 'E' },
    { 'f', 'm', 't', ' ' },
    16,
    1,
    1,
    44100,
    88200,
    2,
    16,
    { 'd', 'a', 't', 'a' },
    0
};

using WAVData = std::vector<std::int16_t>;

float calc_amplitude(int16_t sample)
{
    return (sample < 0 ? (float)sample / -INT16_MIN : (float)sample / INT16_MAX);
}

class SineGenerator {
private:
    wav::Waveform _filedata;

    // parameters
    const double amplitude, frequency, phase, duration;

public:
    /**
     * @brief Construct a new (WAVE) Sine Generator object
     *
     * @param header the header format to use
     * @param a maximum amplitude of the generated sine wave [-1, 1]
     * @param f the frequency of the generated wave, in hertz
     * @param phi starting phase of the sine wave (phi_0), in radians
     * @param dur duration of the sine wave, in seconds
     */
    SineGenerator(wav::WAVHeader header, double a, double f, double phi, double dur)
        : amplitude(a)
        , frequency(f)
        , phase(phi)
        , duration(dur)
    {
        _filedata.header() = header;
    }

    void generate()
    {
        _filedata.data().clear();

        double w = 2 * std::numbers::pi_v<double> * frequency;
        double sample_rate = _filedata.header().sample_rate;

        std::size_t num_samples = static_cast<size_t>(sample_rate * duration);

        for (std::size_t i = 0; i < num_samples; i++) {
            double t = static_cast<double>(i) / sample_rate;
            double x = amplitude * std::sin(w * t + phase);

            std::int16_t sample = static_cast<std::int16_t>(x * std::numeric_limits<std::int16_t>::max());
            _filedata.data().push_back(sample);
        }

        std::cout << _filedata.data().size() << " samples generated..." << std::endl;
        update_header();
    }

    void declick()
    {
        declick_in();
        declick_out();

        update_header();
    }

    void save(const std::string& filename = "out.wav")
    {
        _filedata.save(filename);
    }

private:
    void update_header()
    {
        double sample_rate = _filedata.header().sample_rate;
        std::size_t num_samples = static_cast<size_t>(sample_rate * duration);
        _filedata.header().subchunk2_size = num_samples * sizeof(std::int16_t) * _filedata.header().num_channels;
    }

    void declick_in(float threshold = 0.01f)
    {
        while (calc_amplitude(_filedata.data().at(0)) >= threshold) {
            _filedata.data().erase(_filedata.data().cbegin());
        }
    }

    void declick_out(float threshold = 0.01f)
    {
        while (calc_amplitude(_filedata.data().at(_filedata.data().size() - 1)) >= threshold)
            _filedata.data().pop_back();
    }
};

int main()
{
    // SineGenerator sg(mono_16bit, 0.5, 750, 0, 3);
    SineGenerator sg = SineGenerator(mono_16bit, 0.66, 240, std::numbers::pi_v<double> / 2, 1);
    sg.generate();
    sg.declick();
    sg.save("generated_sine.wav");

    wav::Waveform sine("generated_sine.wav");
    sine.filter(demo::filters::Gain<short>(2));
    sine.filter(demo::filters::Clip<short>(INT16_MAX / 2));
    sine.save("modulated_sine.wav");

    return EXIT_SUCCESS;
}