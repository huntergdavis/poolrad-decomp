/* Canary: C tests must run with assert() live. A build that defines NDEBUG for a test target (a Release build without
 * the gate at the end of CMakeLists.txt) would compile every assert() away and let the suite pass blind; this file
 * refuses to build in that case, and at run time proves that assert() really aborts. */
#ifdef NDEBUG
#error "NDEBUG is defined for a C test target: assert() would be compiled out (see the -UNDEBUG gate in CMakeLists.txt)"
#endif
#include <assert.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void)
{
    pid_t child = fork();
    if (child < 0) {
        perror("fork");
        return 2;
    }
    if (child == 0) {
        fclose(stderr);       /* keep the deliberate assertion message out of the test log */
        assert(0 && "canary");
        _exit(0);             /* reached only if assert() is a no-op */
    }
    int status = 0;
    if (waitpid(child, &status, 0) != child)
        return 2;
    if (!(WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT)) {
        fprintf(stderr, "assert(0) did not abort: C tests are running blind\n");
        return 1;
    }
    puts("assert() is live in C tests");
    return 0;
}
