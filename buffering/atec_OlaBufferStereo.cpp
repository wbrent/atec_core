namespace atec
{
// the WindowingFunction class has no default constructor, so initializion list creates an object for a Hann window of default size. note that normalization arg should be FALSE
OlaBufferStereo::OlaBufferStereo() : mHannWindow(OLABUFDEFAULTSIZE, juce::dsp::WindowingFunction<float>::hann, false)
{
    mDebugFlag = true;
    mRingBuf.debug(mDebugFlag);

    // this needs to be set properly with setOwnerBlockSize() before init()
    mOwnerBlockSize = 1024;
    mWindowSize = OLABUFDEFAULTSIZE;
    mOverlap = OLABUFDEFAULTOVERLAP;
    mHop = mWindowSize/(double)mOverlap;
    
    mProcessFlags.resize(mOverlap);

    // make a buffer with mOverlap channels, each one the same size as the ring buffer
    mOverlapBufL.setSize(mOverlap, mWindowSize);
    mOverlapBufR.setSize(mOverlap, mWindowSize);

    // initialize the buffers so there isn't garbage in them
    init();

    if(mDebugFlag)
    {
        std::string post;
        post = "OlaBufferStereo constructor called. mBufSize: " + std::to_string(mWindowSize) + ", mOverlap: " + std::to_string(mOverlap) + ", mHop: " + std::to_string(mHop);
        DBG(post);
    }
}

OlaBufferStereo::~OlaBufferStereo()
{
    // using smart pointers only, so nothing to delete
    if(mDebugFlag)
        DBG("OlaBufferStereo destructor called");
}

// if host block size changes, best to call this and start buffering process over
void OlaBufferStereo::init()
{
    // always 2 channels for stereo
    // make the RingBuffer twice as big as our window size for OLA? or can it just be the same as the window size?
    // we must know mOwnerBlockSize before init(), so init() will be called in prepareToPlay()
    mRingBuf.setSize(2, mWindowSize, mOwnerBlockSize);
    
    mRingBuf.init();
    mOverlapBufL.clear();
    mOverlapBufR.clear();
    mProcessFlags.fill(false);

    mOverlapBufTargetChannel = 0;
}

void OlaBufferStereo::fillRingBuf(juce::AudioBuffer<float>& inBuf)
{
    // use special writeNoAdvance() method so we can manually advance the RingBuffer write index after all of the OLA work is done
    mRingBuf.writeNoAdvance(inBuf);
}

// call this after fillRingBuf()
void OlaBufferStereo::fillOverlapBuf()
{
    int ringBufWriteIdx = mRingBuf.getWriteIdx();
//    int ringBufSize = mRingBuf.getSize();
    // if we hit a window hop boundary by advancing mRingBufWriteIdx on the last iteration, copy the most recent mRingBufSize samples from the ring buffer to the current overlap buffer target channel, then advance mOverlapBufTargetChannel to point to the next channel in mOverlapBuf
    if(ringBufWriteIdx % mHop == 0)
    {
        for (int stereoChannel=0; stereoChannel<2; ++stereoChannel)
        {
            switch(stereoChannel)
            {
                    // we'll copy from mRingBuf starting at the current ringBufWriteIdx to the end of the ring buffer, and put that oldest chunk of audio at the head of the overlap buf target channel
                    // this must be followed up by another copy from the head of the ring buffer, written starting where we left off in the overlap buf target channel
                case 0:
                    mOverlapBufL.copyFrom(mOverlapBufTargetChannel, 0, mRingBuf.getBufRef(), stereoChannel, ringBufWriteIdx, mWindowSize - ringBufWriteIdx);
                    mOverlapBufL.copyFrom(mOverlapBufTargetChannel, mWindowSize - ringBufWriteIdx, mRingBuf.getBufRef(), stereoChannel, 0, ringBufWriteIdx);
                    break;
                case 1:
                    mOverlapBufR.copyFrom(mOverlapBufTargetChannel, 0, mRingBuf.getBufRef(), stereoChannel, ringBufWriteIdx, mWindowSize - ringBufWriteIdx);
                    mOverlapBufR.copyFrom(mOverlapBufTargetChannel, mWindowSize - ringBufWriteIdx, mRingBuf.getBufRef(), stereoChannel, 0, ringBufWriteIdx);
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

    mHannWindow.multiplyWithWindowingTable(overlapBufDataL, mWindowSize);
    mHannWindow.multiplyWithWindowingTable(overlapBufDataR, mWindowSize);
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
            {
                std::string post;
                post = "OlaBufferStereo WARNING: host block size must be less than " + std::to_string(mHop);
                DBG(post);
            }
        }

        for(int stereoChannel=0; stereoChannel<2; ++stereoChannel)
        {

            // since we've already buffered to the ring buf by the time this method is called, start with an empty buffer for both channels. otherwise, we'll be adding to what's already in the buffer, creating little echos
            outBuf.clear(stereoChannel, 0, n);

            for(int channel=0; channel<mOverlap; ++channel)
            {
                int thisChannel, startIdx, ringBufWriteIdx;

                ringBufWriteIdx = mRingBuf.getWriteIdx();
                
                // the upcoming target channel is the oldest, so start the process there
                thisChannel = mOverlapBufTargetChannel+channel;
                thisChannel = thisChannel%mOverlap;

                startIdx = ringBufWriteIdx - (mHop*thisChannel);
                if(startIdx<0)
                    startIdx += mWindowSize;

                if(false)
                    std::printf("OlaBufferStereo>> mOverlapBufTargetChannel: %i, thisChannel: %i, startIdx: %i\n", mOverlapBufTargetChannel, thisChannel, startIdx);

                switch(stereoChannel)
                {
                    case 0:
                        if(startIdx+n < mWindowSize)
                            outBuf.addFrom(stereoChannel, 0, mOverlapBufL, thisChannel, startIdx, n);
                        else
                        {
                            int numSamps = mWindowSize-startIdx;
                            outBuf.addFrom(stereoChannel, 0, mOverlapBufL, thisChannel, startIdx, numSamps);
                            outBuf.addFrom(stereoChannel, numSamps, mOverlapBufL, thisChannel, 0, n-numSamps);
                        }
                        break;
                    case 1:
                        if(startIdx+n < mWindowSize)
                            outBuf.addFrom(stereoChannel, 0, mOverlapBufR, thisChannel, startIdx, n);
                        else
                        {
                            int numSamps = mWindowSize-startIdx;
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
    mRingBuf.advanceWriteIdx(n);
}

int OlaBufferStereo::getWindowSize()
{
    return mWindowSize;
}

void OlaBufferStereo::setWindowSize(int N)
{
    mWindowSize = N;
}

int OlaBufferStereo::getOverlap()
{
    return mOverlap;
}

void OlaBufferStereo::setOverlap(int o)
{
    mOverlap = o;
}

int OlaBufferStereo::getOwnerBlockSize()
{
    return mOwnerBlockSize;
}

void OlaBufferStereo::setOwnerBlockSize(int N)
{
    mOwnerBlockSize = N;
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
