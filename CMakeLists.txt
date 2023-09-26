cmake_minimum_required(VERSION 3.21)
project(byu_cs324 C)

set(CMAKE_C_STANDARD 11)

include_directories(09a-hw-concurrency)
include_directories(10-lab-proxy-threadpool/tiny)
include_directories(12-lab-proxy-iomultiplex/tiny)

add_executable(byu_cs324
        01c-hw-remote/hello.c
        01d-hw-strings-io-env/learn_c.c
        03-hw-fork-exec/exec.c
        03-hw-fork-exec/fork.c
        04-lab-shell-pipeline/myintgroup.c
        04-lab-shell-pipeline/myppid.c
        04-lab-shell-pipeline/tsh.c
        05-hw-signals/killer.c
        05-hw-signals/signals.c
        06-lab-shell-signals/myint.c
        06-lab-shell-signals/myspin.c
        06-lab-shell-signals/mysplit.c
        06-lab-shell-signals/mystop.c
        06-lab-shell-signals/tsh.c
        07-hw-sockets/client.c
        07-hw-sockets/server.c
        08-lab-socket-hunt/treasure_hunter.c
        09a-hw-concurrency/csapp.c
        09a-hw-concurrency/csapp.h
        09a-hw-concurrency/echo.c
        09a-hw-concurrency/echo_cnt.c
        09a-hw-concurrency/echoserveri.c
        09a-hw-concurrency/echoserverp.c
        09a-hw-concurrency/echoservert.c
        09a-hw-concurrency/echoservert_pre.c
        09a-hw-concurrency/sbuf.c
        09a-hw-concurrency/sbuf.h
        10-lab-proxy-threadpool/tiny/cgi-bin/adder.c
        10-lab-proxy-threadpool/tiny/cgi-bin/hello.c
        10-lab-proxy-threadpool/tiny/cgi-bin/slow.c
        10-lab-proxy-threadpool/tiny/csapp.c
        10-lab-proxy-threadpool/tiny/csapp.h
        10-lab-proxy-threadpool/tiny/tiny.c
        10-lab-proxy-threadpool/proxy.c
        11a-hw-openmp/mandelbrot.c
        12-lab-proxy-iomultiplex/echoserver-epoll/echoservere.c
        12-lab-proxy-iomultiplex/tiny/cgi-bin/adder.c
        12-lab-proxy-iomultiplex/tiny/cgi-bin/hello.c
        12-lab-proxy-iomultiplex/tiny/cgi-bin/slow.c
        12-lab-proxy-iomultiplex/tiny/csapp.c
        12-lab-proxy-iomultiplex/tiny/csapp.h
        12-lab-proxy-iomultiplex/tiny/tiny.c
        12-lab-proxy-iomultiplex/proxy.c)
