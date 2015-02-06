x = [4 16 64 256 1024 4096 16386 65536 262144 524288];
y1 = [1726 1550 1621 1548 1688 1998 4924 12898 58051 117564];
y2 = [4493 3599 3393 3443 3540 5049 6220 15684 52584 107334];
y3 = [1939 1616 1616 1573 1629 1823 2650 6134 19274 36160];

figure
loglog(x, y1, 'r-+', x, y2, 'b--o', x, y3, 'g:x')

xlabel('Message size (bytes)')
ylabel('Latency (ns)')
legend('pipe', 'socket', 'mmap')

print -deps 'latency.eps'
