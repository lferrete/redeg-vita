#include "objc/objc_classes.h"
#include "macho/macho.h"
#include "utils/logger.h"

#include <psp2/audioout.h>
#include <psp2/kernel/threadmgr.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

extern uintptr_t g_got_base;

/* ------------------------------------------------------------------ */
/* AudioToolbox: Real PS Vita sceAudioOut backend for AudioQueue       */
/* ------------------------------------------------------------------ */

typedef struct audio_file {
    int used;
    int fd;
} audio_file_t;

static int s_audio_logged = 0;

int AudioFileClose(void *id) { (void)id; return 0; }

int AudioFileOpenURL(void *url, signed char permissions, uint32_t hint, void **out) {
    (void)url; (void)permissions; (void)hint;
    if (out) *out = NULL;
    if (s_audio_logged++ < 8) l_warn("[audio] AudioFileOpenURL -> not implemented");
    return -1;
}

int AudioFileOpenWithCallbacks(void *client, void *readProc, void *getSizeProc,
                               void *setSizeProc, void *writeProc, uint32_t hint, void **out) {
    (void)client; (void)readProc; (void)getSizeProc; (void)setSizeProc; (void)writeProc; (void)hint;
    if (out) *out = NULL;
    if (s_audio_logged++ < 8) l_warn("[audio] AudioFileOpenWithCallbacks -> not implemented");
    return -1;
}

int AudioFileGetProperty(void *id, uint32_t prop, uint32_t *size, void *out) {
    (void)id; (void)prop; (void)size; (void)out;
    return -1;
}
int AudioFileGetPropertyInfo(void *id, uint32_t prop, uint32_t *size, uint32_t *writable) {
    (void)id; (void)prop;
    if (size) *size = 0;
    if (writable) *writable = 0;
    return -1;
}
int AudioFileReadPackets(void *id, unsigned char useCache, uint32_t *numBytes,
                         void *packetDescs, uint32_t numPackets, void *outData) {
    (void)id; (void)useCache; (void)packetDescs; (void)numPackets; (void)outData;
    if (numBytes) *numBytes = 0;
    return -1;
}

/* iOS AudioQueueBuffer layout expected by Marmalade's HandleOutputBuffer (0x000351e4):
 *   +0x00: uint32_t mAudioDataBytesCapacity
 *   +0x04: void    *mAudioData
 *   +0x08: uint32_t mAudioDataByteSize
 *   +0x0c: void    *mUserData
 *   +0x10: uint32_t mPacketDescriptionCapacity
 *   +0x14: void    *mPacketDescriptions
 *   +0x18: uint32_t mPacketDescriptionCount
 */
typedef struct redeg_aq_buffer {
    uint32_t mAudioDataBytesCapacity;
    void    *mAudioData;
    uint32_t mAudioDataByteSize;
    void    *mUserData;
    uint32_t mPacketDescriptionCapacity;
    void    *mPacketDescriptions;
    uint32_t mPacketDescriptionCount;
    volatile int queued;
} redeg_aq_buffer_t;

#define AQ_MAX_BUFFERS 8

typedef void (*aq_output_cb_t)(void *userData, void *q, void *buf);

typedef struct redeg_aq {
    double           sample_rate;
    uint32_t         channels;
    uint32_t         bits_per_channel;
    aq_output_cb_t   callback;
    void            *user_data;
    redeg_aq_buffer_t *buffers[AQ_MAX_BUFFERS];
    int              num_buffers;
    volatile int     read_idx;
    volatile int     write_idx;
    volatile int     running;
    volatile int     disposed;
    int              port;
    SceUID           thread;
    float            volume;
} redeg_aq_t;

/* Call guest AudioQueueOutputCallback with r9 = g_got_base */
__attribute__((naked)) static void call_aq_callback(aq_output_cb_t cb, void *userData, void *q, void *buf, uint32_t got_base) {
    __asm__ volatile(
        "push {r4, r5, r9, lr}\n"
        "ldr r9, [sp, #16]\n"
        "mov r4, r0\n"
        "mov r0, r1\n"
        "mov r1, r2\n"
        "mov r2, r3\n"
        "blx r4\n"
        "pop {r4, r5, r9, pc}\n"
    );
}

