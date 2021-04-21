/*

 */

#ifndef RING_BUFFER_H
#define RING_BUFFER_H

namespace atec
{
    #define RINGBUFDEFAULTOWNERBLOCKSIZE 1024
    #define RINGBUFDEFAULTSIZE 32768
    #define RINGBUFDEFAULTCHAN 2

    class RingBuffer
    {
    public:
        RingBuffer();
        ~RingBuffer();

        // TODO: too many overloaded functions here. need to pick a design and commit to it
        void debug(bool d);
        void init();
        void write(juce::AudioBuffer<float>& inBuf, bool advance = true);
        // overload write() method so we can write to a specific channel
        void write(int destChannel, juce::AudioBuffer<float>& sourceBuf, int sourceChannel, int numSamps, bool advance = true);
        void writeSample(int channel, int writeIdx, float sample);
        
        void read(juce::AudioBuffer<float>& destBuf);
        void read(juce::AudioBuffer<float>& destBuf, int delaySamps);
        // overload read() method so we can read from a specific channel
        void read(int sourceChannel, int delaySamps, juce::AudioBuffer<float>& destBuf, int destChannel, int numSamps);
        void readUnsafe(int sourceChannel, int delaySamps, juce::AudioBuffer<float>& destBuf, int destChannel, int numSamps);

        void readInterp(juce::AudioBuffer<float>& destBuf, double delaySamps);
        void readInterp(int sourceChannel, double delaySamps, juce::AudioBuffer<float>& destBuf, int destChannel, int numSamps);
        double readInterpSample(int channel, int samp, double delaySamps);
        double readInterpSample(int channel, double sampInc, double* lastReadIdx);

        int getWriteIdx();
        void advanceWriteIdx(int blockSize);
        int getOwnerBlockSize();
        void setOwnerBlockSize(int N);
        int getSize();
        void setSize(int numChan, int numSamps, int ownerBlockSize);
        const float* getReadPointer(int channel);
        float* getWritePointer(int channel);
        const juce::AudioBuffer<float>& getBufRef();

    private:

        juce::AudioBuffer<float> mBuffer;
        int mOwnerBlockSize;
        int mBufSize;
        int mNumChan;
        int mWriteIdx;
        bool mDebugFlag;

    };
} // namespace atec

#endif
