/**
 * gain_example.c
 *
 * Intercepts the default audio input device, applies a gain multiplier,
 * and sends the result to the default audio output device.
 *
 * Compile:
 *   clang -o gain_example gain_example.c \
 *       -framework CoreAudio \
 *       -framework AudioToolbox \
 *       -framework CoreFoundation
 *
 * Run:
 *   ./gain_example
 */

#include <CoreAudio/CoreAudio.h>
#include <AudioToolbox/AudioToolbox.h>
#include <CoreFoundation/CoreFoundation.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

// ---------------------------------------------------------------------------
// Configuration
// ---------------------------------------------------------------------------

#define GAIN 0.5f   // 0.0 = silence, 1.0 = unity, >1.0 = amplify

// ---------------------------------------------------------------------------
// Shared state between the two I/O callbacks
// ---------------------------------------------------------------------------

// A dead-simple single-producer / single-consumer ring buffer.
// The input callback writes; the output callback reads.
#define RING_BUFFER_FRAMES 8192

typedef struct {
    float   *data;          // interleaved float samples
    uint32_t capacity;      // total floats the buffer can hold (frames * channels)
    uint32_t channels;
    volatile uint32_t writePos;
    volatile uint32_t readPos;
} RingBuffer;

static RingBuffer gRing = {0};

static void ring_init(RingBuffer *rb, uint32_t frames, uint32_t channels) {
    rb->channels  = channels;
    rb->capacity  = frames * channels;
    rb->data      = (float *)calloc(rb->capacity, sizeof(float));
    rb->writePos  = 0;
    rb->readPos   = 0;
}

// Returns number of samples (not frames) available to read
static uint32_t ring_readable(const RingBuffer *rb) {
    return (rb->writePos - rb->readPos + rb->capacity) % rb->capacity;
}

static void ring_write(RingBuffer *rb, const float *src, uint32_t nSamples) {
    for (uint32_t i = 0; i < nSamples; i++) {
        rb->data[rb->writePos % rb->capacity] = src[i];
        rb->writePos = (rb->writePos + 1) % rb->capacity;
    }
}

static void ring_read(RingBuffer *rb, float *dst, uint32_t nSamples) {
    for (uint32_t i = 0; i < nSamples; i++) {
        dst[i] = rb->data[rb->readPos % rb->capacity];
        rb->readPos = (rb->readPos + 1) % rb->capacity;
    }
}

// ---------------------------------------------------------------------------
// AudioUnit helpers
// ---------------------------------------------------------------------------

static AudioUnit create_audio_unit(OSType subType, AudioDeviceID device) {
    AudioComponentDescription desc = {
        .componentType         = kAudioUnitType_Output,
        .componentSubType      = subType,
        .componentManufacturer = kAudioUnitManufacturer_Apple,
        .componentFlags        = 0,
        .componentFlagsMask    = 0,
    };

    AudioComponent comp = AudioComponentFindNext(NULL, &desc);
    if (!comp) { fprintf(stderr, "AudioComponentFindNext failed\n"); return NULL; }

    AudioUnit unit;
    OSStatus err = AudioComponentInstanceNew(comp, &unit);
    if (err) { fprintf(stderr, "AudioComponentInstanceNew: %d\n", (int)err); return NULL; }

    // Bind to a specific device (pass kAudioObjectUnknown to skip)
    if (device != kAudioObjectUnknown) {
        err = AudioUnitSetProperty(unit,
                                   kAudioOutputUnitProperty_CurrentDevice,
                                   kAudioUnitScope_Global,
                                   0,
                                   &device,
                                   sizeof(device));
        if (err) { fprintf(stderr, "SetProperty CurrentDevice: %d\n", (int)err); }
    }
    return unit;
}

// ---------------------------------------------------------------------------
// Input callback — called by the HAL when new mic/device frames arrive
// ---------------------------------------------------------------------------

static OSStatus input_callback(void                       *inRefCon,
                               AudioUnitRenderActionFlags *ioActionFlags,
                               const AudioTimeStamp        *inTimeStamp,
                               UInt32                      inBusNumber,
                               UInt32                      inNumberFrames,
                               AudioBufferList            *ioData)
{
    AudioUnit inputUnit = *(AudioUnit *)inRefCon;

    // We must call AudioUnitRender to actually pull the captured samples
    // into a buffer we own. Build a local AudioBufferList for that.
    uint32_t channels = 2;
    float scratch[inNumberFrames * channels];
    memset(scratch, 0, sizeof(scratch));

    AudioBufferList abl;
    abl.mNumberBuffers              = 1;
    abl.mBuffers[0].mNumberChannels = channels;
    abl.mBuffers[0].mDataByteSize   = (uint32_t)(inNumberFrames * channels * sizeof(float));
    abl.mBuffers[0].mData           = scratch;

    OSStatus err = AudioUnitRender(inputUnit,
                                   ioActionFlags,
                                   inTimeStamp,
                                   inBusNumber,
                                   inNumberFrames,
                                   &abl);
    if (err) { return err; }

    // ----- Apply gain -----
    float *samples = (float *)abl.mBuffers[0].mData;
    uint32_t nSamples = inNumberFrames * channels;
    for (uint32_t i = 0; i < nSamples; i++) {
        samples[i] *= GAIN;
    }

    // Push processed audio into the ring buffer for the output callback
    ring_write(&gRing, samples, nSamples);

    return noErr;
}

// ---------------------------------------------------------------------------
// Output callback — called by the HAL when the speaker needs more frames
// ---------------------------------------------------------------------------

