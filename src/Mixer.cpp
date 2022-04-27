/*--------------------------------------------------------------------------------
 Mixer.cpp
 Version 1.0.0
 Apache Lisence 2.0
 --------------------------------------------------------------------------------*/

#include <Mixer.h>

#include <strstream>
#include <iostream>
#include <algorithm>
#include <cmath> // for M_PI define

#ifndef MIN
#define MIN(a,b) ((a <= b) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a,b) ((a >= b) ? (a) : (b))
#endif

int Mixer::mix(const float** bufferPgm, const int nsamples, int nchannels, const float samplerate, const float* bufferBeeps, float** bufferMix)
{
  progress_mix = 0;
  std::cout << "Progress MIX = " << progress_mix << std::endl;

  std::vector<float> timestamps;
  std::vector<float> beepLevel;
  float percentile10;
  
  computeBeepLevel(bufferPgm[0], nsamples, samplerate, timestamps, beepLevel, percentile10);
  int ntimestamps = beepLevel.size()-2;

  // get linear gain values
  float defBeepLevel = pow(10.f, mDefaultBeepLevel/20.f);
  float defPgmLevel = pow(10.f, mDefaultProgramLevel/20.f);
  
  std::cout << "Progress MIX = " << 95 << std::endl;

  float maxpeak = 0.;
  if (mMode == kDynamicLevelMode)
  {
    int eidx = 0; // energy index
    float level = 1.f;
    float interp = 0.f;
    
    for (int i=0; i < nsamples; i++)
    {
      // get current index in energy timestamps
      if ((i/samplerate) > timestamps[eidx+1])
        eidx++;
      eidx = std::min(eidx, ntimestamps);
      
      // interpolate level value per sample
      interp = ((i/samplerate) - timestamps[eidx])/ (timestamps[eidx+1] - timestamps[eidx]);
      level = (1.f - interp) * beepLevel[eidx] + interp * beepLevel[eidx+1];
      // mix buffers
      for (int j=0; j < nchannels; j++){
        bufferMix[j][i] = MAX(-1.0, MIN(1.0, level * bufferBeeps[i] + defPgmLevel * bufferPgm[j][i]));
        // update max peak
        maxpeak = (abs(bufferMix[j][i]) > maxpeak) ? abs(bufferMix[j][i]) : maxpeak;
      }
    }
  }
  else
    if (mMode == kGlobalLevelMode){
      
      float minlevelLin = pow(10.f, mMinBeepLevel/20.f);
      float globalLevelDB = percentile10 + mDefaultBeepLevel;
      float globalBeepLevel = std::max(minlevelLin, std::min(.95f, powf(10.f, globalLevelDB /20.f)));
      
      // apply default gain for all channels
      for (int i=0; i < nsamples; i++)
        for (int j=0; j < nchannels; j++){
          bufferMix[j][i] = MAX(-1.0, MIN(1.0, globalBeepLevel * bufferBeeps[i] + defPgmLevel * bufferPgm[j][i]));
          // update max peak
          maxpeak = (abs(bufferMix[j][i]) > maxpeak) ? abs(bufferMix[j][i]) : maxpeak;
        }
    }
    else
      if (mMode == kDefaultMode)
      {
        // apply default gain for all channels
        for (int i=0; i < nsamples; i++)
          for (int j=0; j < nchannels; j++){

            //bufferMix[j][i] = defBeepLevel * bufferBeeps[i] + defPgmLevel * bufferPgm[j][i];
            bufferMix[j][i] = MAX(-1.0,MIN(1.0,defBeepLevel * bufferBeeps[i] + defPgmLevel * bufferPgm[j][i]));
            // update max peak
            maxpeak = (abs(bufferMix[j][i]) > maxpeak) ? abs(bufferMix[j][i]) : maxpeak;
          }
      }
      else
      {
        return 1;
      }
  
  // might be disabled for optimization
  if (mUseNormalize)
    for (int i=0; i < nsamples; i++)
      for (int j=0; j < nchannels; j++)
        bufferMix[j][i] /= maxpeak;
  
  std::cout << "Progress MIX = " << 100 << std::endl;

  return 0;
}


