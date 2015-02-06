x = [32 64 128 256 512 1024];
y1 = [3927.04 3939.22 3894.65 3890.49 3884.14 3858.14];
y2 = [6547.36 6400.77 6492.11 6616.29 6582.8 6616.43];
y3 = [2748.13 4266.3 4251.26 4257.74 4272.55 4322.51];

figure
semilogx(x, y1, 'r-+', x, y2, 'b--o', x, y3, 'g:x')

xlabel('Message size (Mbytes)')
ylabel('Throughput (MB/s)')
legend('pipe', 'socket', 'mmap')

print -deps 'throughput.eps'
