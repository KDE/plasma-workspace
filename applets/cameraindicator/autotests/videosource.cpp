/*
    SPDX-FileCopyrightText: 2018 Wim Taymans <wtaymans@redhat.com>
    SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>
    SPDX-License-Identifier: MIT
*/

#include <cerrno>
#include <csignal>
#include <cstdint>

#include <iostream>
#include <spa/debug/pod.h>
#include <spa/param/video/format-utils.h>

#include <pipewire/pipewire.h>

#define BPP 3
#define CURSOR_WIDTH 64
#define CURSOR_HEIGHT 64
#define CURSOR_BPP 4

#define MAX_BUFFERS 64

struct Data {
    pw_main_loop *loop = nullptr;
    spa_source *timer = nullptr;

    pw_context *context = nullptr;
    pw_core *core = nullptr;

    pw_stream *stream = nullptr;
    spa_hook stream_listener;

    spa_video_info_raw format;
    int32_t stride;

    int res;
};

static void on_process(void *)
{
    // do nothing
}

static void on_timeout(void *userdata, uint64_t /*expirations*/)
{
    Data *data = static_cast<struct Data *>(userdata);
    pw_log_trace("timeout");
    pw_stream_trigger_process(data->stream);
}

static void on_stream_state_changed(void *_data, enum pw_stream_state /*old*/, enum pw_stream_state state, const char * /*error*/)
{
    Data *data = static_cast<struct Data *>(_data);

    std::cout << "stream state: " << pw_stream_state_as_string(state) << std::endl;

    switch (state) {
    case PW_STREAM_STATE_ERROR:
    case PW_STREAM_STATE_UNCONNECTED:
        pw_main_loop_quit(data->loop);
        break;

    case PW_STREAM_STATE_PAUSED:
        std::cout << "node id: " << pw_stream_get_node_id(data->stream) << std::endl;
        pw_loop_update_timer(pw_main_loop_get_loop(data->loop), data->timer, nullptr, nullptr, false);
        break;
    case PW_STREAM_STATE_STREAMING: {
        timespec timeout, interval;

        timeout.tv_sec = 0;
        timeout.tv_nsec = 1;
        interval.tv_sec = 0;
        interval.tv_nsec = 40 * SPA_NSEC_PER_MSEC;

        pw_loop_update_timer(pw_main_loop_get_loop(data->loop), data->timer, &timeout, &interval, false);
        break;
    }
    default:
        break;
    }
}

static void on_trigger_done(void *)
{
    pw_log_trace("trigger done");
}

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
    .trigger_done = on_trigger_done,
};

static void do_quit(void *userdata, int /*signal_number*/)
{
    auto data = static_cast<struct Data *>(userdata);
    pw_main_loop_quit(data->loop);
}

int main(int argc, char *argv[])
{
    Data data;
    const spa_pod *params[1];
    uint8_t buffer[1024];
    spa_pod_builder b = SPA_POD_BUILDER_INIT(buffer, sizeof(buffer));

    pw_init(&argc, &argv);

    data.loop = pw_main_loop_new(nullptr);

    pw_loop_add_signal(pw_main_loop_get_loop(data.loop), SIGINT, do_quit, &data);
    pw_loop_add_signal(pw_main_loop_get_loop(data.loop), SIGTERM, do_quit, &data);

    data.context = pw_context_new(pw_main_loop_get_loop(data.loop), nullptr, 0);

    data.timer = pw_loop_add_timer(pw_main_loop_get_loop(data.loop), on_timeout, &data);

    auto cleanup = [&data] {
        pw_context_destroy(data.context);
        pw_main_loop_destroy(data.loop);
        pw_deinit();
    };

    data.core = pw_context_connect(data.context, nullptr, 0);
    if (data.core == nullptr) {
        std::cerr << "can't connect" << std::endl;
        data.res = -errno;
        cleanup();
        return data.res;
    }

    data.stream = pw_stream_new(data.core,
                                "mock-camera",
                                pw_properties_new(PW_KEY_MEDIA_CLASS, //
                                                  "Video/Source",
                                                  PW_KEY_MEDIA_CATEGORY,
                                                  "Capture",
                                                  PW_KEY_MEDIA_ROLE,
                                                  "Camera",
                                                  PW_KEY_NODE_NICK,
                                                  "KDE Camera",
                                                  nullptr));

    spa_rectangle rectangle1 = SPA_RECTANGLE(320, 240);
    spa_rectangle rectangle2 = SPA_RECTANGLE(1, 1);
    spa_rectangle rectangle3 = SPA_RECTANGLE(4096, 4096);
    spa_fraction fraction = SPA_FRACTION(25, 1);
    params[0] = static_cast<struct spa_pod *>(spa_pod_builder_add_object(&b,
                                                                         SPA_TYPE_OBJECT_Format,
                                                                         SPA_PARAM_EnumFormat,
                                                                         SPA_FORMAT_mediaType,
                                                                         SPA_POD_Id(SPA_MEDIA_TYPE_video),
                                                                         SPA_FORMAT_mediaSubtype,
                                                                         SPA_POD_Id(SPA_MEDIA_SUBTYPE_raw),
                                                                         SPA_FORMAT_VIDEO_format,
                                                                         SPA_POD_Id(SPA_VIDEO_FORMAT_RGB),
                                                                         SPA_FORMAT_VIDEO_size,
                                                                         SPA_POD_CHOICE_RANGE_Rectangle(&rectangle1, &rectangle2, &rectangle3),
                                                                         SPA_FORMAT_VIDEO_framerate,
                                                                         SPA_POD_Fraction(&fraction)));

    pw_stream_add_listener(data.stream, &data.stream_listener, &stream_events, &data);

    pw_stream_connect(data.stream, PW_DIRECTION_OUTPUT, PW_ID_ANY, static_cast<pw_stream_flags>(PW_STREAM_FLAG_DRIVER | PW_STREAM_FLAG_MAP_BUFFERS), params, 1);

    pw_main_loop_run(data.loop);

    cleanup();
    return data.res;
}
