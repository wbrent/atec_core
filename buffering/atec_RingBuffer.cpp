namespace atec
{
RingBuffer::RingBuffer()
{
    mDebugFlag = false;

    mBufSize = RINGBUFDEFAULTSIZE;
    mNumChan = RINGBUFDEFAULTCHAN;
    mOwnerBlockSize = RINGBUFDEFAULTOWNERBLOCKSIZE;

    mBuffer.setSize(mNumChan, mBufSize);

    // initialize the buffers so there isn't garbage in them
    init();
    
    if(mDebugFlag)
        DBG("RingBuffer constructor called");
}

RingBuffer::~RingBuffer()
{
    // using smart pointers only, so nothing to delete
    if(mDebugFlag)
        DBG("RingBuffer destructor called");
}

void RingBuffer::debug(bool d)
{
    mDebugFlag = d;
}

// if host block size changes, best to call this and start buffering process over
void RingBuffer::init()
{
    mBuffer.clear();
    mWriteIdx = 0;
}

// TODO: assumes mBuffer and inBuf have the same number of channels
void RingBuffer::write(juce::AudioBuffer<float>& inBuf)
{
    auto N = inBuf.getNumSamples();

    // TODO: safety check to make sure that inBuf numSamples <= mBuffer numSamples and inBuf numChan == mBuffer numChan
    for(int channel = 0; channel < mNumChan; channel++)
    {
        // copy a block from the host into our ring buffer starting at mRingBufWriteIdx
        // TODO: protect wraparound
        mBuffer.copyFrom(channel, mWriteIdx, inBuf, channel, 0, N);
    }
    
    // advance by the host buffer size
    advanceWriteIdx(N);
}

void RingBuffer::write(int channel, juce::AudioBuffer<float>& inBuf)
{
    auto N = inBuf.getNumSamples();

    // TODO: safety check to make sure that inBuf numSamples <= mBuffer numSamples

    // copy a block from the host into our ring buffer starting at mRingBufWriteIdx
    // TODO: protect wraparound
    mBuffer.copyFrom(channel, mWriteIdx, inBuf, channel, 0, N);
    
    // advance by the host buffer size
    advanceWriteIdx(N);
}

// TODO: assumes mBuffer and outBuf have the same number of channels
void RingBuffer::read(juce::AudioBuffer<float>& outBuf, int delaySamps)
{
    int numChannels, outBufSize;
    
    numChannels = outBuf.getNumChannels();
    outBufSize = outBuf.getNumSamples();
    
    for(int channel = 0; channel < mNumChan; channel++)
    {
        auto* outBufPtr = outBuf.getWritePointer(channel);
        auto* ringBufPtr = mBuffer.getReadPointer(channel);
        int readIdx;
        
        // calculate a safe readIdx for this channel
        readIdx = mWriteIdx - delaySamps;

        // if readIdx is before the beginning of the buffer, we need to wrap around to the end of the buffer
        if(readIdx<0)
            readIdx += mBufSize;
        
        for(int sample = 0; sample < outBufSize; sample++, readIdx++)
            outBufPtr[sample] = ringBufPtr[readIdx % mBufSize];
    }
}

void RingBuffer::read(int channel, juce::AudioBuffer<float>& outBuf, int delaySamps)
{
    int outBufSize = outBuf.getNumSamples();
    
    auto* outBufPtr = outBuf.getWritePointer(channel);
    auto* ringBufPtr = mBuffer.getReadPointer(channel);
    int readIdx;
    
    // calculate a safe readIdx for this channel
    readIdx = mWriteIdx - delaySamps;

    // if readIdx is before the beginning of the buffer, we need to wrap around to the end of the buffer
    if(readIdx<0)
        readIdx += mBufSize;
    
    for(int sample = 0; sample < outBufSize; sample++, readIdx++)
        outBufPtr[sample] = ringBufPtr[readIdx % mBufSize];
}

// TODO: this shouldn't work properly yet
// TODO: assumes mBuffer and outBuf have the same number of channels
void RingBuffer::readInterp(juce::AudioBuffer<float>& outBuf, double delaySamps)
{
    int numChannels, outBufSize;
    
    numChannels = outBuf.getNumChannels();
    outBufSize = outBuf.getNumSamples();
    
    for(int channel = 0; channel < mNumChan; channel++)
    {
        auto* outBufPtr = outBuf.getWritePointer(channel);
        double readIdx;
        
        // calculate a safe readIdx for this channel
        readIdx = mWriteIdx - delaySamps;

        // if readIdx is before the beginning of the buffer, we need to wrap around to the end of the buffer
        if(readIdx < 0.0f)
            readIdx += mBufSize;
        
        for(int sample = 0; sample < outBufSize; sample++, readIdx++)
        {
            double readIdxWrapped;
            
            readIdxWrapped = std::fmod(readIdx, mBufSize);
            outBufPtr[sample] = Utilities::bufReadInterp(channel, readIdxWrapped, mBuffer);
        }
    }
}

// pass in the channel number and buffer sample number along with the fractional delay time
//double RingBuffer::readInterpSamp(int channel, int samp, double delaySamps)
double RingBuffer::readInterpSamp(int channel, double delaySamps)
{
    double outSamp, readIdx;
    
    outSamp = 0.0f;
    
    // calculate a safe readIdx
//    readIdx = mWriteIdx + samp - delaySamps;
    readIdx = mWriteIdx - delaySamps;
    // in the 2-arg case, when delaySamps is expected to be: desiredDelay - bufferSampIdx, the sample index ends up being added to the RingBuffer write index before subtracting the desired delay amount

    // if readIdx is before the beginning of the buffer, we need to wrap around to the end of the buffer
    // we'll also mod it by mBufSize in case it's beyond the end of the RingBuffer
    if(readIdx < 0.0f)
        readIdx += mBufSize;
    
    readIdx = std::fmod(readIdx, mBufSize);
    
    outSamp = Utilities::bufReadInterp(channel, readIdx, mBuffer);
    
    return outSamp;
}

int RingBuffer::getWriteIdx()
{
    return mWriteIdx;
}

// call this at the end of a block
void RingBuffer::advanceWriteIdx(int N)
{
    // advance mRingBufWriteIdx at the end of the processBlock call
    mWriteIdx += N;
    // wrap at mRingBufSize
    mWriteIdx = mWriteIdx % mBufSize;
}

int RingBuffer::getOwnerBlockSize()
{
    return mOwnerBlockSize;
}

void RingBuffer::setOwnerBlockSize(int N)
{
    mOwnerBlockSize = N;
}

int RingBuffer::getSize()
{
    return mBufSize;
}

void RingBuffer::setSize(int numChan, int numSamps, int ownerBlockSize)
{
    double thisSize;
    
    // update the owner block size
    setOwnerBlockSize(ownerBlockSize);
    
    // update number of channels
    mNumChan = numChan;
    
    // make the ring buffer size a multiple of the host's block size for convenience of wraparound
    thisSize = std::floor(numSamps/(double)mOwnerBlockSize);
    thisSize *= mOwnerBlockSize;
    
    mBufSize = thisSize;
    
    // do the acutal AudioBuffer resize
    mBuffer.setSize(mNumChan, mBufSize);
    
    if(mDebugFlag)
    {
        std::string debugPost = "RingBuffer mBufSize: " + std::to_string(mBufSize);
        DBG(debugPost);
    }
}

const float* RingBuffer::getReadPointer(int channel)
{
    return mBuffer.getReadPointer(channel);
}

float* RingBuffer::getWritePointer(int channel)
{
    return mBuffer.getWritePointer(channel);
}
} // namespace atec
