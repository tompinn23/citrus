#include "citrus.h"
#include "citrus_xdg.h"

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
    struct citrus_server* server = wl_container_of(listener, server, new_xdg_surface);
    struct wlr_xdg_surface* xdg_surface = data;

    if(xdg_surface->role == WLR_XDG_SURFACE_ROLE_POPUP) {
        struct wlr_xdg_surface* parent = wlr_xdg_surface_from_wlr_surface(xdg_surface->popup->parent);
        struct wlr_scene_node* parent_node = parent->data;
        xdg_surface->data = wlr_scene_xdg_surface_create(parent_node, xdg_surface);
        return;
    }
    struct citrus_view* view = calloc(1, sizeof(*view));
    view->server = server;
    view->xdg_toplevel = xdg_surface->toplevel;
    view->scene_node = wlr_scene_xdg_surface_create(&view->server->scene->node, view->xdg_toplevel->base);

	view->scene_node->data = view;
	xdg_surface->data = view->scene_node;

	/* Listen to the various events it can emit */
	view->map.notify = xdg_toplevel_map;
	wl_signal_add(&xdg_surface->events.map, &view->map);
	view->unmap.notify = xdg_toplevel_unmap;
	wl_signal_add(&xdg_surface->events.unmap, &view->unmap);
	view->destroy.notify = xdg_toplevel_destroy;
	wl_signal_add(&xdg_surface->events.destroy, &view->destroy);

	/* cotd */
	struct wlr_xdg_toplevel *toplevel = xdg_surface->toplevel;
	view->request_move.notify = xdg_toplevel_request_move;
	wl_signal_add(&toplevel->events.request_move, &view->request_move);
	view->request_resize.notify = xdg_toplevel_request_resize;
	wl_signal_add(&toplevel->events.request_resize, &view->request_resize);
	view->request_maximize.notify = xdg_toplevel_request_maximize;
	wl_signal_add(&toplevel->events.request_maximize,
		&view->request_maximize);
	view->request_fullscreen.notify = xdg_toplevel_request_fullscreen;
	wl_signal_add(&toplevel->events.request_fullscreen,
		&view->request_fullscreen);

}

void server_cursor_motion(struct wl_listener* listener, void* data) {

}
void server_cursor_motion_absolute(struct wl_listener* listener, void* data) {

}
void server_cursor_button(struct wl_listener* listener, void* data) {

}
void server_cursor_axis(struct wl_listener* listener, void* data) {

}
void server_cursor_frame(struct wl_listener* listener, void* data) {

}
void server_new_input(struct wl_listener* listener, void* data) {

}
void seat_request_cursor(struct wl_listener* listener, void* data) {

}
void seat_request_set_selection(struct wl_listener* listener, void* data) {
    
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

    server.cursor = wlr_cursor_create();
	wlr_cursor_attach_output_layout(server.cursor, server.output_layout);

	server.cursor_mgr = wlr_xcursor_manager_create(NULL, 24);
	wlr_xcursor_manager_load(server.cursor_mgr, 1);

	server.cursor_mode = CURSOR_PASSTHROUGH;
	server.cursor_motion.notify = server_cursor_motion;
	wl_signal_add(&server.cursor->events.motion, &server.cursor_motion);
	server.cursor_motion_absolute.notify = server_cursor_motion_absolute;
	wl_signal_add(&server.cursor->events.motion_absolute,
			&server.cursor_motion_absolute);
	server.cursor_button.notify = server_cursor_button;
	wl_signal_add(&server.cursor->events.button, &server.cursor_button);
	server.cursor_axis.notify = server_cursor_axis;
	wl_signal_add(&server.cursor->events.axis, &server.cursor_axis);
	server.cursor_frame.notify = server_cursor_frame;
	wl_signal_add(&server.cursor->events.frame, &server.cursor_frame);

 

    wl_list_init(&server.keyboards);
	server.new_input.notify = server_new_input;
	wl_signal_add(&server.backend->events.new_input, &server.new_input);
	server.seat = wlr_seat_create(server.wl_display, "seat0");
	server.request_cursor.notify = seat_request_cursor;
	wl_signal_add(&server.seat->events.request_set_cursor,
			&server.request_cursor);
	server.request_set_selection.notify = seat_request_set_selection;
	wl_signal_add(&server.seat->events.request_set_selection,
			&server.request_set_selection);

    const char *socket = wl_display_add_socket_auto(server.wl_display);
	if (!socket) {
		wlr_backend_destroy(server.backend);
		return 1;
	}


    if(!wlr_backend_start(server.backend)) {
        fprintf(stderr, "Failed to start backend\n");
        wl_display_destroy(server.wl_display);
        return 1;
    }
    wl_display_run(server.wl_display);
    wl_display_destroy(server.wl_display);
    return 0;
}
