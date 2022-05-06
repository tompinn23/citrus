
#include <bits/time.h>
#include <wayland-server-core.h>
#include <wayland-util.h>
#include <wlr/backend.h>

#include <wlr/types/wlr_output.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/render/allocator.h>

#include <wlr/types/wlr_subcompositor.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_xdg_shell.h>

#include <xdg-shell-protocol.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
struct citrus_server {
    struct wl_display* wl_display;
    struct wl_event_loop* wl_event_loop;

    struct wlr_backend* backend;
    struct wlr_renderer* renderer;
    struct wlr_allocator* allocator;
    struct wlr_output_layout* output_layout;
    struct wlr_scene* scene;

    struct wlr_xdg_shell* xdg_shell;
    struct wl_listener new_xdg_surface;
    struct wl_list views;

    struct wl_listener new_output;
    struct wl_list outputs;
};

struct citrus_output {
    struct wl_list link;
    struct citrus_server* server;
    struct wlr_output* wlr_output;
    struct wl_listener frame;
    struct wl_listener destroy;
};

static void output_frame(struct wl_listener* listener, void* data) {
    struct citrus_output* output = wl_container_of(listener, output, frame);
    struct wlr_scene* scene = output->server->scene;

    struct wlr_scene_output* scene_output = wlr_scene_get_scene_output(scene, output->wlr_output);
    wlr_scene_output_commit(scene_output);

    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    wlr_scene_output_send_frame_done(scene_output, &now);
}

static void output_destroy(struct wl_listener* listener, void* data) {
    struct citrus_output* output = wl_container_of(listener, output, destroy);
    wl_list_remove(&output->frame.link);
    wl_list_remove(&output->destroy.link);
    wl_list_remove(&output->link);
    free(output);
}

/*
    Called when an output is added to wayland
    Sets the mode to the preferred mode if mode exists.

*/
static void new_output_notify(struct wl_listener* listener, void* data) {
    struct citrus_server* server = wl_container_of(listener, server, new_output);
    struct wlr_output* wlr_output = data;

    wlr_output_init_render(wlr_output, server->allocator, server->renderer);
    fprintf(stderr, "wlr output added: %s\n", wlr_output->description);
    if(!wl_list_empty(&wlr_output->modes)) {
        struct wlr_output_mode* mode = wlr_output_preferred_mode(wlr_output);
        fprintf(stderr, "wlr mode: %dx%d %dhz\n", mode->width, mode->height, mode->refresh);
        wlr_output_set_mode(wlr_output, mode);
        wlr_output_enable(wlr_output, true);
        if(!wlr_output_commit(wlr_output)) {
            return;
        }
    }

    struct citrus_output* output =
        calloc(1, sizeof(struct citrus_output));
    output->wlr_output = wlr_output;
    output->server = server;
    output->frame.notify = output_frame;
    wl_signal_add(&wlr_output->events.frame, &output->frame);

    output->destroy.notify = output_destroy;
    wl_signal_add(&wlr_output->events.destroy, &output->destroy);
    wl_list_insert(&server->outputs, &output->link);

    wlr_output_layout_add_auto(server->output_layout, wlr_output);
}

static void new_xdg_surface_notify(struct wl_listener* listener, void* data) {
    struct citrus_server*
}


int main(int argc, char** argv) {
    struct citrus_server server;
    server.wl_display = wl_display_create();
    server.wl_event_loop = wl_display_get_event_loop(server.wl_display);
    server.backend = wlr_backend_autocreate(server.wl_display);

    server.renderer = wlr_renderer_autocreate(server.backend);
    wlr_renderer_init_wl_display(server.renderer, server.wl_display);

    server.allocator = wlr_allocator_autocreate(server.backend, server.renderer);

    wlr_compositor_create(server.wl_display, server.renderer);
    wlr_subcompositor_create(server.wl_display);
    wlr_data_device_manager_create(server.wl_display);

    server.output_layout = wlr_output_layout_create();

    wl_list_init(&server.outputs);
    server.new_output.notify = new_output_notify;
    wl_signal_add(&server.backend->events.new_output, &server.new_output);

    server.scene = wlr_scene_create();
    wlr_scene_attach_output_layout(server.scene, server.output_layout);

    wl_list_init(&server.views);
    server.xdg_shell = wlr_xdg_shell_create(server.wl_display);
    server.new_xdg_surface.notify = new_xdg_surface_notify;
    wl_signal_add(&server.xdg_shell->events.new_surface, &server.new_xdg_surface);

    if(!wlr_backend_start(server.backend)) {
        fprintf(stderr, "Failed to start backend\n");
        wl_display_destroy(server.wl_display);
        return 1;
    }
    wl_display_run(server.wl_display);
    wl_display_destroy(server.wl_display);
    return 0;
}
