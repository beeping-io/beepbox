/*--------------------------------------------------------------------------------
 LoudnessStats.cpp
 Version 1.0.0
 Apache Lisence 2.0
 --------------------------------------------------------------------------------*/

#include <LoudnessStats.h>

#include <sndfile.h>
#include <ebur128.h>

#include <stdlib.h>
#include <string.h>
#include <math.h>

double test_global_loudness(const char* filename) {
  SF_INFO file_info;
  SNDFILE* file;
  sf_count_t nr_frames_read;

  ebur128_state* st = NULL;
  double gated_loudness;
  double* buffer;

  memset(&file_info, '\0', sizeof(file_info));
  file = sf_open(filename, SFM_READ, &file_info);
  if (!file) {
    fprintf(stderr, "Could not open file %s!\n", filename);
    return 0.0;
  }
  st = ebur128_init((unsigned)file_info.channels,
    (unsigned)file_info.samplerate,
    EBUR128_MODE_I);
  if (file_info.channels == 5) {
    ebur128_set_channel(st, 0, EBUR128_LEFT);
    ebur128_set_channel(st, 1, EBUR128_RIGHT);
    ebur128_set_channel(st, 2, EBUR128_CENTER);
    ebur128_set_channel(st, 3, EBUR128_LEFT_SURROUND);
    ebur128_set_channel(st, 4, EBUR128_RIGHT_SURROUND);
  }
  buffer = (double*)malloc(st->samplerate * st->channels * sizeof(double));
  while ((nr_frames_read = sf_readf_double(file, buffer,
    (sf_count_t)st->samplerate))) {
    ebur128_add_frames_double(st, buffer, (size_t)nr_frames_read);
  }

  ebur128_loudness_global(st, &gated_loudness);

  /* clean up */
  ebur128_destroy(&st);

  free(buffer);
  buffer = NULL;
  if (sf_close(file)) {
    fprintf(stderr, "Could not close input file!\n");
  }
  return gated_loudness;
}


double test_true_peak(const char* filename) {
  SF_INFO file_info;
  SNDFILE* file;
  sf_count_t nr_frames_read;
  int i;

  ebur128_state* st = NULL;
  double true_peak;
  double max_true_peak = -HUGE_VAL;
  double* buffer;

  memset(&file_info, '\0', sizeof(file_info));
  file = sf_open(filename, SFM_READ, &file_info);
  if (!file) {
    fprintf(stderr, "Could not open file %s!\n", filename);
    return 0.0;
  }
  st = ebur128_init((unsigned)file_info.channels,
    (unsigned)file_info.samplerate,
    EBUR128_MODE_TRUE_PEAK);
  if (file_info.channels == 5) {
    ebur128_set_channel(st, 0, EBUR128_LEFT);
    ebur128_set_channel(st, 1, EBUR128_RIGHT);
    ebur128_set_channel(st, 2, EBUR128_CENTER);
    ebur128_set_channel(st, 3, EBUR128_LEFT_SURROUND);
    ebur128_set_channel(st, 4, EBUR128_RIGHT_SURROUND);
  }
  buffer = (double*)malloc(st->samplerate * st->channels * sizeof(double));
  while ((nr_frames_read = sf_readf_double(file, buffer,
    (sf_count_t)st->samplerate))) {
    ebur128_add_frames_double(st, buffer, (size_t)nr_frames_read);
  }

  for (i = 0; i < file_info.channels; i++) {
    ebur128_true_peak(st, (unsigned)i, &true_peak);
    if (true_peak > max_true_peak)
      max_true_peak = true_peak;
  }
  /* clean up */
  ebur128_destroy(&st);

  free(buffer);
  buffer = NULL;
  if (sf_close(file)) {
    fprintf(stderr, "Could not close input file!\n");
  }
  return 20 * log10(max_true_peak);
}

