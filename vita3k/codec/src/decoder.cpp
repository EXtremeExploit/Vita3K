#include <codec/state.h>

extern "C" {
#include <libavcodec/avcodec.h>
}

#include <cassert>

uint32_t DecoderState::get(DecoderQuery query) {
    switch (query) {
    case DecoderQuery::WIDTH:
        return this->context->width;
    case DecoderQuery::HEIGHT:
        return this->context->height;
    case DecoderQuery::CHANNELS:
        return this->context->channels;
    case DecoderQuery::BIT_RATE:
        return this->context->bit_rate;
    case DecoderQuery::SAMPLE_RATE:
        return this->context->sample_rate;
    case DecoderQuery::AT9_BLOCK_ALIGN:
        return 0;
    case DecoderQuery::AT9_SAMPLE_PER_SUPERFRAME:
        return 0;
    case DecoderQuery::AT9_FRAMES_IN_SUPERFRAME:
        return 0;
    case DecoderQuery::AT9_SUPERFRAME_SIZE:
        return 0;
    default:
        return 0;
    }
}

void DecoderState::configure(void *options) {
    // do nothing
}

void DecoderState::flush() {
    avcodec_flush_buffers(context);
}

DecoderState::~DecoderState() {
    avcodec_close(context);
    avcodec_free_context(&context);
}
