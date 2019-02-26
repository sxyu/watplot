#pragma once
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <iomanip>
#include <vector>
#include <set>
#include <map>
#include <unordered_map>
#include <deque>
#include <algorithm>
#include <numeric>
#include <random>
#include <chrono>
#include <cctype>
#include <thread>
#include <future>
#include <iterator>
#include <climits>
#include <cfloat>
#include <cmath>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <unistd.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <Eigen/Core>
#include <Eigen/Dense>

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include "consts.hpp"

// compatibility
#ifndef max
    #define max(x, y) (((x) > (y)) ? (x) : (y))
#endif
#ifndef min
    #define min(x, y) (((x) < (y)) ? (x) : (y))
#endif
#ifndef M_PI
    #define M_PI 3.14159265358979323846264338327950288
#endif
