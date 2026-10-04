/* Exercise guest audio commands through snd.c; host audio and DMA are stubbed. */
#include "main.h"
#include "configuration.h"
#include "snd.h"
#include "audio.h"
#include "cycInt.h"
#include "grab.h"
#include "kms.h"
#include "leds.h"
#include "log.h"
#include <assert.h>

CNF_PARAMS ConfigureParams;
static uint8_t queued[16], recorded[16];
static int queued_len, recorded_len, pings;
static uint64_t next_delay;

bool Log_DebugEnabled(void) { return false; }
void Log_PrintfInt(LOGTYPE level, const char *fmt, ...) { (void)level; (void)fmt; }
void leds_ping(LedId id) { assert(id == LED_SND); pings++; }
void Audio_Output_Enable(bool enable) { (void)enable; }
void Audio_Output_Queue_Clear(void) { queued_len = 0; }
void Audio_Output_Queue_Flush(void) {}
int Audio_Output_Queue_Size(void) { return 0; }
void Audio_Output_Queue_Put(uint8_t *data, int len) {
    assert(len <= (int)sizeof(queued));
    memcpy(queued, data, len);
    queued_len = len;
}
void Grab_Sound(uint8_t *data, int len) {
    assert(len <= (int)sizeof(recorded));
    memcpy(recorded, data, len);
    recorded_len = len;
}
void CycInt_AddTimeEvent(uint64_t real, uint64_t fast, event_id id) {
    (void)fast;
    assert(id == EVENT_SND_OUTPUT);
    next_delay = real;
}
void CycInt_UpdateTimeEvent(uint64_t real, uint64_t fast, event_id id) {
    CycInt_AddTimeEvent(real, fast, id);
}
void kms_send_sndout_request(void) {
    static const uint8_t sample[] = {0x12, 0x34, 0xab, 0xcd};
    memcpy(snd_buffer, sample, sizeof(sample));
    snd_buffer_len = sizeof(sample);
}
void kms_send_sndout_underrun(void) { assert(!"unexpected underrun"); }

static void expect_sample(uint8_t mode, const uint8_t *expected, int len) {
    snd_start_output(mode);
    snd_send_sample(0x1234abcd);
    assert(queued_len == len && recorded_len == len);
    assert(memcmp(queued, expected, len) == 0);
    assert(memcmp(recorded, expected, len) == 0);
}

int main(void) {
    static const uint8_t normal[] = {0x12, 0x34, 0xab, 0xcd};
    static const uint8_t repeat[] = {0x12, 0x34, 0xab, 0xcd, 0x12, 0x34, 0xab, 0xcd};
    static const uint8_t zero[] = {0x12, 0x34, 0xab, 0xcd, 0, 0, 0, 0};
    static const uint8_t silence[] = {0, 0, 0, 0};
    Sound_Reset();
    expect_sample(0x00, normal, sizeof(normal));
    expect_sample(0x10, repeat, sizeof(repeat));
    expect_sample(0x30, zero, sizeof(zero));
    assert(pings == 3);

    /* Guest mute still consumes frames and records silence. */
    snd_gpo_access(0x10);
    expect_sample(0x00, silence, sizeof(silence));
    snd_gpo_access(0);

    /* Disabling host sound must preserve guest playback timing. */
    ConfigureParams.Sound.bEnableSound = false;
    SND_Out_Handler();
    assert(next_delay == 23);
    assert(queued_len == 4 && memcmp(queued, normal, 4) == 0);
    snd_start_output(0x10);
    SND_Out_Handler();
    assert(next_delay == 46);
    assert(queued_len == 8 && memcmp(queued, repeat, 8) == 0);

    snd_stop_output();
    int before = pings;
    snd_send_sample(0xfeedbeef);
    assert(pings == before);
    puts("test-sound: OK");
    return 0;
}
