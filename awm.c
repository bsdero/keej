#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <errno.h>

#include <cairo.h>
#include <cairo-xcb.h>

#include <xcb/xcb.h>
#include <xcb/xcb_image.h>
#include <xcb/xcb_aux.h>
#include <xcb/xcb_icccm.h>


/*
 * Move this many pixels when moving or resizing with keyboard unless
 * the window has hints saying otherwise.
 */
#define MOVE_STEP 32

/*
 * Use this modifier combined with other keys to control wm from
 * keyboard. Default is Mod4, which on my keyboard is the Alt key but
 * is usually the Windows key on more normal keyboard layouts.
 */
#define MODKEY XCB_MOD_MASK_4

/* Extra modifier for resizing. Default is Shift. */
#define SHIFTMOD XCB_MOD_MASK_SHIFT

/*
 * Modifier key to use with mouse buttons. Default Mod1, Meta on my
 * keyboard.
 */
#define MOUSEMODKEY XCB_MOD_MASK_1



//#include "trace.h"


typedef struct{
    xcb_connection_t *connection;
    xcb_screen_t *screen;
    int screen_nbr;
}display_server_t;


struct dsclient_s{
    xcb_window_t window_id; 
    int x, y; 
    int w, h;
    int border;
    display_server_t *ds;
    struct dsclient_s *content;
    xcb_visualtype_t *visual_type;
    cairo_surface_t *cairo_surface;
    cairo_t *cairo;    
};

typedef struct dsclient_s dsclient_t; 

float window_bar_color[] = { 0.35, 0, 0.63, 1.0 };
float window_buttons_colors[]= { 
	0.8,   0.0,  0.0,
    0.0,   0.0,  0.8,
    0.8,   0.2,  0.8, 
    0.2,   0.8,  0.8,
    0.64, 0.41,  0.8 };


int ds_open( display_server_t *ds){
    xcb_connection_t     *c;
    xcb_screen_t         *screen;
    int                   screen_nbr;
    xcb_screen_iterator_t iter;

    /* Open the connection to the X server. Use the DISPLAY environment variable */
    c = xcb_connect (NULL, &screen_nbr);

    if (xcb_connection_has_error(c)) {
        printf("Error opening display. c=%p\n", c);
        exit(1);
    }

    /* Get the screen #screen_nbr */
    iter = xcb_setup_roots_iterator (xcb_get_setup (c));
    for (; iter.rem; --screen_nbr, xcb_screen_next (&iter))
        if (screen_nbr == 0) {
            screen = iter.data;
        break;
    }

    #ifdef DEBUG_DS
        printf ("\n");
        printf ("Informations of screen %u:\n", screen->root);
        printf ("  width.........: %d\n", screen->width_in_pixels);
        printf ("  height........: %d\n", screen->height_in_pixels);
        printf ("  white pixel...: %u\n", screen->white_pixel);
        printf ("  black pixel...: %u\n", screen->black_pixel);
        printf ("\n");	
    #endif

    ds->connection = c;
    ds->screen = screen;
    ds->screen_nbr = screen_nbr;

    return(0);
}

int ds_close( display_server_t *ds){
    xcb_disconnect( ds->connection);
    return(0);
}

int dsclient_create( display_server_t *ds, dsclient_t *dsc, 
                     int x, int y, int w, int h, int border){
    unsigned int prop_name = XCB_CW_BACK_PIXEL;
    unsigned int prop_value = ds->screen->white_pixel;


    dsc->window_id = xcb_generate_id( ds->connection);
    dsc->x = x;
    dsc->y = y;
    dsc->w = w;
    dsc->h = h;
    dsc->border = border;


    xcb_create_window( ds->connection, /* connection */
                       ds->screen->root_depth, /* depth */
                       dsc->window_id,  
                       ds->screen->root, /* parent window */ 
                       dsc->x, dsc->y, dsc->w, dsc->h,   /* x, y, w, h */
                       dsc->border,                /* border width */
                       XCB_WINDOW_CLASS_INPUT_OUTPUT, /* window class */
                       ds->screen->root_visual,  /* visual */
                       prop_name, /* masks */
                       &prop_value);

    dsc->ds = ds;
    dsc->content = NULL;
    dsc->cairo = NULL;
    dsc->cairo_surface = NULL;
    return(0);
}

int dsclient_map( dsclient_t *dsc){
    xcb_connection_t *connection = dsc->ds->connection;


    /* Map the window on the screen */
    xcb_map_window ( connection, dsc->window_id);

    if( dsc->content != NULL){	
        /* Map the window on the screen */
        xcb_map_window ( connection, dsc->content->window_id);
    }

    /* Make sure commands are sent before we pause, so window is shown */
    xcb_flush ( connection);
}


