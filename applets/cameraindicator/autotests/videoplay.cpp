/*
    SPDX-FileCopyrightText: 2018 Wim Taymans <wtaymans@redhat.com>
    SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>
    SPDX-License-Identifier: MIT
*/

#include <csignal>
#include <iostream>

#include <spa/debug/pod.h>
#include <spa/param/latency-utils.h>
#include <spa/param/props.h>
#include <spa/utils/result.h>

#include <pipewire/pipewire.h>

struct Data {
    pw_main_loop *loop = nullptr;
    pw_stream *stream = nullptr;
};

static void on_process(void *)
{
    // dp nothing
}

static void on_stream_state_changed(void *_data, enum pw_stream_state /*old*/, enum pw_stream_state state, const char * /*error*/)
{
    Data *data = static_cast<struct Data *>(_data);
    std::cout << "player stream state: " << pw_stream_state_as_string(state) << std::endl;
    switch (state) {
    case PW_STREAM_STATE_UNCONNECTED:
        pw_main_loop_quit(data->loop);
        break;
    case PW_STREAM_STATE_PAUSED:
        /* because we started inactive, activate ourselves now */
        std::cout << "pw_stream_set_active " << pw_stream_set_active(data->stream, true) << std::endl;
        break;
    default:
        break;
    }
}

/* these are the stream events we listen for */
static const pw_stream_events stream_events = {
    .version = PW_VERSION_STREAM_EVENTS,
    .destroy = nullptr,
    .state_changed = on_stream_state_changed,
    .control_info = nullptr,
    .io_changed = nullptr,
    .param_changed = nullptr,
    .add_buffer = nullptr,
    .remove_buffer = nullptr,
    .process = on_process,
    .drained = nullptr,
    .command = nullptr,
    .trigger_done = nullptr,
};

static void do_quit(void *userdata, int /*signal_number*/)
{
    Data *data = static_cast<struct Data *>(userdata);
    pw_main_loop_quit(data->loop);
}

int main(int argc, char *argv[])
{
    Data data;
    const spa_pod *params[1];
    pw_properties *props;
    int res;

    pw_init(&argc, &argv);

    /* create a main loop */
    data.loop = pw_main_loop_new(nullptr);

    pw_loop_add_signal(pw_main_loop_get_loop(data.loop), SIGINT, do_quit, &data);
    pw_loop_add_signal(pw_main_loop_get_loop(data.loop), SIGTERM, do_quit, &data);

    /* create a simple stream, the simple stream manages to core and remote
     * objects for you if you don't need to deal with them
     *
     * If you plan to autoconnect your stream, you need to provide at least
     * media, category and role properties
     *
     * Pass your events and a user_data pointer as the last arguments. This
     * will inform you about the stream state. The most important event
     * you need to listen to is the process event where you need to consume
     * the data provided to you.
     */
    props = pw_properties_new(PW_KEY_MEDIA_TYPE, //
                              "Video",
                              PW_KEY_MEDIA_CATEGORY,
                              "Playback",
                              PW_KEY_MEDIA_ROLE,
                              "Test",
                              nullptr),
    data.stream = pw_stream_new_simple(pw_main_loop_get_loop(data.loop), "mock-camera", props, &stream_events, &data);
    /* now connect to the stream, we need a direction (input/output),
     * an optional target node to connect to, some flags and parameters
     */
    if ((res = pw_stream_connect(data.stream,
                                 PW_DIRECTION_INPUT,
                                 std::strtol(argv[1], nullptr, 10),
                                 static_cast<pw_stream_flags>(PW_STREAM_FLAG_AUTOCONNECT | /* try to automatically connect this stream */
                                                              PW_STREAM_FLAG_INACTIVE | /* we will activate ourselves */
                                                              PW_STREAM_FLAG_MAP_BUFFERS), /* mmap the buffer data for us */
                                 params,
                                 0)) /* extra parameters, see above */
        < 0) {
        std::cerr << "can't connect: " << spa_strerror(res) << std::endl;
        return -1;
    }

    /* do things until we quit the mainloop */
    pw_main_loop_run(data.loop);

    pw_stream_destroy(data.stream);
    pw_main_loop_destroy(data.loop);
    pw_deinit();

    return 0;
}
