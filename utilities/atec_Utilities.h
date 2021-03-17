/*

    Description

*/

namespace atec
{
    class Utilities
    {
    public:

        Utilities();
        ~Utilities();
        
        // since there are no member variables to this class, the function definitions won't change. thus, we can make all function declarations static here in the class definition and invoke them via Utilties::funcName()
        static bool isEven(double value);
        static int getSign(double value);
        static double expToLinear(double value, double power, juce::Range<double> range);
        static double linearToExp(double value, double power, juce::Range<double> range);
        static double sec2samp(double sec, double sampleRate);
        static double samp2sec(double samp, double sampleRate);
        static double freq2midi(double f);
        static double midi2freq(double m);
        static double transpo2freq(double transpo, double windowSizeMs);

        static double cubicInterpolate(double y0, double y1, double y2, double y3, double mu);
        static double bufReadInterp(int channel, double readIdx, juce::AudioBuffer<float>& buffer);
        
        static void arrayShuffle(juce::Array<int>& seq);
        
        static int getZeroCrossingPoints(int channel, juce::AudioBuffer<float>& buffer, juce::Array<int>& xIndices, double threshDb);
        static double getZeroCrossingRate(int channel, juce::AudioBuffer<float>& buffer);
        static void getFftPowerSpec(juce::dsp::Complex<float>* inBuf, juce::Array<double>& outBuf, int N);
        static void getFftMagSpec(juce::dsp::Complex<float>* inBuf, juce::Array<double>& outBuf, int N);
        static void getFftPhaseSpec(juce::dsp::Complex<float>* buffer, juce::Array<double>& outBuf, int N);
        static void fftZeroPhase(juce::dsp::Complex<float>* buffer, int N);
        static void fftApplyFilter(juce::dsp::Complex<float>* inBuf, juce::Array<double> filter, int N);

    private:

    };
} // namespace atec
