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
void RingBuffer::write(juce::AudioBuffer<float>& inBuf, bool advance)
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
    if(advance)
        advanceWriteIdx(N);
}

void RingBuffer::write(int destChannel, juce::AudioBuffer<float>& sourceBuf, int sourceChannel, int numSamps, bool advance)
{
    // TODO: safety check to make sure that inBuf numSamples <= mBuffer numSamples

    // copy a block from the host into our ring buffer starting at mRingBufWriteIdx
    // TODO: protect wraparound
    mBuffer.copyFrom(destChannel, mWriteIdx, sourceBuf, sourceChannel, 0, numSamps);
    
    // advance by the host buffer size
    if(advance)
        advanceWriteIdx(numSamps);
}

// write one sample value in a given channel and at a given offset from the current write index position
void RingBuffer::writeSample(int channel, int index, float sample)
{
    int thisIdx = (mWriteIdx + index) % mBufSize;
    
    mBuffer.setSample(channel, thisIdx, sample);
}

void RingBuffer::read(juce::AudioBuffer<float>& destBuf)
{
    int numChannels, destBufSize;
    
    numChannels = destBuf.getNumChannels();
    destBufSize = destBuf.getNumSamples();
    
    for(int channel = 0; channel < mNumChan; channel++)
    {
        auto* destBufPtr = destBuf.getWritePointer(channel);
        auto* ringBufPtr = mBuffer.getReadPointer(channel);
        int readIdx;
        
        // calculate a safe readIdx for this channel
        // read start point should be mOwnerBlockSize samples behind the write index at a minimum
        readIdx = mWriteIdx - mOwnerBlockSize;

        // if readIdx is before the beginning of the buffer, we need to wrap around to the end of the buffer
        if(readIdx < 0)
            readIdx += mBufSize;
        
        for(int sample = 0; sample < destBufSize; sample++, readIdx++)
            destBufPtr[sample] = ringBufPtr[readIdx % mBufSize];
    }
}

// TODO: assumes mBuffer and destBuf have the same number of channels
void RingBuffer::read(juce::AudioBuffer<float>& destBuf, int delaySamps)
{
    int numChannels, destBufSize;
    
    numChannels = destBuf.getNumChannels();
    destBufSize = destBuf.getNumSamples();
    
    for(int channel = 0; channel < mNumChan; channel++)
    {
        auto* destBufPtr = destBuf.getWritePointer(channel);
        auto* ringBufPtr = mBuffer.getReadPointer(channel);
        int readIdx;
        
        // calculate a safe readIdx for this channel
        // read start point should be mOwnerBlockSize samples behind the write index at a minimum
        readIdx = (mWriteIdx - mOwnerBlockSize) - delaySamps;

        // if readIdx is before the beginning of the buffer, we need to wrap around to the end of the buffer
        if(readIdx < 0)
            readIdx += mBufSize;
        
        for(int sample = 0; sample < destBufSize; sample++, readIdx++)
            destBufPtr[sample] = ringBufPtr[readIdx % mBufSize];
    }
}

void RingBuffer::read(int sourceChannel, int delaySamps, juce::AudioBuffer<float>& destBuf, int destChannel, int numSamps)
{
//    int destBufSize = destBuf.getNumSamples();
    
    auto* destBufPtr = destBuf.getWritePointer(destChannel);
    auto* ringBufPtr = mBuffer.getReadPointer(sourceChannel);
    int readIdx;
    
    // calculate a safe readIdx for this channel
    // read start point should be mOwnerBlockSize samples behind the write index at a minimum
    readIdx = (mWriteIdx - mOwnerBlockSize) - delaySamps;

    // if readIdx is before the beginning of the buffer, we need to wrap around to the end of the buffer
    if(readIdx < 0)
        readIdx += mBufSize;
    
    for(int sample = 0; sample < numSamps; sample++, readIdx++)
        destBufPtr[sample] = ringBufPtr[readIdx % mBufSize];
}

// this can be used if you don't want to guarantee a read index that's at least one host block size behind the write index.
// if used with writeNoAdvance, this can be used to achieve the lowest latency
void RingBuffer::readUnsafe(int sourceChannel, int delaySamps, juce::AudioBuffer<float>& destBuf, int destChannel, int numSamps)
{
//    int destBufSize = destBuf.getNumSamples();
    
    auto* destBufPtr = destBuf.getWritePointer(destChannel);
    auto* ringBufPtr = mBuffer.getReadPointer(sourceChannel);
    int readIdx;
    
    // calculate a readIdx for this channel
    // unsafe version doesn't back up by mOwnerBlockSize, so it's possible to read past the write index
    readIdx = mWriteIdx - delaySamps;

    // if readIdx is before the beginning of the buffer, we need to wrap around to the end of the buffer
    if(readIdx < 0)
        readIdx += mBufSize;
    
    for(int sample = 0; sample < numSamps; sample++, readIdx++)
        destBufPtr[sample] = ringBufPtr[readIdx % mBufSize];
}

