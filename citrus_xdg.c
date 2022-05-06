#include "citrus.h"
#include "citrus_xdg.h"

static void focus_view(struct citrus_view* view, struct wlr_surface* surface) {
    if(view == NULL)
        return;

    struct citrus_server* server = view->server;
}

static void begin_interactive() {
    
}


void xdg_toplevel_map(struct wl_listener *listener, void *data) {
	/* Called when the surface is mapped, or ready to display on-screen. */
	struct citrus_view *view = wl_container_of(listener, view, map);

	wl_list_insert(&view->server->views, &view->link);

	focus_view(view, view->xdg_toplevel->base->surface);
}

void xdg_toplevel_unmap(struct wl_listener *listener, void *data) {
	/* Called when the surface is unmapped, and should no longer be shown. */
	struct citrus_view *view = wl_container_of(listener, view, unmap);

	wl_list_remove(&view->link);
}

void xdg_toplevel_destroy(struct wl_listener *listener, void *data) {
	/* Called when the surface is destroyed and should never be shown again. */
	struct citrus_view *view = wl_container_of(listener, view, destroy);

	wl_list_remove(&view->map.link);
	wl_list_remove(&view->unmap.link);
	wl_list_remove(&view->destroy.link);
	wl_list_remove(&view->request_move.link);
	wl_list_remove(&view->request_resize.link);
	wl_list_remove(&view->request_maximize.link);
	wl_list_remove(&view->request_fullscreen.link);

	free(view);
}

void xdg_toplevel_request_move(
		struct wl_listener *listener, void *data) {
	/* This event is raised when a client would like to begin an interactive
	 * move, typically because the user clicked on their client-side
	 * decorations. Note that a more sophisticated compositor should check the
	 * provided serial against a list of button press serials sent to this
	 * client, to prevent the client from requesting this whenever they want. */
	struct citrus_view *view = wl_container_of(listener, view, request_move);
	begin_interactive(view, CURSOR_MOVE, 0);
}

void xdg_toplevel_request_resize(
		struct wl_listener *listener, void *data) {
	/* This event is raised when a client would like to begin an interactive
	 * resize, typically because the user clicked on their client-side
	 * decorations. Note that a more sophisticated compositor should check the
	 * provided serial against a list of button press serials sent to this
	 * client, to prevent the client from requesting this whenever they want. */
	struct wlr_xdg_toplevel_resize_event *event = data;
	struct citrus_view *view = wl_container_of(listener, view, request_resize);
	begin_interactive(view, CURSOR_RESIZE, event->edges);
}

void xdg_toplevel_request_maximize(
		struct wl_listener *listener, void *data) {
	/* This event is raised when a client would like to maximize itself,
	 * typically because the user clicked on the maximize button on
	 * client-side decorations. tinywl doesn't support maximization, but
	 * to conform to xdg-shell protocol we still must send a configure.
	 * wlr_xdg_surface_schedule_configure() is used to send an empty reply. */
	struct citrus_view *view =
		wl_container_of(listener, view, request_maximize);
	wlr_xdg_surface_schedule_configure(view->xdg_toplevel->base);
}

void xdg_toplevel_request_fullscreen(
		struct wl_listener *listener, void *data) {
	/* Just as with request_maximize, we must send a configure here. */
	struct citrus_view *view =
		wl_container_of(listener, view, request_fullscreen);
	wlr_xdg_surface_schedule_configure(view->xdg_toplevel->base);
}


