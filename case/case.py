# %%
from operator import itemgetter
from build123d import *
from ocp_vscode import *

# %%
# Clock Enclosure
# Width between M3 screws is 182.88 mm, total width 189.992 mm, with antenna 195.326 mm
# Height between M3 screws is 42.901 mm, total height 50.013 mm

# Enclosure dimensions
width = 215.4
height = 75.4
length = 4 * IN

# Window dimensions
window_width = 190
window_height = 50

# ledge height
ledge = 1 / 2 * IN

# Enclosure wall thickness
wall_thickness = 3 / 16 * IN  # 1/4 inch in mm

# Inner dimensions of the enclosure
inner_width = width - 2 * wall_thickness
inner_height = height - 2 * wall_thickness
inner_length = length - 2 * wall_thickness

# Radii
outer_radius = 10
inner_radius = 5.25
window_radius = 3.5

# Create the box
box = Box(width, length, height)

# Create the cuttout box (inner box)
# cuttout_box = Pos(0, 3, 0) * Box(inner_width, length, inner_height)
cuttout_box = Pos(0, -4.5, 0) * Box(inner_width, length - 15, inner_height)

# Subtract the PCB box from the outer box to create the enclosure walls
enclosure = box - cuttout_box

enclosure = fillet(
    itemgetter(0, 1, 2, 3)(enclosure.edges().filter_by(Axis.Y)), radius=outer_radius
)
enclosure = fillet(
    itemgetter(8, 9, 10, 11)(enclosure.edges().filter_by(Axis.Y)), radius=inner_radius
)

window = Pos(0, -length / 2, 0) * Rectangle(
    window_width, window_height, rotation=(90, 0, 0)
)
window = fillet(window.vertices(), radius=window_radius)

# Cutt out the window
enclosure -= extrude(window, amount=3, dir=(0, 1, 0))

rear_panel = Pos(0, length / 2, 0) * Rectangle(
    inner_width, inner_height, rotation=(90, 0, 0)
)
rear_panel = fillet(rear_panel.vertices(), radius=inner_radius)

# Cutt out rear panel
enclosure -= extrude(rear_panel, amount=6, dir=(0, -1, 0))

rear_cuttout_pts = [
    (inner_width / 2, inner_height / 2 - 10),
    (inner_width / 2 - 10, inner_height / 2 - 10),
    (inner_width / 2 - 10, inner_height / 2),
    (-inner_width / 2 + 10, inner_height / 2),
    (-inner_width / 2 + 10, inner_height / 2 - 10),
    (-inner_width / 2, inner_height / 2 - 10),
    (-inner_width / 2, -inner_height / 2 + 10),
    (-inner_width / 2 + 10, -inner_height / 2 + 10),
    (-inner_width / 2 + 10, -inner_height / 2),
    (inner_width / 2 - 10, -inner_height / 2),
    (inner_width / 2 - 10, -inner_height / 2 + 10),
    (inner_width / 2, -inner_height / 2 + 10),
]

l1 = Polyline(*rear_cuttout_pts)
l2 = Line(l1 @ 0, l1 @ 1)
line = l1 + l2
face = Pos(0, length / 2 - 6, 0) * make_face(line).rotate(Axis.X, 90)
face = fillet(itemgetter(1, 4, 7, 10)(face.vertices()), radius=3)

enclosure -= extrude(face, amount=6, dir=(0, -1, 0))

show(enclosure)

# TODO, import both clock and converter PCBs, Nixie Tubes, sockets,...
