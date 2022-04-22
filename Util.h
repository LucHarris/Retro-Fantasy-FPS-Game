#pragma once
#include <algorithm>

float RandFloat(float low = 0.0f, float high = 1.0f)
{
  if (low > high)
  {
    std::swap(low, high);
  }

  return low + ((float)(rand() / (float)(RAND_MAX / (high - low))));
}