// returns a vector of level (linear gain) for the beeps signal
int Mixer::computeBeepLevel(const float* buffer, const int nsamples,  const float samplerate, std::vector<float> &timestamps, std::vector<float> &beepLevel, float &percentile10)
{
  // estimate energy and dynamics curves
  std::vector<float> energy;
  std::vector<float> energyDB;
  std::vector<float> stab;
  
  // Program statistics
  
  // set frameTime  to 11.6ms
  float frametime = 512.f/samplerate; // 11.6ms default
  
  computeEnergy(buffer, nsamples, samplerate, frametime, timestamps, energy);
  
  std::cout << "Progress MIX = " << 75 << std::endl;
  
  computeDynamicsStability(energy, frametime, energyDB, stab, percentile10);

  std::cout << "Progress MIX = " << 90 << std::endl;
  
  
  // Program Typical levels:
  // - comercial Rock music: [-5..-35dB]
  // - acoustic pop music: [-12..-40dB]
  // - documentary speech: [-15..-45dB]
  
  // Beeps typical levels:
  // -3dB: usual level for inaudible
  // -12dB: usual level for hidden
  // -3dB: usual level for audible
  
  
  // compute mixing levels for beep signal
  float maxLevelDB = mDefaultBeepLevel;
  float minLevelDB = mMinBeepLevel;
  float level = 1.f;
  float levelDB = 0.f;
  
  for (int i = 0; i < (int)energyDB.size(); i++)
  {
    // scale range [-5..-40] to [maxLevelDB..maxLevelDB-20] dB
    //levelDB = (std::max(maxLevelDB - minLevelDB, std::min(maxLevelDB, maxLevelDB + (energyDB[i] + 5.f) / (-5.f + 40.f))) / minLevelDB);
    levelDB = std::max(minLevelDB, std::min(energyDB[i]+mDefaultBeepLevel, maxLevelDB));
    level = powf(10.f, levelDB/20.f);
    //todo: ask jordi janer about this line
    //level = powf(10.f, (std::max(maxLevelDB-minLevelDB, std::min(maxLevelDB, maxLevelDB + (energyDB[i] + 5.f)/ (-5.f + 40.f)))/minLevelDB));
    beepLevel.push_back(level); // linear
  }

  return 0;
}


int Mixer::computeEnergy(const float *buffer, const int nsamples,  const float samplerate, float frameTime, std::vector<float> &timestamps, std::vector<float> &energy)
{ 
  int h = int(frameTime*samplerate + 0.5); // hop size
  int ws = 4*h; //int(2048*samplerate/44100.f); // win size
  ws = ws - (ws%2); // make it even
  std::vector<float> w;
  w.reserve(nsamples);
  energy.reserve(nsamples);
  timestamps.reserve(nsamples);
  float area = hanning(ws+1, w);
  int hws = ws/2;
  
  int nFrames = int(nsamples/h)-2;  
  
  int i,k;
  for (i=0; i<nFrames; i++) {
    
    float current_progress_mix = ((float)i / (float)nFrames)*75.f; //from 0% to 75%
    if (current_progress_mix > progress_mix + 5)
    {
      progress_mix = current_progress_mix;
      std::cout << "Progress MIX = " << progress_mix << std::endl;
    }

    // compute energy for one window frame
    int b = h*i-hws;
    int e = h*i+hws;
    float en = 0.f;
    if (b<1)
      for(k=0;k<=e;k++)
        en += (buffer[k]*buffer[k])*w[k-b];
    else
      if (e>=nsamples)
        for(k=b;k<nFrames;k++){
          en += (buffer[k]*buffer[k])*w[k-b];
        }
      else
        for(k=b;k<=e;k++)
          en += (buffer[k]*buffer[k])*w[k-b];
    
    // store values
    timestamps.push_back(i * frameTime);
    en /= area;
    energy.push_back(en);
  }

  return 0;
}