static int snap_vita_voice_freq(int freq) {
    static const int valid_freqs[] = {
        8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000
    };
    int best = 22050;
    int best_diff = 1000000;
    for (unsigned i = 0; i < sizeof(valid_freqs) / sizeof(valid_freqs[0]); i++) {
        int d = freq - valid_freqs[i];
        if (d < 0) d = -d;
        if (d < best_diff) {
            best_diff = d;
            best = valid_freqs[i];
        }
    }
    return best;
}

static int redeg_audio_thread(SceSize args, void *argp) {
    (void)args;
    redeg_aq_t *aq = *(redeg_aq_t **)argp;
    if (!aq) return 0;

    int raw_freq = (int)(aq->sample_rate + 0.5);
    int freq = snap_vita_voice_freq(raw_freq);
    int mode = (aq->channels >= 2) ? SCE_AUDIO_OUT_MODE_STEREO : SCE_AUDIO_OUT_MODE_MONO;
    /* 512 samples per buffer matches Marmalade's mBufferByteSize = mChannelsPerFrame << 10 */
    int port = sceAudioOutOpenPort(SCE_AUDIO_OUT_PORT_TYPE_VOICE, 512, freq, mode);
    if (port < 0) {
        l_warn("[audio] sceAudioOutOpenPort(freq=%d, mode=%d) failed (0x%08x), retrying 22050Hz", freq, mode, port);
        freq = 22050;
        port = sceAudioOutOpenPort(SCE_AUDIO_OUT_PORT_TYPE_VOICE, 512, freq, mode);
    }
    aq->port = port;
    if (port >= 0) {
        int vol = (int)(aq->volume * (float)SCE_AUDIO_VOLUME_0DB);
        if (vol > SCE_AUDIO_VOLUME_0DB) vol = SCE_AUDIO_VOLUME_0DB;
        if (vol < 0) vol = 0;
        int vols[2] = { vol, vol };
        sceAudioOutSetVolume(port, SCE_AUDIO_VOLUME_FLAG_L_CH | SCE_AUDIO_VOLUME_FLAG_R_CH, vols);
        l_info("[audio] opened sceAudioOut port=%d freq=%d (req=%d) channels=%u vol=%d",
               port, freq, raw_freq, aq->channels, vol);
    } else {
        l_error("[audio] failed to open sceAudioOut port: 0x%08x", port);
    }

    int16_t silence[512 * 2];
    memset(silence, 0, sizeof(silence));
    int16_t pcm_out[512 * 2];
    uint32_t frame_bytes = 512u * (aq->channels >= 2 ? 2u : 1u) * 2u;
    int non_zero_logged = 0;

    while (!aq->disposed) {
        if (!aq->running || aq->num_buffers == 0) {
            sceKernelDelayThread(5000);
            continue;
        }

        int idx = aq->read_idx % aq->num_buffers;
        redeg_aq_buffer_t *b = aq->buffers[idx];
        if (!b || !b->queued) {
            sceKernelDelayThread(1000);
            continue;
        }

        aq->read_idx = (idx + 1) % aq->num_buffers;

        if (aq->port >= 0 && b->mAudioData && b->mAudioDataByteSize > 0) {
            if (!non_zero_logged) {
                const int16_t *s = (const int16_t *)b->mAudioData;
                uint32_t n = b->mAudioDataByteSize / 2;
                for (uint32_t i = 0; i < n; i++) {
                    if (s[i] != 0) {
                        non_zero_logged = 1;
                        l_info("[audio] active non-zero PCM audio stream detected! sample[%u]=%d", i, (int)s[i]);
                        break;
                    }
                }
            }
            if (b->mAudioDataByteSize == frame_bytes) {
                sceAudioOutOutput(aq->port, b->mAudioData);
            } else {
                memset(pcm_out, 0, frame_bytes);
                uint32_t copy_bytes = b->mAudioDataByteSize < frame_bytes ? b->mAudioDataByteSize : frame_bytes;
                memcpy(pcm_out, b->mAudioData, copy_bytes);
                sceAudioOutOutput(aq->port, pcm_out);
            }
        } else if (aq->port >= 0) {
            sceAudioOutOutput(aq->port, silence);
        } else {
            sceKernelDelayThread(20000);
        }

        b->queued = 0;
        if (aq->callback && !aq->disposed && aq->running) {
            call_aq_callback(aq->callback, aq->user_data, aq, b, g_got_base);
        }
    }

    if (aq->port >= 0) {
        sceAudioOutReleasePort(aq->port);
        aq->port = -1;
    }
    return 0;
}

