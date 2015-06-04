/* push button widget
 *
 * Copyright (C) 2013 Robin Gareus <robin@gareus.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef _ROB_TK_PBTN_H_
#define _ROB_TK_PBTN_H_

typedef struct {
	RobWidget* rw;

	bool sensitive;
	bool prelight;
	bool enabled;

	bool (*cb) (RobWidget* w, void* handle);
	void* handle;
	bool (*cb_up) (RobWidget* w, void* handle);
	void* handle_up;
	bool (*cb_down) (RobWidget* w, void* handle);
	void* handle_down;

	cairo_pattern_t* btn_active;
	cairo_pattern_t* btn_inactive;
	cairo_surface_t* sf_txt;

	float w_width, w_height, l_width, l_height;
	float fg[4];
	float bg[4];
	pthread_mutex_t _mutex;
} RobTkPBtn;

static bool robtk_pbtn_expose_event(RobWidget* handle, cairo_t* cr, cairo_rectangle_t* ev) {
	RobTkPBtn * d = (RobTkPBtn *)GET_HANDLE(handle);

	if (pthread_mutex_trylock (&d->_mutex)) {
		queue_draw(d->rw);
		return TRUE;
	}

	cairo_rectangle (cr, ev->x, ev->y, ev->width, ev->height);
	cairo_clip (cr);

	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

	if (!d->sensitive) {
		cairo_set_source_rgb (cr, d->bg[0], d->bg[1], d->bg[2]);
	} else if (d->enabled) {
		cairo_set_source(cr, d->btn_active);
	} else {
		cairo_set_source(cr, d->btn_inactive);
	}

	rounded_rectangle(cr, 2.5, 2.5, d->w_width - 4, d->w_height -4, C_RAD);
	cairo_fill_preserve (cr);
	cairo_set_line_width (cr, .75);
	cairo_set_source_rgba (cr, .0, .0, .0, 1.0);
	cairo_stroke(cr);

	if (d->enabled) {
		cairo_set_operator (cr, CAIRO_OPERATOR_XOR);
	} else {
		cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
	}
	const float xalign = rint((d->w_width - d->l_width) * d->rw->xalign);
	const float yalign = rint((d->w_height - d->l_height) * d->rw->yalign);
	cairo_set_source_surface(cr, d->sf_txt, xalign, yalign);
	cairo_paint (cr);

	if (d->sensitive && d->prelight) {
		cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
		cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, .1);
		rounded_rectangle(cr, 2.5, 2.5, d->w_width - 4, d->w_height -4, C_RAD);
		cairo_fill_preserve(cr);
		cairo_set_line_width (cr, .75);
		cairo_set_source_rgba (cr, .0, .0, .0, 1.0);
		cairo_stroke(cr);
	}

	pthread_mutex_unlock (&d->_mutex);
	return TRUE;
}

/******************************************************************************
 * UI callbacks
 */

static RobWidget* robtk_pbtn_mousedown(RobWidget *handle, RobTkBtnEvent *event) {
	RobTkPBtn * d = (RobTkPBtn *)GET_HANDLE(handle);
	if (!d->sensitive) { return NULL; }
	if (!d->prelight) { return NULL; }
	d->enabled = TRUE;
	if (d->cb_down) d->cb_down(d->rw, d->handle_down);
	queue_draw(d->rw);
	return handle;
}

static RobWidget* robtk_pbtn_mouseup(RobWidget *handle, RobTkBtnEvent *event) {
	RobTkPBtn * d = (RobTkPBtn *)GET_HANDLE(handle);
	if (!d->sensitive) { return NULL; }
	if (d->enabled && d->cb_up) {
		d->cb_up(d->rw, d->handle_up);
	}
	if (d->prelight && d->enabled) {
		if (d->cb) d->cb(d->rw, d->handle);
	}
	d->enabled = FALSE;
	queue_draw(d->rw);
	return NULL;
}

