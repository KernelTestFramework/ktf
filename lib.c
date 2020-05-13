#include <ktf.h>
#include <lib.h>

void __noreturn halt(void) {
    cli();

    while(1) {
        hlt();
        pause();
    }
}
