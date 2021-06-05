// how often does one need to do/store a measurement of a temporal variable
// when one wants to be sure to capture all changes which exceed the just
// noticable different of the variable, but not (much) more often that?

// all times in seconds -- all intervals should be powers of the lowest one
#define MIN_INTERVAL 5
#define RESET_INTERVAL 20 // should be 10
// 20 40 80 160 320 640
#define MAX_INTERVAL 12*60 // 12 minutes

// JND of Lux is 7%

// JND of Kelvin is ~100 until 4000, ~500 above 5000
// https://www.elementalled.com/correlated-color-temperature-and-kelvin/

// if (newest - mean) > jnd -> RESET_INTERVAL
// else look at sd : mean ratio
// sd / mean > ? -> HALF interval
// sd / mean < ? -> DOUBLE interval