static void robtk_pbtn_enter_notify(RobWidget *handle) {
	RobTkPBtn * d = (RobTkPBtn *)GET_HANDLE(handle);
	if (!d->prelight) {
		d->prelight = TRUE;
		queue_draw(d->rw);
	}
}

static void robtk_pbtn_leave_notify(RobWidget *handle) {
	RobTkPBtn * d = (RobTkPBtn *)GET_HANDLE(handle);
	if (d->prelight) {
		d->prelight = FALSE;
		queue_draw(d->rw);
	}
}

static void create_pbtn_pattern(RobTkPBtn * d) {
	pthread_mutex_lock (&d->_mutex);
	if (d->btn_active) cairo_pattern_destroy(d->btn_active);
	if (d->btn_inactive) cairo_pattern_destroy(d->btn_inactive);

	d->btn_inactive = cairo_pattern_create_linear (0.0, 0.0, 0.0, d->w_height);
	cairo_pattern_add_color_stop_rgb (d->btn_inactive, ISBRIGHT(d->bg) ? 0.5 : 0.0, SHADE_RGB(d->bg, 1.95));
	cairo_pattern_add_color_stop_rgb (d->btn_inactive, ISBRIGHT(d->bg) ? 0.0 : 0.5, SHADE_RGB(d->bg, 0.75));

	d->btn_active = cairo_pattern_create_linear (0.0, 0.0, 0.0, d->w_height);
	cairo_pattern_add_color_stop_rgb (d->btn_active, ISBRIGHT(d->bg) ? 0.5 : 0.0, SHADE_RGB(d->bg, .95));
	cairo_pattern_add_color_stop_rgb (d->btn_active, ISBRIGHT(d->bg) ? 0.0 : 0.5, SHADE_RGB(d->bg, 2.4));
	pthread_mutex_unlock (&d->_mutex);
}

static void create_pbtn_text_surface(RobTkPBtn * d, const char * txt, PangoFontDescription *font) {
	pthread_mutex_lock (&d->_mutex);
	if (d->sf_txt) {
		cairo_surface_destroy(d->sf_txt);
	}
	d->sf_txt = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, d->w_width, d->w_height);
	cairo_t *cr = cairo_create (d->sf_txt);
	cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
	cairo_paint (cr);
	cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

	if (!font) {
		font = get_font_from_theme();
	}

	write_text_full(cr, txt, font,
			d->w_width / 2.0 + 1,
			d->w_height / 2.0 + 1, 0, 2, d->fg);
	cairo_destroy (cr);
	pthread_mutex_unlock (&d->_mutex);

	pango_font_description_free(font);
}

/******************************************************************************
 * RobWidget stuff
 */

static void
priv_pbtn_size_request(RobWidget* handle, int *w, int *h) {
	RobTkPBtn * d = (RobTkPBtn *)GET_HANDLE(handle);
	*w = d->l_width;
	*h = d->l_height;
}

static void
priv_pbtn_size_allocate(RobWidget* handle, int w, int h) {
	RobTkPBtn * d = (RobTkPBtn *)GET_HANDLE(handle);
	bool recreate_patterns = FALSE;
	if (h != d->w_height) recreate_patterns = TRUE;
	d->w_width = w;
	d->w_height = h;
	if (recreate_patterns) create_pbtn_pattern(d);
	robwidget_set_size(handle, d->w_width, d->w_height);
}

/******************************************************************************
 * public functions
 */