// -------------------------------------------------------------------------------------------------
// compute a dynamics stability measure from a input energy (linear) vector and outputs the energy in DB
int Mixer::computeDynamicsStability(const std::vector<float> energy, float frameTime, std::vector<float> &energyDB, std::vector<float> &st, float &percentile10)
{
  int nFr = energy.size();
  energyDB.clear();       // empty vector
  st.clear();             // empty vector
  std::vector<float> ew;  // energy weight
  
  float edB = 0;
  for (int i=0; i < nFr; ++i)
  {
    edB = 10.f*log10f(energy[i] + 1e-200);
    energyDB.push_back(edB);
    st.push_back( edB );
    ew.push_back( edB );
  }
  
  std::cout << "Progress MIX = " << 80 << std::endl;

  // compute range of energy weight
  std::sort(ew.begin(),ew.end());
  int per10Idx =  floor(float(ew.size()) * .1f);
  double p10 = *(ew.end() - per10Idx); // compute percetile 10
  percentile10 = p10;
  //printf("Percentile: %f  (%d frames)\n", p10, nFr);
  
  float rangeDB = 18; // in DB
  float stabThres = 3.25;
  float stabK = 1.75; // sigmoid factor
  
  float xval = 0.;
  st[0] = 0.;
  for(int i=1; i<nFr-2; i++)
  {
    st[i] = 0.5 * (energyDB[i+2] + energyDB[i+1] - energyDB[i] - energyDB[i-1]); // non energy-weighted
#ifdef __LINUX__
    st[i] = std::abs(st[i]);
#else
    st[i] = abs(st[i]);
#endif
    
    // sigmoid to scale it in a range [0..1] // from non-stable (0) to stable (1)
    xval = (st[i] - stabThres); // remove threshold
    st[i] = 1.f - ( 1.f / (1.f + exp(- stabK * xval)));  // non-linear interpolation (logistic function to preserve best the boundaries)
    
    // apply energy-weight to avoid having stablility low-energy (noise floor). We apply a weight relative to the 2 times the range dB from the percentile-10 of the energy values.
#ifdef __LINUX__
    st[i] *= std::max(0.0, std::min(1.0, (energyDB[i] - (p10-2.f*rangeDB)) / (rangeDB)));
#else
    st[i] *= fmax(0.0, std::min(1.0, (energyDB[i] - (p10-2.f*rangeDB)) / (rangeDB))); // apply energy-weighted relative to the range -24dB from the percentile-10 of the energy values.
#endif
    
  }
  
  std::cout << "Progress MIX = " << 85 << std::endl;

  st[st.size()-2] = 0.f;
  st[st.size()-1] = 0.f;
  for(int i=0; i<nFr-1; i++) // square to increase binary behaviour at boundaries
    st[i] *= st[i];
  
  // smooth
  int smoothframes = int(mSmoothTime / frameTime);
  smooth(st, smoothframes, false);
  smooth(energyDB, smoothframes, false);

  return 0;
}


void Mixer::smooth(std::vector<float> &v,int window, bool useNonZero)
{
  //
  int i,k;
  int hw = int(window/2);
  std::vector<float> v2 = v;
  for(i=1; i<v.size()-1; i++)
  {
    int b = std::max(0,i-hw);
    int e = std::min((int)v.size()-1,i+hw);
    float s=0.f;
    int den = 0;
    for(k=b;k<=e;k++){
      s+= v2[k];
      if (v2[k] > 0){
        den++;
      }
    }
    
    if (useNonZero){
      // consider only non-zero values
      if (v[i] > 0){
        v[i] = s/float(den);
      }
      else {
        v[i] = 0.f;
      }
    }
    else {
      if ((e-b)> 0)
        v[i] = s/float(e-b+1.f);
      else
        v[i] = s;
    }
    
  }
}

#ifndef M_PI
#define M_PI 3.14159265358979323846264338327950288
#endif

float Mixer::hanning(const int n, std::vector<float> &w)
{
  float area = 0.f;
  float v = 0.f;
  for (int i=0; i<n;i++){
    v = 0.5f * (1 - cos(2*M_PI*i/(n-1)));
    w.push_back(v);
    area += v;
  }

  return area;
}
