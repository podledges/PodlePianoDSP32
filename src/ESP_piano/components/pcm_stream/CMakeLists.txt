# REQUIRES (not PRIV_) because pcm_framer.h includes audio_stream_v1.h.
idf_component_register(
    SRCS "ringbuf.c" "pcm_framer.c"
    INCLUDE_DIRS "include"
    REQUIRES asv1_contracts
)