int dsclient_reparent( dsclient_t *dsc, dsclient_t *dscr){
    xcb_get_geometry_reply_t *tbm_window_geo;
    xcb_screen_t *screen = dsc->ds->screen;
    xcb_connection_t *connection = dsc->ds->connection;
    xcb_depth_iterator_t depth_iter;
    unsigned int prop_name = XCB_CW_BACK_PIXEL;
    unsigned int prop_value = screen->white_pixel;
    int h = 24;

    tbm_window_geo = xcb_get_geometry_reply( connection, 
                                             xcb_get_geometry( connection, 
                                                               dsc->window_id),
                                             NULL);

    dscr->window_id = xcb_generate_id( connection);

    dscr->x = tbm_window_geo->x;
    dscr->y = tbm_window_geo->y;
    dscr->w = tbm_window_geo->width;
    dscr->h = tbm_window_geo->height + h;
    dscr->border = dsc->border;
    dscr->ds = dsc->ds;
   
    /* use this with XCB_MAP_REQUEST instead XCB_CREATE_NOTIFY */

    xcb_create_window( connection, /* connection */
                       screen->root_depth, /* depth */
                       dscr->window_id,  
                       dscr->ds->screen->root, /* parent window */ 
                       dscr->x, dscr->y, dscr->w, dscr->h,   /* x, y, w, h */
                       dscr->border,                /* border width */
                       XCB_WINDOW_CLASS_INPUT_OUTPUT, /* window class */
                       screen->root_visual,  /* visual */
                       prop_name, /* masks */
                       &prop_value);


    xcb_reparent_window( connection, dsc->window_id, dscr->window_id, 0, h);
    dscr->content = dsc; 
    
    dsclient_map( dscr);
    depth_iter = xcb_screen_allowed_depths_iterator( screen);
    for (; depth_iter.rem; xcb_depth_next(&depth_iter)) {
        xcb_visualtype_iterator_t visual_iter;

        visual_iter = xcb_depth_visuals_iterator(depth_iter.data);
        for (; visual_iter.rem; xcb_visualtype_next(&visual_iter)) {
            if ( screen->root_visual == visual_iter.data->visual_id) {
                dscr->visual_type = visual_iter.data;
                break;
            }
        }
    }

    
    dscr->cairo_surface = cairo_xcb_surface_create ( connection, 
                                                     dscr->window_id, 
                                                     dscr->visual_type, 
                                                     dscr->w, 
                                                     dscr->h );

    cairo_t *cr;                                                
    dscr->cairo = cr = cairo_create ( dscr->cairo_surface);

    float *colors = window_buttons_colors;
    float *bcolor = window_bar_color;    
    int i, j;
    
    cairo_set_source_rgba (cr, bcolor[0], bcolor[1], bcolor[2], bcolor[3]);
    cairo_new_path (cr);
    cairo_rectangle (cr, 0, 0, dscr->w, h);
    cairo_fill (cr);
    for( i = 0; i < 5; i++){
        j = i * 3;
        cairo_set_source_rgba (cr, colors[j+0], colors[j+1], colors[j+2], 1.0);
        cairo_new_path (cr);
        cairo_set_line_width (cr, 6.0);
        cairo_arc (cr, 
                   dscr->w - (h * (i+1)), 
                   h / 2, 
                   h * 0.5 / 2, 
                   0, 
                   2*3.141592);
        cairo_fill (cr);
    }

    cairo_select_font_face ( dscr->cairo, 
                             "serif", 
                             CAIRO_FONT_SLANT_NORMAL, 
                             CAIRO_FONT_WEIGHT_BOLD);
                             
    cairo_set_font_size ( dscr->cairo, h - 8);
    cairo_set_source_rgb ( dscr->cairo, 1.0, 1.0, 1.0);
    cairo_move_to ( dscr->cairo, h, h - 6);
    cairo_show_text ( dscr->cairo, "Hello, world");
    cairo_surface_flush( dscr->cairo_surface);

    return(0);
}


int dsclient_remove( dsclient_t *dscr){
	if( dscr->cairo != NULL){
        cairo_destroy( dscr->cairo);
    }
    
    if( dscr->cairo_surface != NULL){
        cairo_surface_destroy( dscr->cairo_surface);
    }
    
    return(0);
}

int ds_suscribe_events( display_server_t *ds){
	
   /* Grab mouse buttons. */

    xcb_grab_button( ds->connection, 0, ds->screen->root, 
                     XCB_EVENT_MASK_BUTTON_PRESS
                       | XCB_EVENT_MASK_BUTTON_RELEASE,
                     XCB_GRAB_MODE_ASYNC, 
                     XCB_GRAB_MODE_ASYNC, 
                     ds->screen->root, XCB_NONE,
                    1 /* left mouse button */,
                    MOUSEMODKEY);
	
}


int main( int argc, char **argv){
    display_server_t ds;
    
    dsclient_t dsc, dscr;
    uint32_t mask = 0;


    ds_open( &ds);
    ds_suscribe_events( &ds);


    dsclient_create( &ds, &dsc, 0, 0, 300, 100, 1);
    dsclient_reparent( &dsc, &dscr);

    dsclient_map( &dscr);

    pause();

    dsclient_remove( &dscr);
    ds_close( &ds);
    
    
    return( 0);
}


