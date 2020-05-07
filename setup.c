#include <ktf.h>
#include <page.h>

/*
 * KTF Stack layout:
 *
 * kernel_stack[page 3] Emergency stack
 * kernel_stack[page 2] Exception stack
 * kernel_stack[page 1] Regular stack
 */
uint8_t kernel_stack[3 * PAGE_SIZE] __aligned(PAGE_SIZE) __data;
