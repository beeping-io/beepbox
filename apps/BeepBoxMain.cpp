/*--------------------------------------------------------------------------------
 BeepBox
 Version 1.1.0
 Apache License 2.0
 --------------------------------------------------------------------------------*/

#include "BeepingCoreLib_api.h"

#include "Globals.h"

#include "sndfile.h"

#include <stdlib.h>
#include <iostream>
#include <ctime>
#include <cassert>
#include <math.h>

#include "Base/CliParser.hxx"
#include "Mixer.h"

#include "LoudnessStats.h"


#ifndef MIN
  #define MIN(a,b) ((a <= b) ? (a) : (b))
#endif

#ifndef MAX
  #define MAX(a,b) ((a >= b) ? (a) : (b))
#endif


void showVersion(int silentMode)
{
  if (silentMode == 0)
  {
    std::cerr << "************************************************************" << std::endl;
    std::cerr << "*  BeepBox App                                             *" << std::endl;
    std::cerr << "*  Apache License 2.0                                      *" << std::endl;    
    std::cerr << "*   -- BeepBox v1.1.0 [20200716] --                        *" << std::endl;    
    std::cerr << "************************************************************" << std::endl << std::endl;
  }
}

char getCharFromIdx(int idx)
{
  if      (idx==0)  return '0';
  else if (idx==1)  return '1';
  else if (idx==2)  return '2';
  else if (idx==3)  return '3';
  else if (idx==4)  return '4';
  else if (idx==5)  return '5';
  else if (idx==6)  return '6';
  else if (idx==7)  return '7';
  else if (idx==8)  return '8';
  else if (idx==9)  return '9';
  else if (idx==10) return 'a';
  else if (idx==11) return 'b';
  else if (idx==12) return 'c';
  else if (idx==13) return 'd';
  else if (idx==14) return 'e';
  else if (idx==15) return 'f';
  else if (idx==16) return 'g';
  else if (idx==17) return 'h';
  else if (idx==18) return 'i';
  else if (idx==19) return 'j';
  else if (idx==20) return 'k';
  else if (idx==21) return 'l';
  else if (idx==22) return 'm';
  else if (idx==23) return 'n';
  else if (idx==24) return 'o';
  else if (idx==25) return 'p';
  else if (idx==26) return 'q';
  else if (idx==27) return 'r';
  else if (idx==28) return 's';
  else if (idx==29) return 't';
  else if (idx==30) return 'u';
  else if (idx==31) return 'v';
  else              return '0';
}

//e.g. hj abcdefghij n9du84bb

char *fromDecToBase(int num, int rad)
{
  char digits[] = "0123456789abcdefghijklmnopqrstuvWXYZabcdefghijklmnopqrstuvwxyz";
  int i;
  char buf[66];   /* enough space for any 64-bit in base 2 */
  buf[0]=0;

  /* bounds check for radix */
  if (rad < 2 || rad > 62)
      return NULL;
  /* if num is zero */
  if (!num)
      //return StrDup("0");
      return strdup("0");

  /* null terminate buf, and set i at end */
  buf[65] = '\0';
  i = 65;

  if (num > 0) {  /* if positive... */
      while (num) { /* until num is 0... */
          /* go left 1 digit, divide by radix, and set digit to remainder */
          buf[--i] = digits[num % rad];
          num /= rad;
      }
  } else {    /* same for negative, but negate the modulus and prefix a '-' */
      while (num) {
          buf[--i] = digits[-(num % rad)];
          num /= rad;
      }
      buf[--i] = '-';
  }
  /* return a duplicate of the used portion of buf */
  //return StrDup(buf + i);
  return strdup(buf + i);
}

//int fromBaseToDec(char* number, int base)

int charToVal(char curChar)
{
  char convArray[] =
  {'0','1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v' };
  for(int i=0;i<32; ++i)
  {
    if(curChar==convArray[i])
    {
      return i;
    }
  }
  return -1;
}

