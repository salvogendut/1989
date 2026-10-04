/* Enumeration-only stand-in for the optional PCAP UI test. No packet I/O. */
#ifndef TEST_PCAP_H
#define TEST_PCAP_H
#define PCAP_ERRBUF_SIZE 256
typedef struct pcap_if {
    struct pcap_if *next;
    char *name;
    char *description;
} pcap_if_t;
int pcap_findalldevs(pcap_if_t **devices, char *error);
void pcap_freealldevs(pcap_if_t *devices);
extern int pcap_test_mode, pcap_test_freed;
#endif
