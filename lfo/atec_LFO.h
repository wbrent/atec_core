/*

    Description

*/

namespace atec
{
    #define NUMLFOTYPES 5

    class LFO
    {
    public:
        enum LfoType {sin, cos, square, saw, triangle};

        LFO();
        ~LFO();

        void init();
        LfoType getType();
        void setType (LfoType t);
        double getFreq();
        void setFreq(double f);
        juce::Range<double> getRange();
        void setRange(juce::Range<double> r);
        double getPhaseDelta();
        double getPhase();
        void setPhase(double p);
        double getSampleRate();
        void setSampleRate(double sampleRate);
        double getNextSample();
        
    private:
        LfoType mType;
        double mFreq;
        juce::Range<double> mRange;
        double mPhaseAngle;
        double mPhaseDelta;
        double mSampleRate;
        
        void calcPhaseDelta();
    };
} // namespace atec

