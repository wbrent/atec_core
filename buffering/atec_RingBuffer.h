/*

 */

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

        void debug(bool d);
        void init();
        void write(juce::AudioBuffer<float>& inBuf);
        // overload write() method so we can write to a specific channel
        void write(int channel, juce::AudioBuffer<float>& inBuf);
        void read(juce::AudioBuffer<float>& outBuf, int delaySamps);
        // overload read() method so we can read from a specific channel
        void read(int channel, juce::AudioBuffer<float>& outBuf, int delaySamps);
        void readInterp(juce::AudioBuffer<float>& outBuf, double delaySamps);
        double readInterpSamp(int channel, int samp, double delaySamps);
        void advanceWriteIdx(int blockSize); // this could use a safety check to disallow negative blockSize values
        void setOwnerBlockSize(int n);
        void setSize(int numChan, int numSamps, int ownerBlockSize);
        int getSize();
        const float* getReadPointer(int channel);
        float* getWritePointer(int channel);

    private:

        juce::AudioBuffer<float> mBuffer;
        int mOwnerBlockSize;
        int mBufSize;
        int mNumChan;
        int mWriteIdx;
        bool mDebugFlag;

    };
} // namespace atec
