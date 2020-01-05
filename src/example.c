#include "doops.h"

int main() {
    // create loop
    struct doops_loop *loop = loop_new();

    int times = 0;

    // schedule event every second (1000ms)
    loop_schedule(loop, {
        times ++;
        printf("Hello world! (%i)\n", times);
        // remove event
        if (times == 10)
            return 1;
    }, 1000);

    // run the loop
    loop_run(loop);

    loop_free(loop);
    return 0;
}
