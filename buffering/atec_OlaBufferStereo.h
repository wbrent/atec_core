/*
  
    For now, this is hard-coded to a window size of 4096 and overlap of 4 since those are good general settings for continuous OLA audio.
 
    NOTE:
    - the host blocksize must be less than mHop! you will get clicks otherwise
 
    TODO:
    - add .setRingBufSize() and .setOverlap() methods.
    - add methods for getting juce::AudioBuffer pointers directly, so we can use AudioBuffer methods
    - improve fillOverlapBuf() and outputOlaBlock() methods so they can handle host block sizes greater than or equal to mHop
 
 */

#include "atec_RingBuffer.h"

namespace atec
{
    #define OLABUFDEFAULTSIZE 4096
    #define OLABUFDEFAULTOVERLAP 4

    class OlaBufferStereo
    {
    public:
        OlaBufferStereo();
        ~OlaBufferStereo();

        void debug(bool d);
        void init();
        void fillRingBuf(juce::AudioBuffer<float>& inBuf);
        void fillOverlapBuf();
        void doWindowing(int channel, juce::dsp::WindowingFunction<float>& window);
        void outputOlaBlock(juce::AudioBuffer<float>& outBuf);
        void advanceWriteIdx(int blockSize); // this could use a safety check to disallow negative blockSize values
        int getWindowSize();
        void setWindowSize(int N);
        int getOverlap();
        void setOverlap(int o);
        int getOwnerBlockSize();
        void setOwnerBlockSize(int N);
        bool getProcessFlag(int channel);
        void clearProcessFlag(int channel);
        const juce::AudioBuffer<float>& getBufRefL();
        const juce::AudioBuffer<float>& getBufRefR();
        const atec::RingBuffer& getRingBufRef();
        const float* getReadPointerL(int channel);
        const float* getReadPointerR(int channel);
        float* getWritePointerL(int channel);
        float* getWritePointerR(int channel);
        
    private:

        juce::AudioBuffer<float> mOverlapBufL;
        juce::AudioBuffer<float> mOverlapBufR;
        RingBuffer mRingBuf;

        int mOwnerBlockSize;
        int mWindowSize;
        int mOverlap;
        int mHop;
        int mOverlapBufTargetChannel;
        juce::Array<bool> mProcessFlags;
        bool mDebugFlag;
    };
} // namespace atec
