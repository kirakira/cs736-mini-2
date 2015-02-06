#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <cstring>
#include <sys/wait.h>
#include <cstdlib>
#include <cstdio>
#include <sys/time.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/mman.h>
#include <fcntl.h>

using namespace std;

long long to_nano(timespec t) {
    return 1000000000LL * t.tv_sec + t.tv_nsec;
}

long long diff(timespec t0, timespec t1) {
    return to_nano(t1) - to_nano(t0);
}

int try_read(int fd, char *buffer, int size) {
    int r = 0;
    while (r < size) {
        int t = read(fd, buffer + r, size - r);
        if (t <= 0)
            return t;
        r += t;
    }
    return r;
}

int try_write(int fd, const char* buffer, int size) {
    int r = 0;
    while (r < size) {
        int t = write(fd, buffer + r, size - r);
        if (t <= 0)
            return t;
        r += t;
    }
    return r;
}

int sizes[] = {1024 * 1024, 4, 16, 64, 256, 1024, 4 * 1024, 16 * 1024, 64 * 1024, 256 * 1024, 512 * 1024};
int nsizes = sizeof(sizes) / sizeof(int);
int nMBs = 32;

void test_pipe_and_socket(bool use_pipe) {
    // Lessons leanred:
    // 1. Do not use rdtsc! Waiting does not count in CPU time.
    // 2. Warm-up cost is expensive!
    char buffer[1024 * 1024];
    int port = 2444;

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
        if (!use_pipe) {

            char c = 0;
            try_read(parent_to_child[0], &c, 1);

            sockaddr_in name;

            int sock = socket(AF_INET, SOCK_STREAM, 0);
            name.sin_family = AF_INET;
            name.sin_port = htons(port);

            if (inet_pton(AF_INET, "127.0.0.1", &name.sin_addr) <= 0)
                cout << "inet_pton" << endl;

            if (connect(sock, (struct sockaddr *) (&name), sizeof(name)) < 0)
                cout << "connect" << endl;

            int flag = 1;
            setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char *) &flag, sizeof(int));

            close(parent_to_child[0]);
            close(child_to_parent[1]);
            close(parent_to_child[1]);
            close(child_to_parent[0]);

            parent_to_child[1] = sock;
            child_to_parent[0] = sock;
            parent_to_child[0] = sock;
            child_to_parent[1] = sock;
        }

        // latency
        for (int i = 0; i < nsizes; ++i) {
            if (sizes[i] != try_read(parent_to_child[0], buffer, sizes[i]))
                cout << "read" << endl;
            if (sizes[i] != try_write(child_to_parent[1], buffer, sizes[i]))
                cout << "write" << endl;
        }
        // throughput
        for (int k = 1; k <= 32; k *= 2) {
            for (int i = 0; i < k * nMBs; ++i)
                if (1024 * 1024 != try_read(parent_to_child[0], buffer, 1024 * 1024))
                    cout << "read" << endl;
            if (1 != try_write(child_to_parent[1], buffer, 1))
                cout << "write" << endl;
        }

        try_read(parent_to_child[0], buffer, 1);

        close(parent_to_child[0]);
        close(child_to_parent[1]);
        close(parent_to_child[1]);
        close(child_to_parent[0]);

        exit(0);
    } else {
        // parent
        if (!use_pipe) {
            sockaddr_in name, from_name;
            unsigned int lfrom_name = sizeof(from_name);

            int sock = socket(AF_INET, SOCK_STREAM, 0);
            name.sin_family = AF_INET;
            name.sin_addr.s_addr = INADDR_ANY;
            name.sin_port = htons(port);
            if (bind(sock, (struct sockaddr *) (&name), sizeof(name)) == -1)
                cout << "bind" << endl;

            char c = 0;
            write(parent_to_child[1], &c, 1);

            if (listen(sock, 10) == -1)
                cout << "listen" << endl;

            int conn = accept(sock, (struct sockaddr *) (&from_name), &lfrom_name);
            if (conn == -1)
                cout << "accept" << endl;

            int flag = 1;
            setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char *) &flag, sizeof(int));

            close(parent_to_child[1]);
            close(child_to_parent[0]);
            close(parent_to_child[0]);
            close(child_to_parent[1]);
            parent_to_child[1] = conn;
            child_to_parent[0] = conn;
            parent_to_child[0] = sock;
            child_to_parent[1] = sock;
        }

        // latency
        for (int i = 0; i < nsizes; ++i) {
            memset(buffer, 0, sizes[i]);

            struct timespec t0, t1;

            clock_gettime(CLOCK_REALTIME, &t0);

            if (sizes[i] != try_write(parent_to_child[1], buffer, sizes[i]))
                cout << "write" << endl;
            try_read(child_to_parent[0], buffer, sizes[i]);
            clock_gettime(CLOCK_REALTIME, &t1);

            cout << "latency " << sizes[i] << ": " << diff(t0, t1) / 2 << endl;
        }
        //throughput
        for (int k = 1; k <= 32; k *= 2) {
            memset(buffer, 0, 1024 * 1024);

            struct timespec t0, t1;

            clock_gettime(CLOCK_REALTIME, &t0);

            for (int i = 0; i < k * nMBs; ++i)
                if (1024 * 1024 != try_write(parent_to_child[1], buffer, 1024 * 1024))
                    cout << "write" << endl;
            try_read(child_to_parent[0], buffer, 1);

            clock_gettime(CLOCK_REALTIME, &t1);

            cout << "throughput " << k * nMBs << "MB: " << diff(t0, t1) << endl;
        }

        if (1 != try_write(parent_to_child[1], buffer, 1))
            cout << "write" << endl;

        waitpid(pid, NULL, 0);

        close(parent_to_child[1]);
        close(child_to_parent[0]);
        close(parent_to_child[0]);
        close(child_to_parent[1]);
    }
}

