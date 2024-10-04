#!/usr/bin/env python

# config and defaults

nogui = False

screen_w = 640
screen_h = 480
tile_count = 4

# main code

import sys
from math import sqrt
from dataclasses import dataclass, field, KW_ONLY
import tkinter as tk

@dataclass(kw_only = True)
class tile:
    w: int = 0
    h: int = 0
    x: int = 0
    y: int = 0

@dataclass(kw_only = True)
class col:
    w: int = 0
    x: int = 0
    tiles: list[tile] = field(default_factory = list)

def split_screen(screen_w: int, screen_h: int, tile_count: int) -> list[tile]:
    screen_area = screen_w * screen_h;                  # calc the total screen area
    if screen_area < tile_count:                        # prevent a divide by zero
        print("Screen size too small")
        return [tile(w = 1, h = 1)]
    tile_area = screen_area // tile_count;              # calc the target tile area by dividing the total area into a 'tile_count' amount of smaller chunks
    tile_width = int(sqrt(tile_area));                  # calc the initial tile width by finding the sqrt of the target tile area
    col_count = screen_w // tile_width;                 # calc how many columns can fit in the screen by dividing the screen width by the initial tile width
    if col_count < 1: col_count = 1                     # prevent another div by zero
    if col_count > tile_count: col_count = tile_count   # there can't be more columns than tiles
    base_tiles_per_col = tile_count // col_count        # calc the minimum amount tiles to be in each column 
    leftover_tiles_count = tile_count % col_count       # calc the amount of tiles left over after the divide (to distribute among the columns)

    cols = []                   # create a list of columns
    for i in range(col_count):  # fill the list of columns
        col_tile_count = base_tiles_per_col # start with the minimum amount of tiles
        if leftover_tiles_count > 0:        # if there are leftovers, take one
            leftover_tiles_count -= 1
            col_tile_count += 1

        c = col()                                                   # create a new column
        if i > 0: c.x = cols[i - 1].x + cols[i - 1].w               # if not the first column, calculate the x position using the previous column (the first will always have an x position of 0)
        if i < col_count - 1:                                       # for the all columns except the last,
            c.w = (screen_w - c.x) * col_tile_count // tile_count   #   take an amount of the remaining width proportional to the tile count (more tiles to pack vertically = more horizontal space to even it out)
            tile_count -= col_tile_count                            #   take away that tile count from the total
        else:                                                       # for the last column,
            c.w = screen_w - c.x                                    #   take the remaining width

        c.tiles = []                            # create a list of tiles under this column
        remaining_col_tiles = col_tile_count    # remember the amount of tiles left
        for j in range(col_tile_count):         # fill the list of tiles
            t = tile(w = c.w, x = c.x)                          # create a new tile with the same x position and width as the column
            if j > 0: t.y = c.tiles[j - 1].y + c.tiles[j - 1].h # if not the first tile, calculate the y position using the previous tile (the first will always have a y position of 0)
            if j < col_tile_count - 1:                          # for all the tiles except the last,
                t.h = (screen_h - t.y) // remaining_col_tiles   #   take an even amount of the remaining height
                remaining_col_tiles -= 1                        #   take away 1 from the remaining tile count
            else:                                               # for the last tile,
                t.h = screen_h - t.y                            #   take the remaining height
            c.tiles.append(t)                                   # add the tile to the tile list

        cols.append(c)  # add the column to the list

    return [t for c in cols for t in c.tiles]   # collect all the tiles in all the columns

if len(sys.argv) == 4:
    if sys.argv[1]: screen_w = int(sys.argv[1])
    if sys.argv[2]: screen_h = int(sys.argv[2])
    if sys.argv[3]: tile_count = int(sys.argv[3])
elif len(sys.argv) > 1:
    print(f"Usage: {sys.argv[0]} <WIDTH> <HEIGHT> <TILE_COUNT>")
    exit()

print(f"Splitting a {screen_w}x{screen_h} screen into {tile_count} tiles...")
tiles = split_screen(screen_w, screen_h, tile_count)
tclen = len(str(len(tiles)))
mxlen = 0
mylen = 0
malen = 0
mslen = 0
for t in tiles:
    l = len(str(t.x))
    if l > mxlen: mxlen = l
    l = len(str(t.y))
    if l > mylen: mylen = l
    l = len(f"{t.w * t.h}")
    if l > malen: malen = l
    l = len(f"{t.w}x{t.h}")
    if l > mslen: mslen = l
for i, t in enumerate(tiles):
    print("  " + str(i + 1).rjust(tclen) + ". \t(" + str(t.x).ljust(mxlen) + ", " + str(t.y).ljust(mylen) + ")  -  " + f"{t.w}x{t.h}".ljust(mslen) + " (" + f"{t.w * t.h}".rjust(malen) + ")")

# quick and dirty graphical stuff (ignore all this below)

if nogui: exit()

root = tk.Tk()
root.title("Loading...")
root.geometry(f"{screen_w}x{screen_h}")
canv = tk.Canvas(root, width = screen_w, height = screen_h, background = "black", highlightthickness = 0)
canv.pack()

def draw(tiles):
    root.title(f"{screen_w}x{screen_h} ({screen_w * screen_h}), {len(tiles)} tiles")
    for t in tiles:
        #h = hash((t.x, t.y, t.w, t.h))
        h = hash((t.w * t.h,))
        c = f"#{(h & 127) + 16 :02x}{(h >> 8 & 127) + 16 :02x}{(h >> 16 & 127) + 16 :02x}"
        canv.create_rectangle(t.x + 2, t.y + 2, t.x + t.w - 3, t.y + t.h - 3, outline = "white", fill = c, width = 5)
        canv.create_text(t.x + t.w / 2, t.y + t.h / 2, text = f"{t.w}×{t.h}\n({t.w * t.h})", font = (None, 12, "bold"), fill = "white")
    l = canv.create_text(0, 0, text = f"{screen_w}x{screen_h} ({screen_w * screen_h}), {len(tiles)} tiles", font = (None, 12, "bold"), fill = "white")
    canv.moveto(l, 0, 0)
    r = canv.create_rectangle(canv.bbox(l), fill = "black", width = 0)
    canv.tag_lower(r, l)
def redraw():
    canv.delete("all")
    draw(split_screen(screen_w, screen_h, tile_count))

draw(tiles)

def config(e):
    global root, canv, screen_w, screen_h, drawnew
    if e.widget == root and (screen_w != e.width or screen_h != e.height):
        screen_w = e.width
        screen_h = e.height
        canv.configure(width = screen_w, height = screen_h)
        redraw()
def keydown(e):
    global tile_count, drawnew
    if (e.char == '-' or e.char == '_') and tile_count > 1:
        tile_count -= 1
        redraw()
    elif e.char == '=' or e.char == '+':
        tile_count += 1
        redraw()

root.bind("<Configure>", config)
root.bind("<KeyPress>", keydown)

root.mainloop()