static OSStatus output_callback(void                       *inRefCon,
                                AudioUnitRenderActionFlags *ioActionFlags,
                                const AudioTimeStamp        *inTimeStamp,
                                UInt32                      inBusNumber,
                                UInt32                      inNumberFrames,
                                AudioBufferList            *ioData)
{
    (void)inRefCon; (void)inTimeStamp; (void)inBusNumber;

    for (uint32_t buf = 0; buf < ioData->mNumberBuffers; buf++) {
        float   *dst      = (float *)ioData->mBuffers[buf].mData;
        uint32_t channels = ioData->mBuffers[buf].mNumberChannels;
        uint32_t nSamples = inNumberFrames * channels;

        uint32_t available = ring_readable(&gRing);
        if (available >= nSamples) {
            ring_read(&gRing, dst, nSamples);
        } else {
            // Underrun — output silence rather than stale data
            memset(dst, 0, nSamples * sizeof(float));
        }
    }
    return noErr;
}

// ---------------------------------------------------------------------------
// Device helpers
// ---------------------------------------------------------------------------

static AudioDeviceID get_default_device(Boolean isInput) {
    AudioDeviceID deviceID = kAudioObjectUnknown;
    UInt32 size = sizeof(deviceID);

    AudioObjectPropertyAddress addr = {
        .mSelector = isInput ? kAudioHardwarePropertyDefaultInputDevice
                             : kAudioHardwarePropertyDefaultOutputDevice,
        .mScope    = kAudioObjectPropertyScopeGlobal,
        .mElement  = kAudioObjectPropertyElementMain,
    };

    AudioObjectGetPropertyData(kAudioObjectSystemObject, &addr,
                               0, NULL, &size, &deviceID);
    return deviceID;
}

// ---------------------------------------------------------------------------
// Graceful shutdown on Ctrl-C
// ---------------------------------------------------------------------------

static volatile int gRunning = 1;
static void on_sigint(int sig) { (void)sig; gRunning = 0; }

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main(void) {
    signal(SIGINT, on_sigint);

    // ---- Set up the stream format (non-interleaved float is Core Audio's native) ----
    AudioStreamBasicDescription fmt = {
        .mSampleRate       = 44100.0,
        .mFormatID         = kAudioFormatLinearPCM,
        .mFormatFlags      = kAudioFormatFlagsNativeFloatPacked,
        .mBytesPerPacket   = 8,   // 2 channels * 4 bytes
        .mFramesPerPacket  = 1,
        .mBytesPerFrame    = 8,
        .mChannelsPerFrame = 2,
        .mBitsPerChannel   = 32,
    };

    ring_init(&gRing, RING_BUFFER_FRAMES, fmt.mChannelsPerFrame);

    // ---- Input AudioUnit (AUHAL bound to the default input device) ----
    AudioDeviceID inputDevice = get_default_device(true);
    AudioUnit inputUnit = create_audio_unit(kAudioUnitSubType_HALOutput, inputDevice);

    // Enable input bus, disable output bus on the input unit
    UInt32 one = 1, zero = 0;
    AudioUnitSetProperty(inputUnit, kAudioOutputUnitProperty_EnableIO,
                         kAudioUnitScope_Input,  1, &one,  sizeof(one));
    AudioUnitSetProperty(inputUnit, kAudioOutputUnitProperty_EnableIO,
                         kAudioUnitScope_Output, 0, &zero, sizeof(zero));

    AudioUnitSetProperty(inputUnit, kAudioUnitProperty_StreamFormat,
                         kAudioUnitScope_Output, 1, &fmt, sizeof(fmt));

    // Register the input callback (inRefCon carries inputUnit for AudioUnitRender)
    static AudioUnit inputUnitRef;
    inputUnitRef = inputUnit;
    AURenderCallbackStruct inputCB = { input_callback, &inputUnitRef };
    AudioUnitSetProperty(inputUnit,
                         kAudioOutputUnitProperty_SetInputCallback,
                         kAudioUnitScope_Global, 0,
                         &inputCB, sizeof(inputCB));

    // ---- Output AudioUnit (AUHAL bound to the default output device) ----
    AudioDeviceID outputDevice = get_default_device(false);
    AudioUnit outputUnit = create_audio_unit(kAudioUnitSubType_HALOutput, outputDevice);

    AudioUnitSetProperty(outputUnit, kAudioOutputUnitProperty_EnableIO,
                         kAudioUnitScope_Output, 0, &one,  sizeof(one));
    AudioUnitSetProperty(outputUnit, kAudioOutputUnitProperty_EnableIO,
                         kAudioUnitScope_Input,  1, &zero, sizeof(zero));

    AudioUnitSetProperty(outputUnit, kAudioUnitProperty_StreamFormat,
                         kAudioUnitScope_Input, 0, &fmt, sizeof(fmt));

    AURenderCallbackStruct outputCB = { output_callback, NULL };
    AUGraphSetNodeInputCallback; // not used; we use the render callback directly
    AudioUnitSetProperty(outputUnit,
                         kAudioUnitProperty_SetRenderCallback,
                         kAudioUnitScope_Input, 0,
                         &outputCB, sizeof(outputCB));

    // ---- Initialize & start ----
    AudioUnitInitialize(inputUnit);
    AudioUnitInitialize(outputUnit);
    AudioOutputUnitStart(inputUnit);
    AudioOutputUnitStart(outputUnit);

    printf("Running — gain = %.2f. Press Ctrl-C to stop.\n", GAIN);
    while (gRunning) {
        CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.1, false);
    }

    // ---- Teardown ----
    AudioOutputUnitStop(inputUnit);
    AudioOutputUnitStop(outputUnit);
    AudioUnitUninitialize(inputUnit);
    AudioUnitUninitialize(outputUnit);
    AudioComponentInstanceDispose(inputUnit);
    AudioComponentInstanceDispose(outputUnit);
    free(gRing.data);

    printf("\nStopped.\n");
    return 0;
}