static RobTkPBtn * robtk_pbtn_new_with_colors(const char * txt, const float bg[4], const float fg[4]) {
	assert(txt);
	RobTkPBtn *d = (RobTkPBtn *) malloc(sizeof(RobTkPBtn));

	d->cb = NULL;
	d->handle = NULL;
	d->cb_up = NULL;
	d->handle_up = NULL;
	d->cb_down = NULL;
	d->handle_down = NULL;
	d->sf_txt = NULL;
	d->sensitive = TRUE;
	d->prelight = FALSE;
	d->enabled = FALSE;
	pthread_mutex_init (&d->_mutex, 0);

	d->btn_active = NULL;
	d->btn_inactive = NULL;
	d->sf_txt = NULL;

	memcpy(d->bg, bg, 4 * sizeof(float));
	memcpy(d->fg, fg, 4 * sizeof(float));

	int ww, wh;
	PangoFontDescription *fd = get_font_from_theme();

	get_text_geometry(txt, fd, &ww, &wh);
	d->w_width = ww + 14;
	d->w_height = wh + 8;
	d->l_width = d->w_width;
	d->l_height = d->w_height;

	create_pbtn_text_surface(d, txt, fd); // free's fd.

	d->rw = robwidget_new(d);
	ROBWIDGET_SETNAME(d->rw, "pbtn");
	robwidget_set_alignment(d->rw, 0, .5);

	robwidget_set_size_request(d->rw, priv_pbtn_size_request);
	robwidget_set_size_allocate(d->rw, priv_pbtn_size_allocate);
	robwidget_set_expose_event(d->rw, robtk_pbtn_expose_event);
	robwidget_set_mouseup(d->rw, robtk_pbtn_mouseup);
	robwidget_set_mousedown(d->rw, robtk_pbtn_mousedown);
	robwidget_set_enter_notify(d->rw, robtk_pbtn_enter_notify);
	robwidget_set_leave_notify(d->rw, robtk_pbtn_leave_notify);

	create_pbtn_pattern(d);
	return d;
}

static RobTkPBtn * robtk_pbtn_new(const char * txt) {
	float fg[4];
	float bg[4];
	get_color_from_theme(0, fg);
	get_color_from_theme(1, bg);
	return robtk_pbtn_new_with_colors(txt, bg, fg);
}

static void robtk_pbtn_destroy(RobTkPBtn *d) {
	robwidget_destroy(d->rw);
	cairo_pattern_destroy(d->btn_active);
	cairo_pattern_destroy(d->btn_inactive);
	cairo_surface_destroy(d->sf_txt);
	pthread_mutex_destroy(&d->_mutex);
	free(d);
}

static void robtk_pbtn_set_alignment(RobTkPBtn *d, float x, float y) {
	robwidget_set_alignment(d->rw, x, y);
}

static RobWidget * robtk_pbtn_widget(RobTkPBtn *d) {
	return d->rw;
}

static void robtk_pbtn_set_callback(RobTkPBtn *d, bool (*cb) (RobWidget* w, void* handle), void* handle) {
	d->cb = cb;
	d->handle = handle;
}

static void robtk_pbtn_set_callback_up(RobTkPBtn *d, bool (*cb) (RobWidget* w, void* handle), void* handle) {
	d->cb_up = cb;
	d->handle_up = handle;
}

static void robtk_pbtn_set_callback_down(RobTkPBtn *d, bool (*cb) (RobWidget* w, void* handle), void* handle) {
	d->cb_down = cb;
	d->handle_down = handle;
}

static void robtk_pbtn_set_sensitive(RobTkPBtn *d, bool s) {
	if (d->sensitive != s) {
		d->sensitive = s;
		queue_draw(d->rw);
	}
}

static void robtk_pbtn_set_text(RobTkPBtn *d, const char *txt) {
	create_pbtn_text_surface (d, txt, NULL);
	queue_draw(d->rw);
}

static void robtk_pbtn_set_bg(RobTkPBtn *d, float r, float g, float b, float a) {
	if (d->bg[0] == r && d->bg[1] == g && d->bg[2] == b && d->bg[3] == a) {
		return;
	}
	d->bg[0] = r;
	d->bg[1] = g;
	d->bg[2] = b;
	d->bg[3] = a;
	create_pbtn_pattern(d);
	queue_draw(d->rw);
}

static bool robtk_pbtn_get_pushed(RobTkPBtn *d) {
	return (d->enabled);
}
#endif