#include <pcap.h>
#include <stddef.h>
int pcap_test_mode, pcap_test_freed;
static pcap_if_t second = {NULL, "test1", "Second test interface"};
static pcap_if_t first = {&second, "test0", "First test interface"};
int pcap_findalldevs(pcap_if_t **devices, char *error) {
    (void)error;
    *devices = pcap_test_mode == 1 ? NULL : &first;
    return pcap_test_mode == 2 ? -1 : 0;
}
void pcap_freealldevs(pcap_if_t *devices) { (void)devices; pcap_test_freed++; }
