#! /usr/bin/env python3

import xcffib
import xcffib.xproto
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
                        0, 0, 200, 200,    # x,y,w,h
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
                        5,                 # border
                        xcffib.xproto.WindowClass.InputOutput, # window class
                        screen.root_visual, #visual
                        xcffib.xproto.CW.BackPixel |
                        xcffib.xproto.CW.EventMask, # mask
                        events)

surface = cairocffi.XCBSurface (conn, window_deco_id,
                                screen.allowed_depths[0].visuals[0],
                                200, 200)
context = cairocffi.Context(surface)

context.set_source_rgba( 0, 1, 1, 1)

context.rectangle( 20, 20, 40, 40)
context.fill()
surface.flush()
conn.core.ReparentWindow( window_id, window_deco_id, 0, 5)
conn.core.MapWindow(window_id)
conn.core.MapWindow(window_deco_id)
conn.flush()
time.sleep( 2)


