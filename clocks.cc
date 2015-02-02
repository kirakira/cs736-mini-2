#include <iostream>
#include <cstring>
#include <vector>
#include <cmath>
#include <ctime>
#include <sys/time.h>
#include "rdtsc.h"

using namespace std;

template<int N>
inline void repeat(double &result) {
    result += 1/result;
    repeat<N-1>(result);
};

template<>
inline void repeat<0>(double &result) {
    // do nothing
}

// gettimeofday
template<int d>
inline int test_gettimeofday(double &result) {
    struct timeval t0, t1;
    gettimeofday(&t0, NULL);
    repeat<d>(result);
    gettimeofday(&t1, NULL);

    if (memcmp(&t0, &t1, sizeof(t0)))
        return d;
    return test_gettimeofday<d+1>(result);
}

template<>
inline int test_gettimeofday<100>(double &result) {
    return 100;
}

int test_gettimeofday(double &result) {
    return test_gettimeofday<0>(result);
}

// clock_gettime

template<int d>
inline int test_clock_gettime(double &result) {
    struct timespec t0, t1;

    clock_gettime(CLOCK_REALTIME, &t0);
    repeat<d>(result);
    clock_gettime(CLOCK_REALTIME, &t1);

    if (memcmp(&t0, &t1, sizeof(t0)))
        return d;
    return test_clock_gettime<d+1>(result);
}

template<>
inline int test_clock_gettime<100>(double &result) {
    return 100;
}

int test_clock_gettime(double &result) {
    return test_clock_gettime<0>(result);
}

// rdtsc

template<int d>
inline int test_rdtsc(double &result) {
    unsigned long long t0, t1;

    t0 = rdtsc();
    repeat<d>(result);
    t1 = rdtsc();

    cout << t1 - t0 << endl;

    if (t0 != t1)
        return d;
    return test_rdtsc<d+1>(result);
}

template<>
inline int test_rdtsc<100>(double &result) {
    return 100;
}

int test_rdtsc(double &result) {
    return test_rdtsc<0>(result);
}

// end

double avg(const vector<int> &data) {
    double sum = 0;
    for (int i = 0; i < data.size(); ++i)
        sum += data[i];
    return sum / data.size();
}

double sd(const vector<int> &data) {
    double sum = 0, mean = avg(data);
    for (int i = 0; i < data.size(); ++i) {
        double t = (data[i] - mean);
        sum += t * t;
    }
    return sqrt(sum / (data.size() - 1));
}

typedef int (*single_test)(double &);

void test(const char *title, single_test test_function, double &result) {
    cout << title << endl;
    vector<int> data;
    for (int i = 0; i < 100; ++i) {
        if (i > 0)
            cout << ",";
        data.push_back(test_function(result));
        cout << data.back();
    }
    cout << endl;
    cout << "avg: " << avg(data) << ", variance: " << sd(data) << endl << endl;
}

int main(void) {
    struct timespec t0, t1;
    clock_gettime(CLOCK_REALTIME, &t0);
    clock_gettime(CLOCK_REALTIME, &t1);
    cout << t1.tv_nsec - t0.tv_nsec << endl;

    double result = 1;
    test("gettimeofday", &test_gettimeofday, result);
    test("clock_gettime", &test_clock_gettime, result);
    test("rdtsc", &test_rdtsc, result);
    cout << result << endl;
    return 0;
}
