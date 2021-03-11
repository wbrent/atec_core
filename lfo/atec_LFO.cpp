namespace atec
{
LFO::LFO()
{
    mDebugFlag = false;
    
    mSampleRate = 48000.0f;
    mType = sin;
    mFreq = 6.0f;
    mRange.setStart(0.0f);
    mRange.setEnd(1.0f);
    mPhaseAngle = 0.0f;
    mPhaseDelta = 0.0f;
    
    init();
}

LFO::~LFO()
{
//    std::printf("LFO destructor\n");
//    std::printf("sampleRate: %f, freq: %f, delta: %f\n", mSampleRate, mFreq, mPhaseDelta);
}

void LFO::debug(bool d)
{
    mDebugFlag = d;
}

void LFO::init()
{
    calcPhaseDelta();
    
    if (mDebugFlag)
    {
        std::string post;
        post = "LFO angle delta: " + std::to_string(mPhaseDelta) + ", freq: " + std::to_string(mFreq);
        DBG(post);
        post = "LFO range start: " + std::to_string(mRange.getStart()) + ", end: " + std::to_string(mRange.getEnd());
        DBG(post);
    }
}

LFO::LfoType LFO::getType()
{
    return mType;
}

void LFO::setType (LFO::LfoType t)
{
    if(t > NUMLFOTYPES-1 || t < 0)
        t = sin;

    mType = t;
}

double LFO::getFreq()
{
    return mFreq;
}

void LFO::setFreq(double f)
{
    mFreq = f;
    
    if (mDebugFlag)
        DBG("LFO frequency: " + juce::String(mFreq));
    
    calcPhaseDelta();
}

juce::Range<double> LFO::getRange()
{
    return mRange;
}

void LFO::setRange(juce::Range<double> r)
{
    mRange = r;
}

double LFO::getPhaseDelta()
{
    return mPhaseDelta;
}

double LFO::getPhase()
{
    return mPhaseAngle;
}

void LFO::setPhase(double p)
{
    mPhaseAngle = p;
}

double LFO::getSampleRate()
{
    return mSampleRate;
}

void LFO::setSampleRate(double sampleRate)
{
    mSampleRate = sampleRate;
    calcPhaseDelta();
}

double LFO::getNextSample()
{
    double thisSample;
    
    switch(mType)
    {
        case sin:
            thisSample = std::sin(mPhaseAngle);
            // normalize to 0-1 range
            thisSample += 1.0f;
            thisSample *= 0.5f;
            break;
        case cos:
            thisSample = std::cos(mPhaseAngle);
            // normalize to 0-1 range
            thisSample += 1.0f;
            thisSample *= 0.5f;
            break;
        case square:
            thisSample = std::cos(mPhaseAngle) * __FLT_MAX__;
            thisSample = (thisSample > 1.0f) ? 1.0f : thisSample;
            thisSample = (thisSample < -1.0f) ? -1.0f : thisSample;
            // normalize to 0-1 range
            thisSample += 1.0f;
            thisSample *= 0.5f;
            break;
        case saw:
            thisSample = mPhaseAngle / juce::MathConstants<double>::twoPi;
            break;
        case triangle:
            thisSample = mPhaseAngle / juce::MathConstants<double>::twoPi;
            // downward cycle
            if(thisSample > 0.5)
                thisSample = 1.0f - thisSample;
            // normalize to 0-1 range
            thisSample *= 2.0f;
            break;
        default:
            thisSample = 0.0f;
            break;
    }
    
    // re-scale according to mRange
    thisSample *= mRange.getLength();
    thisSample += mRange.getStart();
    
    mPhaseAngle += mPhaseDelta;
    mPhaseAngle = std::fmod(mPhaseAngle, juce::MathConstants<double>::twoPi);
    
    // fmod() returns negative values if mPhaseAngle is negative
    // if so let's wrap around to the positive side
    if (mPhaseAngle < 0.0f)
        mPhaseAngle += juce::MathConstants<double>::twoPi;
    
    return thisSample;
}

void LFO::calcPhaseDelta()
{
    double cyclesPerSample = mFreq/mSampleRate;
    mPhaseDelta = cyclesPerSample * juce::MathConstants<double>::twoPi;
    
    if (mDebugFlag)
        DBG("LFO phase delta: " + juce::String(mPhaseDelta));
}
} // namespace atec