int AudioQueueNewOutput(const void *format, void *callback, void *userData,
                        void *runLoop, void *runLoopMode, uint32_t flags, void **outQueue) {
    (void)runLoop; (void)runLoopMode; (void)flags;
    redeg_aq_t *aq = (redeg_aq_t *)calloc(1, sizeof(redeg_aq_t));
    if (!aq) return -1;

    aq->sample_rate = 22050.0;
    aq->channels = 1;
    aq->bits_per_channel = 16;
    if (format) {
        /* AudioStreamBasicDescription:
         * +0x00: double mSampleRate
         * +0x1c: uint32_t mChannelsPerFrame
         * +0x20: uint32_t mBitsPerChannel */
        double sr = 0.0;
        memcpy(&sr, format, sizeof(double));
        uint32_t ch = *(const uint32_t *)((const uint8_t *)format + 0x1c);
        uint32_t bits = *(const uint32_t *)((const uint8_t *)format + 0x20);
        if (sr >= 4000.0 && sr <= 96000.0) aq->sample_rate = sr;
        if (ch >= 1 && ch <= 2) aq->channels = ch;
        if (bits > 0) aq->bits_per_channel = bits;
    }
    aq->callback = (aq_output_cb_t)callback;
    aq->user_data = userData;
    aq->port = -1;
    aq->thread = -1;
    aq->volume = 1.0f;

    if (outQueue) *outQueue = aq;
    l_info("[audio] AudioQueueNewOutput: rate=%d ch=%u bits=%u cb=%p user=%p -> aq=%p",
           (int)(aq->sample_rate + 0.5), aq->channels, aq->bits_per_channel, callback, userData, (void *)aq);
    return 0;
}

int AudioQueueDispose(void *q, unsigned char immediate) {
    (void)immediate;
    if (!q) return 0;
    redeg_aq_t *aq = (redeg_aq_t *)q;
    aq->running = 0;
    aq->disposed = 1;
    if (aq->thread >= 0) {
        sceKernelWaitThreadEnd(aq->thread, NULL, NULL);
        sceKernelDeleteThread(aq->thread);
        aq->thread = -1;
    }
    for (int i = 0; i < aq->num_buffers; i++) {
        free(aq->buffers[i]);
        aq->buffers[i] = NULL;
    }
    free(aq);
    return 0;
}

int AudioQueueAllocateBuffer(void *q, uint32_t size, void **buf) {
    if (!buf) return -1;
    redeg_aq_buffer_t *b = (redeg_aq_buffer_t *)calloc(1, sizeof(redeg_aq_buffer_t) + size + 64);
    if (!b) {
        *buf = NULL;
        return -1;
    }
    b->mAudioDataBytesCapacity = size;
    b->mAudioData = (uint8_t *)b + sizeof(redeg_aq_buffer_t);
    b->mAudioDataByteSize = 0;
    if (q) {
        redeg_aq_t *aq = (redeg_aq_t *)q;
        if (aq->num_buffers < AQ_MAX_BUFFERS) {
            aq->buffers[aq->num_buffers++] = b;
        }
    }
    *buf = b;
    return 0;
}

int AudioQueueFreeBuffer(void *q, void *buf) {
    if (buf) {
        ((redeg_aq_buffer_t *)buf)->queued = 0;
    }
    if (!q) {
        free(buf);
        return 0;
    }
    /* Defer actual free(buf) to AudioQueueDispose after sceKernelWaitThreadEnd
     * so redeg_audio_thread can never read a freed buffer while in flight. */
    return 0;
}

int AudioQueueEnqueueBuffer(void *q, void *buf, uint32_t n, void *descs) {
    (void)n; (void)descs;
    if (!q || !buf) return -1;
    redeg_aq_t *aq = (redeg_aq_t *)q;
    redeg_aq_buffer_t *b = (redeg_aq_buffer_t *)buf;

    int found = -1;
    for (int i = 0; i < aq->num_buffers; i++) {
        if (aq->buffers[i] == b) {
            found = i;
            break;
        }
    }
    if (found < 0 && aq->num_buffers < AQ_MAX_BUFFERS) {
        found = aq->num_buffers++;
        aq->buffers[found] = b;
    }
    b->queued = 1;
    return 0;
}