int fromBaseToDec(char * number, int length, int rad)
{
  int decimal = 0;
  int factor = 1;
  //std::cout << length;
  for (int i=length-1; i >=0; --i)
  {
    int curVal = charToVal(number[i]);
    decimal += factor*curVal;
    factor*=rad;
  }
  //std::cout << decimal << std::endl;
  return decimal;
}

int main(int argc, char** argv)
{
  void* mBeepingCore;

  // Handle command line interface:
  CliParser cliParser;
  cliParser.addOption("m", "mode", CliParser::CLI_INT, true, "value", "Beeping Mode (0:audible, 1:hidden, 2:non-audible, 3:custom)", "2");
  cliParser.addOption("f", "file", CliParser::CLI_STRING, true, "filename", "Input filename (.wav) to mix with beeps", "");
  cliParser.addOption("k", "key", CliParser::CLI_STRING, false, "key", "Key identifier (5 characters) to encode in output audio (e.g. 01234)", "");
  cliParser.addOption("d", "duration", CliParser::CLI_FLOAT, true, "value", "Duration of output file in seconds (>=5.1)", "5.1");
  cliParser.addOption("i", "interval", CliParser::CLI_FLOAT, true, "value", "Interval in seconds (>=2.5) between two audio marks (e.g. 10)", "2.5");
  cliParser.addOption("s", "start", CliParser::CLI_FLOAT, true, "value", "Start time of the first audio mark in seconds (>2.2) (e.g. 2.5)", "5");
  cliParser.addOption("o", "output", CliParser::CLI_STRING, false, "filename", "Filename of output audio file that will be written (.wav)", "");

  cliParser.addOption("x", "mixmode", CliParser::CLI_INT, true, "value", "Mixing mode (0: DefaultLevel, 1: GlobalLevel, 2: DynamicLevel)", "0");
  cliParser.addOption("v", "volumebeeps", CliParser::CLI_FLOAT, true, "value", "Set default beeps level in DB", "-3.0"); //see Cliparser hack to allow negative values
  cliParser.addOption("p", "volumeprogram", CliParser::CLI_FLOAT, true, "value", "Set default program level in DB", "0.0"); //see Cliparser hack to allow negative values

  cliParser.addOption("r", "samplerate", CliParser::CLI_FLOAT, true, "value", "Sampling rate for output file (e.g. 44100.0 or 48000.0)", "44100.0");

  cliParser.addOption("l", "loudnessstatistics", CliParser::CLI_INT, true, "value", "Loudness statistics including LKFS and True Peak (0: disabled, 1:enabled)", "0");

  cliParser.addOption("bf", "basefreq", CliParser::CLI_FLOAT, true, "value", "Base Frequency in Hz for beeping custom mode  (e.g. 12000.0)", "12000.0");
  cliParser.addOption("ts", "tonesseparation", CliParser::CLI_INT, true, "value", "Separation between tones (1: minimum separation, 20:maximum separation)", "1");

  cliParser.addOption("sm", "synthmode", CliParser::CLI_INT, true, "value", "Synthesis mixed with beeps (0: disabled, 1: r2d2)", "0");
  cliParser.addOption("sv", "synthvolume", CliParser::CLI_FLOAT, true, "value", "Set volume of synth in DB related to beeps volume", "0.0");

  if (cliParser.parse(argc, argv) != true)
  {
    std::cerr << "" << std::endl;
    showVersion(0);
    std::cerr << cliParser.generateUsageMessage();
    return 0;
  }

  //Should be equal to the one in Globals::durToken
  double durToken = Globals::durToken; //dur in seconds for each token

  //int bufferSize = 2048;
  //int bufferSize = 1024;
  int bufferSize = 128;

  int i = 0;

  clock_t total_start,total_end;
  total_start = clock();

  //const int param_mode = cliParser.getOptionAsInt("m", 2);
  const int param_mode = 2;
  std::string inputFnStr = cliParser.getOptionAsString("f", "");
  std::string keyStr = cliParser.getOptionAsString("k", "");
  const float duration = cliParser.getOptionAsFloat("d", 60.0);
  const float interval = cliParser.getOptionAsFloat("i", 10.0);
  float startTime = cliParser.getOptionAsFloat("s", 5.0);
  std::string outputFnStr = cliParser.getOptionAsString("o", "");

  const int mixmode = cliParser.getOptionAsInt("x", 0);
  const float volumebeeps = cliParser.getOptionAsFloat("v", -3.f);
  const float volumeprogram = cliParser.getOptionAsFloat("p", 0.f);

  //double sampleRate = 44100.0;
  //float sampleRate = 22050.f;
  const float sampleRate = cliParser.getOptionAsFloat("r", 44100.0);

  const int loudnessStats = cliParser.getOptionAsInt("l", 0);

  const float baseFreq = cliParser.getOptionAsFloat("bf", 12000.0);
  const int tonesSeparation = cliParser.getOptionAsInt("ts", 1);

  const int synthMode = cliParser.getOptionAsInt("sm", 0);
  const float synthVolume = cliParser.getOptionAsFloat("sv", 0.0);

  //CHECK THAT PARAMETERS ARE CORRECT
  if (keyStr.size() != 5)
  {
    std::cerr << "Wrong key. Please use a 5 characters only key" << std::endl;
    return -1;
  }
  else //check that digits are valid
  {
    for (int i = 0; i < keyStr.size(); i++)
    {
      if (charToVal(keyStr.c_str()[i]) == -1)
      {
        std::cerr << "Wrong character in key [" << keyStr.c_str()[i] << "]. Please use digits in {0-9, a-v} range only" << std::endl;
        return -1;
      }
    }
  }

  float min_startTime = (durToken*20.f) + 0.1f;
  float min_interval = (durToken*20.f) + 0.2f;
  float min_duration = startTime + 0.1f;

  if (interval < min_interval)
  {
    std::cerr << "Interval too short. The minimum allowed interval is " << min_interval << " seconds" << std::endl;
    return -1;
  }
  /*if (interval > duration)
  {
    std::cerr << "Interval too big. Interval should be < duration" << std::endl;
    return -1;
  }*/
  if (duration < min_duration)
  {
    std::cerr << "Duration too short. The minimum allowed duration is " << min_duration << " seconds (start time + 0.1 seconds)" << std::endl;
    return -1;
  }
  if (duration > 86400.f) //24 hours of wav 44Khz 16 bits mono is 7.620.480.000 bytes
  { //1 hour is 317.520.000 bytes (317 Mbytes)
    std::cerr << "Duration too big. The maximum allowed duration is 86400 seconds (=24 hours)" << std::endl;
    return -1;
  }
  if ((startTime > duration) || (startTime < min_startTime)) //valid start time for first audio mark
  {
    std::cerr << "Start time is not valid. It should be > " << min_startTime << " secs." << std::endl;
    return -1;
  }

  startTime = MAX(startTime, min_startTime);

  //enum BEEPING_MODE { BEEPING_MODE_AUDIBLEOLD = 0, BEEPING_MODE_NONAUDIBLEOLD = 1, BEEPING_MODE_AUDIBLE = 2, BEEPING_MODE_NONAUDIBLE = 3, BEEPING_MODE_HIDDEN = 4, BEEPING_MODE_ALL = 5, BEEPING_MODE_CUSTOM = 6 };
  int mode = /*BEEPING_MODE::*/BEEPING_MODE_NONAUDIBLE; //2 audible, 3 non-audible
  if (param_mode == 0)
    mode = /*BEEPING_MODE::*/BEEPING_MODE_AUDIBLE;
  else if (param_mode == 1)
    mode = /*BEEPING_MODE::*/BEEPING_MODE_HIDDEN;
  else if (param_mode == 2)
    mode = /*BEEPING_MODE::*/BEEPING_MODE_NONAUDIBLE;
  else if (param_mode == 3)
    mode = /*BEEPING_MODE::*/BEEPING_MODE_CUSTOM;


  //Creation
  mBeepingCore = BEEPING_Create();

  if (param_mode == 3)
  {
    //float baseFreq = 100.f;
    //int tonesSeparation = 25;
    BEEPING_SetCustomBaseFreq(baseFreq, tonesSeparation, mBeepingCore);
  }

  BEEPING_SetSynthMode(synthMode, mBeepingCore);
  BEEPING_SetSynthVolume(synthVolume, mBeepingCore);


  //OUTPUT FILE
  SF_INFO sfinfoOutput;
  memset(&sfinfoOutput, '\0', sizeof(sfinfoOutput));
  SNDFILE *pWaveFileOutput = NULL;

  if (inputFnStr.size() == 0) //NO INPUT AUDIO, ONLY GENERATE BEEPS
  {
    //Configuration
    BEEPING_Configure(mode, sampleRate, bufferSize, mBeepingCore);

    //CREATE OUTPUT AUDIO FILE
    sfinfoOutput.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
    sfinfoOutput.channels = 1; //or 2
    sfinfoOutput.frames = 0;
    sfinfoOutput.samplerate = (int)sampleRate;

    pWaveFileOutput = sf_open(outputFnStr.c_str(), SFM_WRITE, &sfinfoOutput);
    if (!pWaveFileOutput)
    {
      std::cerr << "Cannot create Output WaveFile " << outputFnStr.c_str() << std::endl;
      return -1;
    }

    double currentTimeInSeconds = 0.0;
    double nextMarkTime = currentTimeInSeconds + startTime;

    float defBeepLevel = pow(10.f, volumebeeps / 20.f);

    //PINK NOISE
    //VRand rand;
    //rand.seed();
  //float w = rand.white(); // returns white noise +- 0.5
    //float p = rand.pink();  // returns pink noise  +- 0.5
  // float b = rand.brown(); // returns brown noise +- 0.5

    //ENCODE *******************************************************
    float *silenceBuffer = new float[bufferSize];
    memset(silenceBuffer, 0, bufferSize * sizeof(float));

    float *audioBuffer = new float[bufferSize];

    int progress_beeps = 0;
    std::cout << "Progress BEEPS = " << progress_beeps << std::endl;

    while (currentTimeInSeconds < duration)
    {
      float current_progress_beeps = (currentTimeInSeconds / duration)*100.f;
      if (current_progress_beeps > progress_beeps + 5)
      {
        progress_beeps = current_progress_beeps;
        std::cout << "Progress BEEPS = " << progress_beeps << std::endl;
      }

      if (currentTimeInSeconds >= (nextMarkTime - (durToken*20.f)))
      {
        int timestampInSeconds = (int)(nextMarkTime + 0.5f);
        //char timestamp[33]; //itoa(timestampInSeconds,timestamp,32);

        char* timestamp;
        timestamp = fromDecToBase(timestampInSeconds, 32);

        char currentTimestamp[5] = "\0";
        int len = strlen(timestamp);
        for (int i = len; i < 4; i++)
        {
          strcat(currentTimestamp, "0");
        }
        strcat(currentTimestamp, timestamp);

        //int num = fromBaseToDec(currentTimestamp, 4, 32);

        char stringToDecode[10];
        sprintf(stringToDecode, "%s%s", keyStr.c_str(), currentTimestamp);

        int size = strlen(stringToDecode);
        //int type = synthMode; //0 for only tones, 1 for tones + R2D2 sound, 2 for melody
        int type = Globals::synthMode; //0 for only tones, 1 for tones + R2D2 sound, 2 for melody
        int sizeAudioBuffer = BEEPING_EncodeDataToAudioBuffer(stringToDecode, size, type, 0, 0, mBeepingCore);

        int samplesRetrieved = 0;

        do
        {
          memset(audioBuffer, 0, bufferSize * sizeof(float));
          samplesRetrieved = BEEPING_GetEncodedAudioBuffer(audioBuffer, mBeepingCore);

          //mix with pink noise
          /*for (int i=0;i<bufferSize;i++)
          {
            //audioBuffer[i] = rand.pink();
            audioBuffer[i] = rand.brown();
          }*/

          //adjust volume of beeps based on parameter volumebeeps
          for (int i = 0; i < bufferSize; i++)
          {
            audioBuffer[i] = defBeepLevel * audioBuffer[i];
          }

          int count = (int)sf_write_float(pWaveFileOutput, audioBuffer, samplesRetrieved);

          currentTimeInSeconds = currentTimeInSeconds + (double)samplesRetrieved / sampleRate;
          //currentTimeInSeconds = currentTimeInSeconds + bufferSize/sampleRate;
        } while (samplesRetrieved > 0);

        //add silence at the end of file
        //memset(silenceBuffer,0,silenceSize*sizeof(float));
        //sf_write_float(pWaveFileOutput, silenceBuffer, silenceSize);

        BEEPING_ResetEncodedAudioBuffer(mBeepingCore);

        nextMarkTime += interval;
      }
      else
      {
        //add silence between marks
        sf_write_float(pWaveFileOutput, silenceBuffer, bufferSize);
        currentTimeInSeconds = currentTimeInSeconds + bufferSize / sampleRate;
      }
    }

    delete[] silenceBuffer;
    delete[] audioBuffer;

    std::cout << "Progress BEEPS = " << 100 << std::endl;
  }
  else //MIX WITH INPUT AUDIO
  {
    //READ INPUT FILE TO BUFFER
    SNDFILE *pWaveFileInput = NULL;
    SF_INFO sfinfoInput;
    memset(&sfinfoInput, '\0', sizeof(sfinfoInput));
    float **ppInputBuffer = NULL;

    pWaveFileInput = sf_open(inputFnStr.c_str(), SFM_READ, &sfinfoInput);

    long nFrames;
    int nch;
    float sampleRate;
    int buffersamples = 4096;

    if (pWaveFileInput) // read Input File to buffer
    {
      nFrames = sfinfoInput.frames;
      nch = (int)sfinfoInput.channels;
      sampleRate = (float)sfinfoInput.samplerate;

      if ((sampleRate != 44100.f) && (sampleRate != 48000.f))
      {
        printf("%s is not a valid Wav File! Please use 44.1Khz or 48Khz 16bits PCM Wave File\n", inputFnStr.c_str());
        sf_close(pWaveFileInput);
        return -2;
      }

      //Configuration
      BEEPING_Configure(mode, sampleRate, bufferSize, mBeepingCore);

      //CREATE OUTPUT AUDIO FILE
      sfinfoOutput.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
      sfinfoOutput.channels = 1; //or 2
      sfinfoOutput.frames = 0;
      sfinfoOutput.samplerate = (int)sampleRate;

      pWaveFileOutput = sf_open(outputFnStr.c_str(), SFM_WRITE, &sfinfoOutput);
      if (!pWaveFileOutput)
      {
        std::cerr << "Cannot create Output WaveFile " << outputFnStr.c_str() << std::endl;
        return -1;
      }


      //float *pInputBufferInterleaved = new float[nFrames*nch];
      float *pInputBufferInterleaved = new float[buffersamples*nch];

      ppInputBuffer = new float*[nch];
      for (int i = 0; i < nch; i++)
        ppInputBuffer[i] = new float[nFrames];

      int readsamples = 0;
      while (readsamples < nFrames*nch)
      {
        //int ReadCount = (int)sf_read_float(pWaveFileInput, pInputBufferInterleaved, nFrames*nch);
        int ReadCount = (int)sf_read_float(pWaveFileInput, pInputBufferInterleaved, buffersamples*nch);

        //Copy from interleaved to buffers
        for (int t = 0; t < nch; t++)
        {
          for (int i = 0; i < (ReadCount/nch); i++)
          {
            ppInputBuffer[t][(readsamples/nch)+i] = pInputBufferInterleaved[i*nch + t];
          }
        }

        readsamples += ReadCount;
      }

      //Delete interleaved
      delete[] pInputBufferInterleaved;
      sf_close(pWaveFileInput);

    }
    else
    {
      printf("%s is not a valid Wav File or file not found! Please use 44.1Khz or 48Khz 16bits PCM Wave File\n", inputFnStr.c_str());
      return -2;
    }

    //CALCULATE BEEPS IN BUFFER

    int progress_beeps = 0;
    std::cout << "Progress BEEPS = " << progress_beeps << std::endl;

    float *pBeepsBuffer = new float[nFrames + (int)((durToken*20.f)*sampleRate)]; //added duration of one beep message just to avoid buffer overflow when beep starts at the end of file (not sure it is needed though)
    memset(pBeepsBuffer, 0, nFrames*sizeof(float));

    long counterSamples = 0;
    double currentTimeInSeconds = 0.0;
    double nextMarkTime = currentTimeInSeconds + startTime;
    float input_duration = nFrames / sampleRate;
    while (currentTimeInSeconds < (input_duration-(bufferSize/sampleRate))) //todo:
    {
      float current_progress_beeps = (currentTimeInSeconds / input_duration)*100.f;
      if (current_progress_beeps > progress_beeps + 5)
      {
        progress_beeps = current_progress_beeps;
        std::cout << "Progress BEEPS = " << progress_beeps << std::endl;
      }

      if (currentTimeInSeconds >= (nextMarkTime - (durToken*20.f)))
      {
        int timestampInSeconds = (int)(nextMarkTime + 0.5f);
        //char timestamp[33]; //itoa(timestampInSeconds,timestamp,32);

        char* timestamp;
        timestamp = fromDecToBase(timestampInSeconds, 32);

        char currentTimestamp[5] = "\0";
        int len = strlen(timestamp);
        for (int i = len; i < 4; i++)
        {
          strcat(currentTimestamp, "0");
        }
        strcat(currentTimestamp, timestamp);

        //int num = fromBaseToDec(currentTimestamp, 4, 32);

        char stringToDecode[10];
        sprintf(stringToDecode, "%s%s", keyStr.c_str(), currentTimestamp);

        int size = strlen(stringToDecode);
        int type = synthMode; //0 for only tones, 1 for tones + R2D2 sound, 2 for melody
        int sizeAudioBuffer = BEEPING_EncodeDataToAudioBuffer(stringToDecode, size, type, 0, 0, mBeepingCore);

        int samplesRetrieved = 0;

        do
        {
          //memset(audioBuffer, 0, bufferSize * sizeof(float));
          samplesRetrieved = BEEPING_GetEncodedAudioBuffer(pBeepsBuffer+counterSamples, mBeepingCore);

          counterSamples += samplesRetrieved;

          //mix with pink noise
          /*for (int i=0;i<bufferSize;i++)
          {
          //audioBuffer[i] = rand.pink();
          audioBuffer[i] = rand.brown();
          }*/

          //int count = (int)sf_write_float(pWaveFileOutput, audioBuffer, samplesRetrieved);

          currentTimeInSeconds = currentTimeInSeconds + (double)samplesRetrieved / sampleRate;
          //currentTimeInSeconds = currentTimeInSeconds + bufferSize/sampleRate;
        } while (samplesRetrieved > 0);

        //add silence at the end of file
        //memset(silenceBuffer,0,silenceSize*sizeof(float));
        //sf_write_float(pWaveFileOutput, silenceBuffer, silenceSize);

        BEEPING_ResetEncodedAudioBuffer(mBeepingCore);

        nextMarkTime += interval;
      }
      else
      {
        //add silence between marks
        //sf_write_float(pWaveFileOutput, silenceBuffer, bufferSize);
        counterSamples += bufferSize;
        currentTimeInSeconds = currentTimeInSeconds + bufferSize / sampleRate;
      }
    }

    std::cout << "Progress BEEPS = " << 100 << std::endl;

    //MIX BUFFERS
    Mixer mixer;
    mixer.setBeepLevel(volumebeeps);
    mixer.setMinBeepLevel(-20.f);
    mixer.setProgramLevel(volumeprogram);
    //mixer.setSmoothTime(float time);
    mixer.setMode(mixmode);
    mixer.setUseNormalize(false);

    float **ppMixedBuffer = new float*[nch];
    for (int i = 0; i < nch; i++)
      ppMixedBuffer[i] = new float[nFrames];

    //mixer.mix(const float** bufferPgm, const int nsamples, int nchannels, const float samplerate, const float* bufferBeeps, float** bufferMix);
    mixer.mix((const float**)ppInputBuffer, nFrames, nch, sampleRate, pBeepsBuffer, ppMixedBuffer);

    //Bypass
    /*for (int t=0;t<nch;t++)
    {
      for (int i = 0; i < nFrames; i++)
      {
        ppMixedBuffer[t][i] = (ppInputBuffer[t][i]+pBeepsBuffer[i]);
      }
    }*/
    delete[] pBeepsBuffer;

    //WRITE MIXED AUDIO TO OUTPUT FILE
    int progress_save = 0;
    std::cout << "Progress SAVE = " << progress_save << std::endl;
    sfinfoInput.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
    pWaveFileOutput = sf_open(outputFnStr.c_str(), SFM_WRITE, &sfinfoInput);
    if (!pWaveFileOutput)
    {
      if (ppInputBuffer)
      {
        for (int i = 0; i < nch; i++)
        {
          delete[] ppInputBuffer[i];
        }
        delete[] ppInputBuffer;
      }
      ppInputBuffer = NULL;

      if (ppMixedBuffer)
      {
        for (int i = 0; i < nch; i++)
        {
          delete[] ppMixedBuffer[i];
        }
        delete[] ppMixedBuffer;
      }
      ppMixedBuffer = NULL;

      printf("Cannot create Output WaveFile %s!\n", outputFnStr.c_str());

      return -4;
    }
    else
    {
      int buffersamples = 4096;
      float *pOutputBufferInterleaved = new float[buffersamples*nch];

      int samplesread = 0;

      while (samplesread < nFrames)
      {
        float current_progress_save = ((float)samplesread / (float)nFrames)*100.f;
        if (current_progress_save > progress_save + 5)
        {
          progress_save = current_progress_save;
          std::cout << "Progress SAVE = " << progress_save << std::endl;
        }

        int samplesToWrite = MIN(buffersamples, nFrames - samplesread);

        for (int t = 0; t < nch; t++)
        {
          for (int i = 0; i < samplesToWrite; i++)
          {
            pOutputBufferInterleaved[i*nch + t] = ppMixedBuffer[t][samplesread+i];
          }
        }

        int count = (int)sf_write_float(pWaveFileOutput, pOutputBufferInterleaved, samplesToWrite*nch);

        samplesread += samplesToWrite;
      }

      delete[] pOutputBufferInterleaved;

      if (ppMixedBuffer)
      {
        for (int i = 0; i < nch; i++)
        {
          delete[] ppMixedBuffer[i];
        }
        delete[] ppMixedBuffer;
      }
      ppMixedBuffer = NULL;

      sf_close(pWaveFileOutput);

    }

    std::cout << "Progress SAVE = " << 100 << std::endl;

  }



  if (loudnessStats == 1)
  {
    std::cout << "STATISTICS: " << std::endl;

    double lufs = test_global_loudness(outputFnStr.c_str());
    std::cout << " Lufs:      " << lufs << " dB" << std::endl;

    double tp = test_true_peak(outputFnStr.c_str());
    std::cout << " True Peak: " << tp << " dB" << std::endl;

    double bf = BEEPING_GetDecodingBeginFreq(mBeepingCore);
    std::cout << " Begin Freq: " << bf << " Hz" << std::endl;
    double ef = BEEPING_GetDecodingEndFreq(mBeepingCore);
    std::cout << " End Freq:   " << ef << " Hz" << std::endl;
  }

  //Destroy
  BEEPING_Destroy(mBeepingCore);

  sf_close(pWaveFileOutput);

  total_end = clock();
  double totalDuration = double(total_end - total_start) / (double)CLOCKS_PER_SEC;
  std::cout << "Total Duration: " << totalDuration << " secs" << std::endl;

  return 0;
}
