#! /usr/bin/env python3

import xcffib
import xcffib.xproto
import cairocffi.xcb
import cairocffi
import time

# display server open
conn = xcffib.connect() # open connection
screen = conn.get_setup().roots[0] # get root screen



# display server client create
window_id = conn.generate_id()
events = [screen.black_pixel,
          xcffib.xproto.EventMask.ButtonPress |
          xcffib.xproto.EventMask.ButtonRelease |
          xcffib.xproto.EventMask.EnterWindow |
          xcffib.xproto.EventMask.LeaveWindow |
          xcffib.xproto.EventMask.Exposure |
          xcffib.xproto.EventMask.PointerMotion |
          xcffib.xproto.EventMask.ButtonMotion |
          xcffib.xproto.EventMask.KeyPress |
          xcffib.xproto.EventMask.KeyRelease]

conn.core.CreateWindow( screen.root_depth, # color depth
                        window_id,
                        screen.root,       # root window
                        40, 40, 100, 100,    # x,y,w,h
                        1,                 # border
                        xcffib.xproto.WindowClass.InputOutput, # window class
                        screen.root_visual, #visual
                        xcffib.xproto.CW.BackPixel |
                        xcffib.xproto.CW.EventMask, # mask
                        events)

window_deco_id = conn.generate_id()
events = [screen.white_pixel,
          xcffib.xproto.EventMask.ButtonPress |
          xcffib.xproto.EventMask.ButtonRelease |
          xcffib.xproto.EventMask.EnterWindow |
          xcffib.xproto.EventMask.LeaveWindow |
          xcffib.xproto.EventMask.Exposure |
          xcffib.xproto.EventMask.PointerMotion |
          xcffib.xproto.EventMask.ButtonMotion |
          xcffib.xproto.EventMask.KeyPress |
          xcffib.xproto.EventMask.KeyRelease]


conn.core.CreateWindow( screen.root_depth, # color depth
                        window_deco_id,
                        screen.root,       # root window
                        0, 0, 200, 200,    # x,y,w,h
                        10,                 # border
                        xcffib.xproto.WindowClass.InputOutput, # window class
                        screen.root_visual, #visual
                        xcffib.xproto.CW.BackPixel |
                        xcffib.xproto.CW.EventMask, # mask
                        events)

surface = cairocffi.xcb.XCBSurface (conn, window_deco_id,
                                screen.allowed_depths[0].visuals[0],
                                400, 400)
context = cairocffi.Context(surface)

context.set_source_rgb( 0.5, 0.5, 0.5)
context.paint()
#context.new_path()
#context.rectangle( 0, 0, 400, 400)
#context.fill()
surface.flush()
conn.core.ReparentWindow( window_id, window_deco_id, 50, 50)
conn.core.MapWindow(window_id)
conn.core.MapWindow(window_deco_id)
conn.flush()
surface.flush()
conn.flush()
time.sleep( 2)


