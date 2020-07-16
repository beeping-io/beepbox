/*--------------------------------------------------------------------------------
 Mixer.h
 Version 1.1.0
 Apache Lisence 2.0
 --------------------------------------------------------------------------------*/

#ifndef Mixer_h
#define Mixer_h

//#include <stdio.h>

#include <vector>
#include <math.h>


#define kDefaultMode 0
#define kGlobalLevelMode 1
#define kDynamicLevelMode 2

class Mixer{
public:
  Mixer(){
    
    mDefaultBeepLevel = -3.; // -3dB
    mMinBeepLevel = -20.f; // -20dB
    mDefaultProgramLevel = 0; // 0dB
    
    mSmoothTime = 0.350; // 350 ms
    
    // set default flags
    mMode = kDynamicLevelMode;
    mUseNormalize = true;
  };

  Mixer(int mode, float volumedb) {

    mDefaultBeepLevel = volumedb; // -3dB
    mMinBeepLevel = -20.f; // -20dB
    mDefaultProgramLevel = 0; // 0dB
    mSmoothTime = 0.350; // 350 ms

                         // set default flags
    mMode = mode;
    mUseNormalize = true;

    progress_mix = 0;
  };
  
  ~Mixer() {};
  int mix(const float** bufferPgm, const int nsamples, int nchannels, const float samplerate, const float* bufferBeeps, float** bufferMix);
  int computeBeepLevel(const float* buffer, const int nsamples,  const float samplerate, std::vector<float> &timestamps, std::vector<float> &beepLevel, float &percentile10);
  int computeEnergy(const float *buffer, const int nsamples,  const float samplerate, float frameTime, std::vector<float> &timestamps, std::vector<float> &energy);
  int computeDynamicsStability(const std::vector<float> energy, float frameTime, std::vector<float> &energyDB, std::vector<float> &st, float &percentile10);
  void smooth(std::vector<float> &v,int window, bool useNonZero);
  int computeLevels(const std::vector<float> energy, std::vector<float> &energyDB, std::vector<float> &st);
  
  float hanning(const int n, std::vector<float> &w);
  
  // set functions
  void setBeepLevel(float gainDB){ mDefaultBeepLevel = gainDB;};
  void setMinBeepLevel(float gainDB){ mMinBeepLevel = gainDB;};
  void setProgramLevel(float gainDB){ mDefaultProgramLevel = gainDB;};
  void setSmoothTime(float time){ mSmoothTime = time;};
  void setMode(int val) {mMode = val;};
  void setUseNormalize(bool val) {mUseNormalize = val;};
  
private:
  float mDefaultBeepLevel;
  float mDefaultProgramLevel;
  float mSmoothTime;
  float mMinBeepLevel;
      // flags
  int mMode;
  bool mUseNormalize;

  int progress_mix;
};

#endif /* Mixer_h */