// TODO: assumes mBuffer and destBuf have the same number of channels
void RingBuffer::readInterp(juce::AudioBuffer<float>& destBuf, double delaySamps)
{
    int numChannels, destBufSize;
    
    numChannels = destBuf.getNumChannels();
    destBufSize = destBuf.getNumSamples();
    
    for(int channel = 0; channel < mNumChan; channel++)
    {
        auto* destBufPtr = destBuf.getWritePointer(channel);
        double readIdx;
        
        // calculate a safe readIdx for this channel
        // read start point should be mOwnerBlockSize samples behind the write index at a minimum
        readIdx = (mWriteIdx - mOwnerBlockSize) - delaySamps;

        // if readIdx is before the beginning of the buffer, we need to wrap around to the end of the buffer
        if(readIdx < 0.0f)
            readIdx += mBufSize;
        
        for(int sample = 0; sample < destBufSize; sample++, readIdx++)
        {
            double readIdxWrapped;
            
            readIdxWrapped = std::fmod(readIdx, mBufSize);
            destBufPtr[sample] = Utilities::bufReadInterp(channel, readIdxWrapped, mBuffer);
        }
    }
}

void RingBuffer::readInterp(int sourceChannel, double delaySamps, juce::AudioBuffer<float>& destBuf, int destChannel, int numSamps)
{
//    int destBufSize = destBuf.getNumSamples();
    
    auto* destBufPtr = destBuf.getWritePointer(destChannel);
    double readIdx;
    
    // calculate a safe readIdx for this channel
    // read start point should be mOwnerBlockSize samples behind the write index at a minimum
    readIdx = (mWriteIdx - mOwnerBlockSize) - delaySamps;

    // if readIdx is before the beginning of the buffer, we need to wrap around to the end of the buffer
    if(readIdx < 0.0f)
        readIdx += mBufSize;
    
    for(int sample = 0; sample < numSamps; sample++, readIdx++)
    {
        double readIdxWrapped;
        
        readIdxWrapped = std::fmod(readIdx, mBufSize);
        destBufPtr[sample] = Utilities::bufReadInterp(sourceChannel, readIdxWrapped, mBuffer);
    }
}

// pass in the channel number and destination buffer sample number along with the fractional delay time
double RingBuffer::readInterpSample(int channel, int samp, double delaySamps)
//double RingBuffer::readInterpSamp(int channel, double delaySamps)
{
    double outSamp, readIdx;
    
    outSamp = 0.0f;
    
    // calculate a safe readIdx
    // read start point should be mOwnerBlockSize samples behind the write index at a minimum
    // need to go samp samples beyond this minimum in step with the hypothetical buffer we're filling
    // finally, need to back up delaySamps samples in order to achieve the correct delay
    readIdx = ((mWriteIdx - mOwnerBlockSize) + samp) - delaySamps;

    // if readIdx is before the beginning of the buffer, we need to wrap around to the end of the buffer
    if(readIdx < 0.0f)
        readIdx += mBufSize;
    
    // we'll also mod it by mBufSize in case it's beyond the end of the RingBuffer
    readIdx = std::fmod(readIdx, mBufSize);
    
    outSamp = Utilities::bufReadInterp(channel, readIdx, mBuffer);
    
    return outSamp;
}

double RingBuffer::readInterpSample(int channel, double sampInc, double* lastReadIdx)
{
    double outSamp, readIdx;
    
    outSamp = 0.0f;
    
    // calculate a safe readIdx
    // read start point should be mOwnerBlockSize samples behind the write index at a minimum
    // need to go samp samples beyond this minimum in step with the hypothetical buffer we're filling
    // finally, need to back up delaySamps samples in order to achieve the correct delay
    if (*lastReadIdx < 0.0f)
        readIdx = (mWriteIdx - mOwnerBlockSize);
    else
        readIdx = *lastReadIdx + sampInc;

    // if readIdx is before the beginning of the buffer, we need to wrap around to the end of the buffer
    if(readIdx < 0.0f)
        readIdx += mBufSize;
    
    // we'll also mod it by mBufSize in case it's beyond the end of the RingBuffer
    readIdx = std::fmod(readIdx, mBufSize);
        
    outSamp = Utilities::bufReadInterp(channel, readIdx, mBuffer);
    
    // update the read index value
    *lastReadIdx = readIdx;

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

const juce::AudioBuffer<float>& RingBuffer::getBufRef()
{
    return mBuffer;
}

} // namespace atec
