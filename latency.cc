#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <cstring>
#include <sys/wait.h>
#include <cstdlib>
#include <sys/time.h>
#include <time.h>

using namespace std;

long long to_nano(timespec t) {
    return 1000000000LL * t.tv_sec + t.tv_nsec;
}

long long diff(timespec t0, timespec t1) {
    return to_nano(t1) - to_nano(t0);
}

void test_pipe_latency() {
    // Lessons leanred:
    // 1. Do not use rdtsc! Waiting does not count in CPU time.
    // 2. Warm-up cost is expensive!
    int sizes[] = {1024 * 1024, 4, 16, 64, 256, 1024, 4 * 1024, 16 * 1024, 64 * 1024, 256 * 1024, 512 * 1024};
    int nsizes = sizeof(sizes) / sizeof(int);
    char buffer[1024 * 1024];
    int nMBs = 32;

    int parent_to_child[2],
        child_to_parent[2];
    if (0 != pipe(parent_to_child))
        cout << "error" << endl;
    if (0 != pipe(child_to_parent))
        cout << "error" << endl;

    pid_t pid = fork();
    if (pid == -1)
        cout << "error" << endl;

    if (pid == 0) {
        // child
        close(parent_to_child[1]);
        close(child_to_parent[0]);

        // latency
        for (int i = 0; i < nsizes; ++i) {
            read(parent_to_child[0], buffer, sizes[i]);
            write(child_to_parent[1], buffer, sizes[i]);
        }
        // throughput
        for (int k = 1; k <= 32; k *= 2) {
            for (int i = 0; i < k * nMBs; ++i)
                read(parent_to_child[0], buffer, 1024 * 1024);
            write(child_to_parent[1], buffer, 1);
        }

        close(parent_to_child[0]);
        close(child_to_parent[1]);

        exit(0);
    } else {
        // parent
        close(parent_to_child[0]);
        close(child_to_parent[1]);

        // latency
        for (int i = 0; i < nsizes; ++i) {
            memset(buffer, 0, sizes[i]);

            struct timespec t0, t1;

            clock_gettime(CLOCK_REALTIME, &t0);

            write(parent_to_child[1], buffer, sizes[i]);
            read(child_to_parent[0], buffer, sizes[i]);
            clock_gettime(CLOCK_REALTIME, &t1);

            cout << "latency " << sizes[i] << ": " << diff(t0, t1) << endl;
        }
        //throughput
        for (int k = 1; k <= 32; k *= 2) {
            memset(buffer, 0, 1024 * 1024);

            struct timespec t0, t1;

            clock_gettime(CLOCK_REALTIME, &t0);

            for (int i = 0; i < k * nMBs; ++i)
                write(parent_to_child[1], buffer, 1024 * 1024);
            read(child_to_parent[0], buffer, 1);

            clock_gettime(CLOCK_REALTIME, &t1);

            cout << "throughput " << k * nMBs << "MB: " << diff(t0, t1) << endl;
        }

        close(parent_to_child[1]);
        close(child_to_parent[0]);

        waitpid(pid, NULL, 0);
    }
}

void test_socket_latency() {
}

int main() {
    test_pipe_latency();
    return 0;
}
