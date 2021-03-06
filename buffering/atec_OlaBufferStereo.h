/*
  
    For now, this is hard-coded to a window size of 4096 and overlap of 4 since those are good general settings for continuous OLA audio.
 
    NOTE:
    - the host blocksize must be less than mHop! you will get clicks otherwise
 
    TODO:
    - add .setRingBufSize() and .setOverlap() methods.
    - add methods for getting juce::AudioBuffer pointers directly, so we can use AudioBuffer methods
    - improve fillOverlapBuf() and outputOlaBlock() methods so they can handle host block sizes greater than or equal to mHop
 
 */

namespace atec
{
    #define OLABUFDEFAULTSIZE 4096
    #define OLABUFDEFAULTOVERLAP 4

    // now that we have the RingBuffer class, this class should inherit from it
    class OlaBufferStereo
    {
    public:
        OlaBufferStereo();
        ~OlaBufferStereo();

        void init();
        void fillRingBuf(juce::AudioBuffer<float>& inBuf);
        void fillOverlapBuf();
        void doWindowing(int channel);
        void outputOlaBlock(juce::AudioBuffer<float>& outBuf);
        void advanceWriteIdx(int blockSize); // this could use a safety check to disallow negative blockSize values
        int getWindowSize();
        int getOverlap();
        bool getProcessFlag(int channel);
        void clearProcessFlag(int channel);
    //    const juce::AudioBuffer<float>* getBufPointerL();
    //    const juce::AudioBuffer<float>* getBufPointerR();
        const float* getReadPointerL(int channel);
        const float* getReadPointerR(int channel);
        float* getWritePointerL(int channel);
        float* getWritePointerR(int channel);
        
    private:

        juce::AudioBuffer<float> mOverlapBufL;
        juce::AudioBuffer<float> mOverlapBufR;
        juce::AudioBuffer<float> mRingBuf;
        // it appears that there is no .resize member function of the WindowingFunction class. so a better solution may be to use an AudioBuffer object and fill it with a home-made Hann window. this will also require a manual element-by-element multiply, since we lose the .multiplyWithWindowingTable member function. perhaps AudioBuffer has a member function for element-by-element multiplication against another buffer?
        juce::dsp::WindowingFunction<float> mHannWindow;

        int mRingBufSize;
        int mRingBufWriteIdx;
        int mOverlap;
        int mHop;
        int mOverlapBufTargetChannel;
        juce::Array<bool> mProcessFlags;
        bool mDebugFlag;
    };
} // namespace atec
