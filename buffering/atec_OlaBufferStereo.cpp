namespace atec
{
OlaBufferStereo::OlaBufferStereo()
{
    mDebugFlag = false;
    mRingBuf.debug(mDebugFlag);

    // this needs to be set properly with setOwnerBlockSize() before init()
    mOwnerBlockSize = 1024;
    mWindowSize = OLABUFDEFAULTSIZE;
    mOverlap = OLABUFDEFAULTOVERLAP;

    // initialize the buffers so there isn't garbage in them
    init();

    if(mDebugFlag)
    {
        std::string post;
        post = "OlaBufferStereo constructor called. mWindowSize: " + std::to_string(mWindowSize) + ", mOverlap: " + std::to_string(mOverlap) + ", mHop: " + std::to_string(mHop);
        DBG(post);
    }
}

OlaBufferStereo::~OlaBufferStereo()
{
    // using smart pointers only, so nothing to delete
    if(mDebugFlag)
        DBG("OlaBufferStereo destructor called");
}

void OlaBufferStereo::debug(bool d)
{
    mDebugFlag = d;
}

// if host block size changes, best to call this and start buffering process over
void OlaBufferStereo::init()
{
    mHop = mWindowSize/(double)mOverlap;
    
    mProcessFlags.resize(mOverlap);

    // make a buffer with mOverlap channels, each one the same size as the ring buffer
    mOverlapBufL.setSize(mOverlap, mWindowSize);
    mOverlapBufR.setSize(mOverlap, mWindowSize);
    
    // always 2 channels for stereo
    // make the RingBuffer size a multiple of the window size for OLA. 2x should be safe
    // we must know mOwnerBlockSize before init(), so init() will be called in prepareToPlay()
    mRingBuf.setSize(2, mWindowSize * 2, mOwnerBlockSize);
    
    mRingBuf.init();
    mOverlapBufL.clear();
    mOverlapBufR.clear();
    mProcessFlags.fill(false);

    mOverlapBufTargetChannel = 0;

    if(mDebugFlag)
    {
        std::string post;
        
        post = "OlaBufferStereo init. mOwnerBlockSize: " + std::to_string(mOwnerBlockSize);
        DBG(post);

        post = "OlaBufferStereo init. mWindowSize: " + std::to_string(mWindowSize) + ", mOverlap: " + std::to_string(mOverlap) + ", mHop: " + std::to_string(mHop);
        DBG(post);
    }
}

void OlaBufferStereo::fillRingBuf(juce::AudioBuffer<float>& inBuf)
{
    // use special writeNoAdvance() method so we can manually advance the RingBuffer write index after all of the OLA work is done
    mRingBuf.writeNoAdvance(inBuf);
//    mRingBuf.write(inBuf);
}

// call this after fillRingBuf()
void OlaBufferStereo::fillOverlapBuf()
{
    int ringBufWriteIdx = mRingBuf.getWriteIdx();
//    int ringBufSize = mRingBuf.getSize();
    // if we hit a window hop boundary by advancing mRingBufWriteIdx on the last iteration, copy the most recent mRingBufSize samples from the ring buffer to the current overlap buffer target channel, then advance mOverlapBufTargetChannel to point to the next channel in mOverlapBuf
    if(ringBufWriteIdx % mHop == 0)
    {
        juce::AudioBuffer<float> ringBufContents;
        ringBufContents.setSize(2, mWindowSize);
        ringBufContents.clear();
        
        mRingBuf.read(ringBufContents, mWindowSize);
        
        for (int stereoChannel = 0; stereoChannel < 2; ++stereoChannel)
        {
            switch(stereoChannel)
            {
                case 0:
                    mOverlapBufL.copyFrom(mOverlapBufTargetChannel, 0, ringBufContents, stereoChannel, 0, mWindowSize);
                    break;
                case 1:
                    mOverlapBufR.copyFrom(mOverlapBufTargetChannel, 0, ringBufContents, stereoChannel, 0, mWindowSize);
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

// TODO: add safety check for size of window == mWindowSize
void OlaBufferStereo::doWindowing(int channel, juce::dsp::WindowingFunction<float>& window)
{
    auto* overlapBufDataL = mOverlapBufL.getWritePointer(channel);
    auto* overlapBufDataR = mOverlapBufR.getWritePointer(channel);

    window.multiplyWithWindowingTable(overlapBufDataL, mWindowSize);
    window.multiplyWithWindowingTable(overlapBufDataR, mWindowSize);
}

void OlaBufferStereo::outputOlaBlock(juce::AudioBuffer<float>& outBuf)
{
    int numChannels = outBuf.getNumChannels();
    int bufSize = outBuf.getNumSamples();

    if(numChannels > 1)
    {
        if(mDebugFlag)
        {
            if(bufSize > mHop)
            {
                std::string post;
                post = "OlaBufferStereo WARNING: host block size must be less than or equal to " + std::to_string(mHop);
                DBG(post);
            }
        }

        for(int stereoChannel = 0; stereoChannel < 2; ++stereoChannel)
        {
            // since we've already buffered to the ring buf by the time this method is called, start with an empty buffer for both channels. otherwise, we'll be adding to what's already in the buffer, creating little echos
            outBuf.clear(stereoChannel, 0, bufSize);

            for(int overlapChannel = 0; overlapChannel < mOverlap; ++overlapChannel)
            {
                int thisChannel, startIdx, ringBufWriteIdx;

                ringBufWriteIdx = mRingBuf.getWriteIdx();
                
                // the upcoming target channel is the oldest, so start the process there
                thisChannel = mOverlapBufTargetChannel + overlapChannel;
                thisChannel = thisChannel % mOverlap;

                // we'll make the startIdx for reading out of the overlap buffer channel track the RingBuffer write index.
                // since the RingBuffer size is set up to be a multiple of the OlaBufferStereo window size, we can just mod by mWindowSize
                startIdx = (ringBufWriteIdx % mWindowSize);
                // by subtracting a multiple of the hop size relative to the given overlap buffer channel we're processing, we grab the correct portion from each overlap buffer channel
                startIdx -= (mHop * thisChannel);
                
                // could go negative, and if so, we should wrap around to the end of this overlap buffer channel
                if(startIdx < 0)
                    startIdx += mWindowSize;

                if(false)
                    std::printf("OlaBufferStereo>> mOverlapBufTargetChannel: %i, thisChannel: %i, startIdx: %i\n", mOverlapBufTargetChannel, thisChannel, startIdx);

                switch(stereoChannel)
                {
                    case 0:
                        // as long as we'll stay in bounds going bufSize samples beyond the startIdx, we can do one call to addFrom() and mix in bufSize samples
                        if(startIdx + bufSize < mWindowSize)
                            outBuf.addFrom(stereoChannel, 0, mOverlapBufL, thisChannel, startIdx, bufSize);
                        else
                        {
                            // otherwise, we need to do two addFrom() calls.
                            int numSamps = mWindowSize - startIdx;
                            
                            // the first adds the samples from startIdx to the end of the overlap buffer channel
                            outBuf.addFrom(stereoChannel, 0, mOverlapBufL, thisChannel, startIdx, numSamps);
                            // the second adds samples from the beginning of the overlap buffer channel to the end of the output buffer
                            outBuf.addFrom(stereoChannel, numSamps, mOverlapBufL, thisChannel, 0, bufSize - numSamps);
                        }
                        break;
                    case 1:
                        if(startIdx + bufSize < mWindowSize)
                            outBuf.addFrom(stereoChannel, 0, mOverlapBufR, thisChannel, startIdx, bufSize);
                        else
                        {
                            int numSamps = mWindowSize - startIdx;

                            outBuf.addFrom(stereoChannel, 0, mOverlapBufR, thisChannel, startIdx, numSamps);
                            outBuf.addFrom(stereoChannel, numSamps, mOverlapBufR, thisChannel, 0, bufSize - numSamps);
                        }
                        break;
                    default:
                        break;
                }
            }

            // reduce gain based on overlap.
            outBuf.applyGain(stereoChannel, 0, bufSize, 1.0f/(double)mOverlap);
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
    init();
}

int OlaBufferStereo::getOverlap()
{
    return mOverlap;
}

void OlaBufferStereo::setOverlap(int o)
{
    mOverlap = o;
    init();
}

int OlaBufferStereo::getOwnerBlockSize()
{
    return mOwnerBlockSize;
}

void OlaBufferStereo::setOwnerBlockSize(int N)
{
    mOwnerBlockSize = N;
    init();
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

const juce::AudioBuffer<float>& OlaBufferStereo::getBufRefL()
{
    return mOverlapBufL;
}

const juce::AudioBuffer<float>& OlaBufferStereo::getBufRefR()
{
    return mOverlapBufR;
}

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