int AudioQueueStart(void *q, const void *time) {
    (void)time;
    if (!q) return -1;
    redeg_aq_t *aq = (redeg_aq_t *)q;
    aq->disposed = 0;
    aq->running = 1;
    if (aq->thread < 0) {
        aq->thread = sceKernelCreateThread("redeg_audio", redeg_audio_thread, 0x10000100, 0x10000, 0, 0, NULL);
        if (aq->thread >= 0) {
            sceKernelStartThread(aq->thread, sizeof(aq), &aq);
            l_info("[audio] started redeg_audio thread 0x%08x for aq=%p", aq->thread, (void *)aq);
        } else {
            l_error("[audio] sceKernelCreateThread failed: 0x%08x", aq->thread);
        }
    }
    return 0;
}

int AudioQueueStop(void *q, unsigned char immediate) {
    (void)immediate;
    if (!q) return -1;
    redeg_aq_t *aq = (redeg_aq_t *)q;
    aq->running = 0;
    return 0;
}

int AudioQueuePause(void *q) {
    if (!q) return -1;
    redeg_aq_t *aq = (redeg_aq_t *)q;
    aq->running = 0;
    return 0;
}

int AudioQueueFlush(void *q) { (void)q; return 0; }
int AudioQueueReset(void *q) { (void)q; return 0; }

/* Guest iOS binary is compiled with ARMv6 softfp ABI, so the 3rd parameter
 * (Float32 inValue) is passed in integer register r2, NOT VFP s0! */
int AudioQueueSetParameter(void *q, uint32_t id, uint32_t value_bits) {
    if (!q) return -1;
    redeg_aq_t *aq = (redeg_aq_t *)q;
    float value = 1.0f;
    memcpy(&value, &value_bits, sizeof(float));
    if (id == 1) { /* kAudioQueueParam_Volume */
        if (value < 0.0f) value = 0.0f;
        if (value > 1.0f) value = 1.0f;
        aq->volume = value;
        if (aq->port >= 0) {
            int vol = (int)(value * (float)SCE_AUDIO_VOLUME_0DB);
            if (vol > SCE_AUDIO_VOLUME_0DB) vol = SCE_AUDIO_VOLUME_0DB;
            if (vol < 0) vol = 0;
            int vols[2] = { vol, vol };
            sceAudioOutSetVolume(aq->port, SCE_AUDIO_VOLUME_FLAG_L_CH | SCE_AUDIO_VOLUME_FLAG_R_CH, vols);
        }
    }
    return 0;
}

int AudioQueueGetParameter(void *q, uint32_t id, float *value) {
    if (!q) { if (value) *value = 1.0f; return 0; }
    redeg_aq_t *aq = (redeg_aq_t *)q;
    if (value) *value = (id == 1) ? aq->volume : 1.0f;
    return 0;
}
int AudioQueueGetProperty(void *q, uint32_t id, void *out, uint32_t *size) { (void)q; (void)id; (void)out; if (size) *size = 0; return 0; }
int AudioQueueSetProperty(void *q, uint32_t id, const void *in, uint32_t size) { (void)q; (void)id; (void)in; (void)size; return 0; }
int AudioQueueAddPropertyListener(void *q, uint32_t id, void *proc, void *user) { (void)q; (void)id; (void)proc; (void)user; return 0; }
int AudioQueueGetCurrentTime(void *q, void *ts, void *time, void *timeline) { (void)q; (void)ts; (void)time; (void)timeline; return 0; }

void AudioServicesPlaySystemSound(uint32_t id) { (void)id; }

int AudioSessionInitialize(void *runLoop, void *mode, void *interrupt, void *user) { (void)runLoop; (void)mode; (void)interrupt; (void)user; return 0; }
int AudioSessionSetActive(unsigned char active) { (void)active; return 0; }
int AudioSessionSetProperty(uint32_t id, uint32_t size, const void *data) { (void)id; (void)size; (void)data; return 0; }
int AudioSessionGetProperty(uint32_t id, uint32_t *size, void *data) { (void)id; if (size) *size = 0; (void)data; return 0; }
int AudioSessionAddPropertyListener(uint32_t id, void *proc, void *user) { (void)id; (void)proc; (void)user; return 0; }