struct shared_data {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    char buffer[1024 * 1024 * 1024];
    int state;
};

void my_copy(char *dst, const char* src, int size, int srcsize) {
    int done = 0;
    while (done < size) {
        int cur = min(size - done, srcsize);
        memcpy(dst + done, src, cur);
        done += cur;
    }
}

void test_mmap() {
    // Lessons learned:
    // 1. Use a state machine to synchronize.

    int prot = PROT_READ | PROT_WRITE;
    int flags = MAP_SHARED | MAP_ANONYMOUS;
    shared_data *data = (shared_data *) mmap(NULL, sizeof(shared_data), prot, flags, -1, 0);
    data->state = 0;

    pthread_mutexattr_t mutex_attr;
    pthread_mutexattr_init(&mutex_attr);
    pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&data->mutex, &mutex_attr);

    pthread_condattr_t cond_attr;
    pthread_condattr_init(&cond_attr);
    pthread_condattr_setpshared(&cond_attr, PTHREAD_PROCESS_SHARED);
    pthread_cond_init(&data->cond, &cond_attr);

    char buffer[1024 * 1024];
    memset(buffer, 0, sizeof(buffer));

    pid_t pid = fork();
    if (!pid) {
        // parent

        // latency
        for (int i = 0; i < nsizes; ++i) {
            pthread_mutex_lock(&data->mutex);
            while (data->state != 0)
                pthread_cond_wait(&data->cond, &data->mutex);

            struct timespec t0, t1;
            clock_gettime(CLOCK_REALTIME, &t0);

            my_copy(data->buffer, buffer, sizes[i], 1024 * 1024);
            data->state = 1;
            pthread_cond_signal(&data->cond);
            pthread_mutex_unlock(&data->mutex);

            pthread_mutex_lock(&data->mutex);
            while (data->state != 2)
                pthread_cond_wait(&data->cond, &data->mutex);
            clock_gettime(CLOCK_REALTIME, &t1);
            data->state = 0;
            pthread_mutex_unlock(&data->mutex);

            cout << "latency " << sizes[i] << ": " << diff(t0, t1) / 2 << endl;
        }

        // throughput
        for (int k = 1; k <= 32; k *= 2) {
            pthread_mutex_lock(&data->mutex);
            while (data->state != 0)
                pthread_cond_wait(&data->cond, &data->mutex);

            struct timespec t0, t1;
            clock_gettime(CLOCK_REALTIME, &t0);

            my_copy(data->buffer, buffer, k * nMBs * 1024 * 1024, 1024 * 1024);
            data->state = 1;
            pthread_cond_signal(&data->cond);
            pthread_mutex_unlock(&data->mutex);

            pthread_mutex_lock(&data->mutex);
            while (data->state != 2)
                pthread_cond_wait(&data->cond, &data->mutex);
            clock_gettime(CLOCK_REALTIME, &t1);
            data->state = 0;
            pthread_mutex_unlock(&data->mutex);

            cout << "throughput " << k * nMBs << "MB: " << diff(t0, t1) << endl;
        }

        waitpid(pid, NULL, 0);
    } else {
        // child

        // latency
        for (int i = 0; i < nsizes; ++i) {
            pthread_mutex_lock(&data->mutex);
            while (data->state != 1)
                pthread_cond_wait(&data->cond, &data->mutex);
            my_copy(data->buffer, buffer, sizes[i], 1024 * 1024);
            data->state = 2;
            pthread_cond_signal(&data->cond);
            pthread_mutex_unlock(&data->mutex);
        }

        for (int k = 1; k <= 32; k *= 2) {
            pthread_mutex_lock(&data->mutex);
            while (data->state != 1)
                pthread_cond_wait(&data->cond, &data->mutex);
            data->state = 2;
            pthread_cond_signal(&data->cond);
            pthread_mutex_unlock(&data->mutex);
        }

        exit(0);
    }

    pthread_cond_destroy(&data->cond);
    pthread_condattr_destroy(&cond_attr);
    pthread_mutex_destroy(&data->mutex);
    pthread_mutexattr_destroy(&mutex_attr);
}

int main() {
    cout << "pipe results" << endl;
    test_pipe_and_socket(true);
    cout << endl << "socket results" << endl;
    test_pipe_and_socket(false);
    cout << endl << "mmap results" << endl;
    test_mmap();
    return 0;
}
