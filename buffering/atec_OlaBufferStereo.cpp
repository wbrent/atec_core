namespace atec
{
// the WindowingFunction class has no default constructor, so initializion list creates an object for a Hann window of default size. note that normalization arg should be FALSE
OlaBufferStereo::OlaBufferStereo() : mHannWindow(OLABUFDEFAULTSIZE, juce::dsp::WindowingFunction<float>::hann, false)
{
    mDebugFlag = false;

    mRingBufSize = OLABUFDEFAULTSIZE;
    mOverlap = OLABUFDEFAULTOVERLAP;
    mHop = mRingBufSize/(double)mOverlap;

    mRingBuf.setSize(2, mRingBufSize);
    mProcessFlags.resize(mOverlap);

    // make a buffer with mOverlap channels, each one the same size as the ring buffer
    mOverlapBufL.setSize(mOverlap, mRingBufSize);
    mOverlapBufR.setSize(mOverlap, mRingBufSize);

    // initialize the buffers so there isn't garbage in them
    init();

    printf("OlaBufferStereo constructor called. mRingBufSize: %i, mOverlap: %i, mHop: %i\n", mRingBufSize, mOverlap, mHop);
}

OlaBufferStereo::~OlaBufferStereo()
{
    // using smart pointers only, so nothing to delete
    printf("OlaBufferStereo destructor called\n");
}

// if host block size changes, best to call this and start buffering process over
void OlaBufferStereo::init()
{
    mRingBuf.clear();
    mOverlapBufL.clear();
    mOverlapBufR.clear();
    mProcessFlags.fill(false);

    mRingBufWriteIdx = 0;
    mOverlapBufTargetChannel = 0;
}

void OlaBufferStereo::fillRingBuf(juce::AudioBuffer<float>& inBuf)
{
    auto n = inBuf.getNumSamples();
    auto numChannels = inBuf.getNumChannels();

    if(numChannels > 1)
    {
        // copy a block from the host into our ring buffer starting at mRingBufWriteIdx
        // if inBuf has more than 2 channels, ignore them
        mRingBuf.copyFrom(0, mRingBufWriteIdx, inBuf, 0, 0, n);
        mRingBuf.copyFrom(1, mRingBufWriteIdx, inBuf, 1, 0, n);
    }
}

// call this after fillRingBuf()
void OlaBufferStereo::fillOverlapBuf()
{
    // if we hit a window hop boundary by advancing mRingBufWriteIdx on the last iteration, copy the most recent mRingBufSize samples from the ring buffer to the current overlap buffer target channel, then advance mOverlapBufTargetChannel to point to the next channel in mOverlapBuf
    if(mRingBufWriteIdx%mHop == 0)
    {
        for (int stereoChannel=0; stereoChannel<2; ++stereoChannel)
        {
            switch(stereoChannel)
            {
                    // we'll copy from mRingBuf starting at the current ringBufWriteIdx to the end of the ring buffer, and put that oldest chunk of audio at the head of the overlap buf target channel
                    // this must be followed up by another copy from the head of the ring buffer, written starting where we left off in the overlap buf target channel
                case 0:
                    mOverlapBufL.copyFrom(mOverlapBufTargetChannel, 0, mRingBuf, stereoChannel, mRingBufWriteIdx, mRingBufSize-mRingBufWriteIdx);
                    mOverlapBufL.copyFrom(mOverlapBufTargetChannel, mRingBufSize-mRingBufWriteIdx, mRingBuf, stereoChannel, 0, mRingBufWriteIdx);
                    break;
                case 1:
                    mOverlapBufR.copyFrom(mOverlapBufTargetChannel, 0, mRingBuf, stereoChannel, mRingBufWriteIdx, mRingBufSize-mRingBufWriteIdx);
                    mOverlapBufR.copyFrom(mOverlapBufTargetChannel, mRingBufSize-mRingBufWriteIdx, mRingBuf, stereoChannel, 0, mRingBufWriteIdx);
                    break;
                default:
                    break;
            }
        }

        // turn on the flag to indicate this overlap channel is ready for processing
        mProcessFlags.set(mOverlapBufTargetChannel, true);

        // advance the target channel for next time
        mOverlapBufTargetChannel++;
        mOverlapBufTargetChannel = mOverlapBufTargetChannel % mOverlap;
    }
}

void OlaBufferStereo::doWindowing(int channel)
{
    auto* overlapBufDataL = mOverlapBufL.getWritePointer(channel);
    auto* overlapBufDataR = mOverlapBufR.getWritePointer(channel);

    mHannWindow.multiplyWithWindowingTable(overlapBufDataL, mRingBufSize);
    mHannWindow.multiplyWithWindowingTable(overlapBufDataR, mRingBufSize);
}

// WB: this needs to be inspected again...
void OlaBufferStereo::outputOlaBlock(juce::AudioBuffer<float>& outBuf)
{
    int numChannels = outBuf.getNumChannels();
    int n = outBuf.getNumSamples();

    if(numChannels>1)
    {
        if(mDebugFlag)
        {
            if(n >= mHop)
                std::printf("OlaBufferStereo WARNING: host block size must be less than %i\n", mHop);
        }

        for(int stereoChannel=0; stereoChannel<2; ++stereoChannel)
        {

            // since we've already buffered to the ring buf by the time this method is called, start with an empty buffer for both channels. otherwise, we'll be adding to what's already in the buffer, creating little echos
            outBuf.clear(stereoChannel, 0, n);

            for(int channel=0; channel<mOverlap; ++channel)
            {
                int thisChannel, startIdx;

                // the upcoming target channel is the oldest, so start the process there
                thisChannel = mOverlapBufTargetChannel+channel;
                thisChannel = thisChannel%mOverlap;

                startIdx = mRingBufWriteIdx - (mHop*thisChannel);
                if(startIdx<0)
                    startIdx += mRingBufSize;

                if(false)
                    std::printf("OlaBufferStereo>> mOverlapBufTargetChannel: %i, thisChannel: %i, startIdx: %i\n", mOverlapBufTargetChannel, thisChannel, startIdx);

                switch(stereoChannel)
                {
                    case 0:
                        if(startIdx+n < mRingBufSize)
                            outBuf.addFrom(stereoChannel, 0, mOverlapBufL, thisChannel, startIdx, n);
                        else
                        {
                            int numSamps = mRingBufSize-startIdx;
                            outBuf.addFrom(stereoChannel, 0, mOverlapBufL, thisChannel, startIdx, numSamps);
                            outBuf.addFrom(stereoChannel, numSamps, mOverlapBufL, thisChannel, 0, n-numSamps);
                        }
                        break;
                    case 1:
                        if(startIdx+n < mRingBufSize)
                            outBuf.addFrom(stereoChannel, 0, mOverlapBufR, thisChannel, startIdx, n);
                        else
                        {
                            int numSamps = mRingBufSize-startIdx;
                            outBuf.addFrom(stereoChannel, 0, mOverlapBufR, thisChannel, startIdx, numSamps);
                            outBuf.addFrom(stereoChannel, numSamps, mOverlapBufR, thisChannel, 0, n-numSamps);
                        }
                        break;
                    default:
                        break;
                }
            }

            // reduce gain based on overlap.
            outBuf.applyGain(stereoChannel, 0, n, 1.0f/(double)mOverlap);
        }
    }
}

// call this at the end of a block
void OlaBufferStereo::advanceWriteIdx(int n)
{
    // advance mRingBufWriteIdx at the end of the processBlock call
    mRingBufWriteIdx += n;
    // wrap at mRingBufSize
    mRingBufWriteIdx = mRingBufWriteIdx % mRingBufSize;
}

int OlaBufferStereo::getWindowSize()
{
    int n;

    n = mRingBufSize;

    return n;
}

int OlaBufferStereo::getOverlap()
{
    int overlap;

    overlap = mOverlap;

    return overlap;
}

bool OlaBufferStereo::getProcessFlag(int channel)
{
    bool state;

    state = mProcessFlags[channel];

    return state;
}

void OlaBufferStereo::clearProcessFlag(int channel)
{
    mProcessFlags.set(channel, false);
}

//const juce::AudioBuffer<float>* getBufPointerL()
//{
//    const juce::AudioBuffer<float>* ptr;
//
//    ptr = &mOverlapBufL;
//}
//
//const juce::AudioBuffer<float>* getBufPointerR()
//{
//    const juce::AudioBuffer<float>* ptr;
//
//    ptr = &mOverlapBufR;
//}

const float* OlaBufferStereo::getReadPointerL(int channel)
{
    const float* ptr;

    ptr = mOverlapBufL.getReadPointer(channel);

    return ptr;
}

const float* OlaBufferStereo::getReadPointerR(int channel)
{
    const float* ptr;

    ptr = mOverlapBufR.getReadPointer(channel);

    return ptr;
}

float* OlaBufferStereo::getWritePointerL(int channel)
{
    float* ptr;

    ptr = mOverlapBufL.getWritePointer(channel);

    return ptr;
}

float* OlaBufferStereo::getWritePointerR(int channel)
{
    float* ptr;

    ptr = mOverlapBufR.getWritePointer(channel);

    return ptr;
}
} //namespace atec
