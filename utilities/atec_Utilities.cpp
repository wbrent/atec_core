namespace atec
{
Utilities::Utilities()
{
}

Utilities::~Utilities()
{
}

bool Utilities::isEven(double value)
{
    bool even;
    
    even = std::fmod(value, 2) == 0;
    
    return even;
}

int Utilities::getSign(double value)
{
    int sign;
    
    if(value > 0)
        sign = 1;
    else if(value < 0)
        sign = -1;
    else
        sign = 0;
    
    return sign;
}

double Utilities::linearToExp(double value, double power, juce::Range<double> range)
{
    double result;
    
    result = std::powf(value, power);
    
    result *= range.getLength();
    result += range.getStart();
    
    return result;
}

double Utilities::expToLinear(double value, double power, juce::Range<double> range)
{
    double result;
    
    result = value - range.getStart();
    result /= range.getLength();
    result = std::powf(result, 1.0f/power);
    
    return result;
}

double Utilities::sec2samp(double sec, double sampleRate)
{
    double samp;
    
    samp = sec * sampleRate;
    
    return samp;
}

double Utilities::samp2sec(double samp, double sampleRate)
{
    double sec;
    
    sec = samp / sampleRate;
    
    return sec;
}

double Utilities::freq2midi(double f)
{
    double m;
    
    m = 12.0f * std::log2(f/440.0f) + 69.0f;
    
    return m;
}

double Utilities::midi2freq(double m)
{
    double f;
    
    //  fm = 2^((m−69)/12) * (440 Hz).
    f = std::pow(2.0f, (m - 69.0f) / 12.0f) * 440.0f;

    return f;
}

/*
 will produce an interpolated sample between y1 and y2, based on a mu value between 0.0 and 1.0
 */
double Utilities::cubicInterpolate(double y0, double y1, double y2, double y3, double mu)
{
    double a0, a1, a2, a3, mu2;
    
    mu2 = mu*mu;
    a0 = y3 - y2 - y0 + y1;
    a1 = y0 - y1 - a0;
    a2 = y2 - y0;
    a3 = y1;
    
    return(a0*mu*mu2+a1*mu2+a2*mu+a3);
}

double Utilities::bufReadInterp(int channel, double readIdx, juce::AudioBuffer<float>& buffer)
{
    int N, j, r0, r1, r2, r3;
    double outSamp, mu;
    auto* bufPtr = buffer.getReadPointer(channel);

    // get the number of samples in the buffer
    N = buffer.getNumSamples();
    
    // get the fractional part of the read position
    mu = readIdx - floor(readIdx);
    // get the integer part of the read position
    j = floor(readIdx);
    
    // set r0 through r3. if any j value is out of bounds, wrap to the beginning or end of the buffer to avoid reading out of bounds
    r0 = ((j-1)>=0) ? j-1 : N + (j-1);
    r1 = j;
    r2 = (j+1) % N;
    r3 = (j+2) % N;

    // pull an interpolated sample
    outSamp = cubicInterpolate(bufPtr[r0], bufPtr[r1], bufPtr[r2], bufPtr[r3], mu);
    
    return outSamp;
}

// shuffle the contents of a juce::Array. Overload with <float> and <double> versions too
// this is a very quick and dirty method! it just randomly swaps elements in the array once per iteration of the while loop.
void Utilities::arrayShuffle(juce::Array<int>& array)
{
    int numElements, numIter;
    
    // use current time as seed for random generator
    std::srand( (unsigned int)std::time(nullptr) );
    
    numElements = array.size();
    // let's do numElements * 10 swaps
    numIter = numElements * 10;

    while(numIter--)
    {
        int i, j;

        i = std::rand() % (numElements - 1);
        j = i + (std::rand() % (numElements - 1));
        j = j % (numElements - 1);

        array.swap(i, j);
    }
}

int Utilities::getZeroCrossingPoints(int channel, juce::AudioBuffer<float>& buffer, juce::Array<int>& xIndices, double threshDb)
{
    int crossings = 0;
    int bufSize = buffer.getNumSamples();
    auto* bufPtr = buffer.getReadPointer(channel);
        
    for (int i = 1; i < bufSize; i++)
    {
        if (getSign(bufPtr[i]) != getSign(bufPtr[i-1]))
        {
            // TODO: a more strict criterion would be that the samples on BOTH sides of the crossing are BOTH below the dB thresh
            // check that the sample at the crossing is below our desired dB thresh
            if (std::fabs(bufPtr[i-1]) < juce::Decibels::decibelsToGain(threshDb))
            {
                // add the sample index preceding the crossing
                xIndices.add(i-1);
                crossings++;
            }
        }
    }
    
    return crossings;
}

double Utilities::getZeroCrossingRate(int channel, juce::AudioBuffer<float>& buffer)
{
    double crossings = 0.0f;
    auto N = buffer.getNumSamples();
    auto* bufPtr = buffer.getReadPointer(channel);
    
    for (int i = 1; i < N; i++)
        crossings += std::fabs(getSign(bufPtr[i]) - getSign(bufPtr[i-1]));
    
    crossings /= 2*N;
    
    return crossings;
}

void Utilities::getFftPowerSpec(juce::dsp::Complex<float>* inBuf, juce::Array<double>& outBuf, int N)
{
    // clear the mag spec buffer
    outBuf.fill(0.0f);
    
    for (int i = 0; i < N; i++)
    {
        double thisMag;
        thisMag = inBuf[i].real() * inBuf[i].real() + inBuf[i].imag() * inBuf[i].imag();
        
        outBuf.set(i, thisMag);
    }
}

void Utilities::getFftMagSpec(juce::dsp::Complex<float>* inBuf, juce::Array<double>& outBuf, int N)
{
    // clear the mag spec buffer
    outBuf.fill(0.0f);
    
    for (int i = 0; i < N; i++)
    {
        double thisMag;
        thisMag = inBuf[i].real() * inBuf[i].real() + inBuf[i].imag() * inBuf[i].imag();
        thisMag = std::sqrt(thisMag);
        
        outBuf.set(i, thisMag);
    }
}

void Utilities::getFftPhaseSpec(juce::dsp::Complex<float>* buffer, juce::Array<double>& outBuf, int N)
{
    // clear the phase spec buffer
    outBuf.fill(0.0f);
    
    for (int i = 0; i < N; i++)
    {
        double thisPhase;
        thisPhase = std::atan2f(buffer[i].imag(), buffer[i].real());
        
        outBuf.set(i, thisPhase);
    }
}

void Utilities::fftZeroPhase(juce::dsp::Complex<float>* buffer, int N)
{
    for (int i = 0; i < N; i++)
    {
        buffer[i].real(std::fabsf(buffer[i].real()));
        buffer[i].imag(0.0f);
    }
}

// incoming filter argument should be N/2+1 elements long. the function handles the mirror image/negative frequency multiplication aspect of the N-point Complex inBuf
void Utilities::fftApplyFilter(juce::dsp::Complex<float>* inBuf, juce::Array<double> filter, int N)
{
    for (int i = 0; i < N; i++)
    {
        float newReal, newImag, thisScalar;
        
        // get the right magnitude scalar for the negative frequencies
        if (i <= N * 0.5)
            thisScalar = filter[i];
        else
            thisScalar = filter[N-i];
        
        newReal = inBuf[i].real() * thisScalar;
        newImag = inBuf[i].imag() * thisScalar;

        inBuf[i].real(newReal);
        inBuf[i].imag(newImag);
    }
}
} // namespace atec
