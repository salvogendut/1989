/* Exercise the private page-buffer lifecycle without a guest or host printer. */
#include "../src/printer.c"
#include <assert.h>

static int pages;
void Grab_Print(uint8_t *data, int width, int height, int dpi) {
    assert(width == 32 && height == 2 && dpi == 400);
    assert(data[0] == 0xa5 && data[7] == 0x5a);
    pages++;
}

int main(void) {
    print_dpi = 400;
    lp_print_start(1 << 16); /* four bytes per row */
    assert(print_data);
    lp_buffer.size = 8;
    memset(lp_buffer.data, 0, 8);
    lp_buffer.data[0] = 0xa5;
    lp_buffer.data[7] = 0x5a;
    lp_print_data();
    lp_print_finish();
    lp_print_finish(); /* finishing twice must not emit a second page */
    assert(pages == 1 && print_data == NULL);

    /* A new zero-width page cancels an in-progress page. It must not
     * retain a freed buffer or divide by zero when the printer shuts down. */
    lp_print_start(1 << 16);
    lp_print_data();
    lp_print_start(0);
    lp_print_data();
    lp_print_finish();
    assert(pages == 1 && print_data == NULL);
    puts("test-printer: OK");
    return 0;
}
