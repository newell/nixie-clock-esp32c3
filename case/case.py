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
length = 3.5 * IN

# Window dimensions
window_width = 190
window_height = 50

# ledge height
# ledge = 1 / 2 * IN

# Enclosure wall thickness
wall_thickness = 3 / 16 * IN  # 1/4 inch in mm

# Inner dimensions of the enclosure
inner_width = width - 2 * wall_thickness
inner_height = height - 2 * wall_thickness
inner_length = length - 2 * wall_thickness

# Radii for enclosure fillets
outer_radius = 10
inner_radius = 5.25
window_radius = 3.5

# Create the main box for the enclosure
box = Box(width, length, height)

# Create the cuttout box (inner box) for the enclosure
cuttout_box = Pos(0, -6, 0) * Box(inner_width, length - 22, inner_height)

# Subtract the PCB box from the outer box to create the enclosure walls
enclosure = box - cuttout_box

# Fillet for inner and outer parts of the enclosure.
enclosure = fillet(
    itemgetter(0, 1, 2, 3)(enclosure.edges().filter_by(Axis.Y)), radius=outer_radius
)
enclosure = fillet(
    itemgetter(8, 9, 10, 11)(enclosure.edges().filter_by(Axis.Y)), radius=inner_radius
)

# Window cuttout
window = Pos(0, -length / 2, 0) * Rectangle(
    window_width, window_height, rotation=(90, 0, 0)
)
window = fillet(window.vertices(), radius=window_radius)
enclosure -= extrude(window, amount=5, dir=(0, 1, 0))

# Rear panel cuttout
rear_panel = Pos(0, length / 2, 0) * Rectangle(
    inner_width, inner_height, rotation=(90, 0, 0)
)
rear_panel = fillet(rear_panel.vertices(), radius=inner_radius)
enclosure -= extrude(rear_panel, amount=6, dir=(0, -1, 0))

# Inner rear cuttout
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
enclosure -= extrude(face, amount=11, dir=(0, -1, 0))

# %%
# Clock model
clock = import_step("clock.step")

# IN-12 Nixie tubes
tube_pts = [
    (10.05, 0, 9.5),
    (44.1, 0, 9.5),
    (44.1 + 20.1, 0, 9.5),
    (-10.05, 0, 9.5),
    (-44.1, 0, 9.5),
    (-44.1 - 20.1, 0, 9.5),
]
tube = import_step("IN-12-NixieTube.STEP")
tubes = [Pos(pos) * tube for pos in tube_pts]

# Nixie tube socket pins
importer = Mesher()
socket_pts = [
    # Middle Row
    (4.325, 0, -6.6),
    (4.325 + 11.5, 0, -6.6),
    (4.325 + 11.5 + 22.5, 0, -6.6),
    (4.325 + 2 * 11.5 + 22.5, 0, -6.6),
    (4.325 + 2 * 11.5 + 22.5 + 8.6, 0, -6.6),
    (4.325 + 3 * 11.5 + 22.5 + 8.6, 0, -6.6),
    (-4.325, 0, -6.6),
    (-4.325 - 11.5, 0, -6.6),
    (-4.325 - 11.5 - 22.5, 0, -6.6),
    (-4.325 - 2 * 11.5 - 22.5, 0, -6.6),
    (-4.325 - 2 * 11.5 - 22.5 - 8.6, 0, -6.6),
    (-4.325 - 3 * 11.5 - 22.5 - 8.6, 0, -6.6),
    # 1 up from Middle Row
    (4.325, 4.5, -6.6),
    (4.325 + 11.5, 4.5, -6.6),
    (4.325 + 11.5 + 22.5, 4.5, -6.6),
    (4.325 + 2 * 11.5 + 22.5, 4.5, -6.6),
    (4.325 + 2 * 11.5 + 22.5 + 8.6, 4.5, -6.6),
    (4.325 + 3 * 11.5 + 22.5 + 8.6, 4.5, -6.6),
    (-4.325, 4.5, -6.6),
    (-4.325 - 11.5, 4.5, -6.6),
    (-4.325 - 11.5 - 22.5, 4.5, -6.6),
    (-4.325 - 2 * 11.5 - 22.5, 4.5, -6.6),
    (-4.325 - 2 * 11.5 - 22.5 - 8.6, 4.5, -6.6),
    (-4.325 - 3 * 11.5 - 22.5 - 8.6, 4.5, -6.6),
    # 1 down from Middle Row
    (4.325, -4.5, -6.6),
    (4.325 + 11.5, -4.5, -6.6),
    (4.325 + 11.5 + 22.5, -4.5, -6.6),
    (4.325 + 2 * 11.5 + 22.5, -4.5, -6.6),
    (4.325 + 2 * 11.5 + 22.5 + 8.6, -4.5, -6.6),
    (4.325 + 3 * 11.5 + 22.5 + 8.6, -4.5, -6.6),
    (-4.325, -4.5, -6.6),
    (-4.325 - 11.5, -4.5, -6.6),
    (-4.325 - 11.5 - 22.5, -4.5, -6.6),
    (-4.325 - 2 * 11.5 - 22.5, -4.5, -6.6),
    (-4.325 - 2 * 11.5 - 22.5 - 8.6, -4.5, -6.6),
    (-4.325 - 3 * 11.5 - 22.5 - 8.6, -4.5, -6.6),
    # 2 up from Middle Row
    (6.075, 8, -6.6),
    (6.075 + 8, 8, -6.6),
    (6.075 + 8 + 26, 8, -6.6),
    (6.075 + 2 * 8 + 26, 8, -6.6),
    (6.075 + 2 * 8 + 26 + 12.1, 8, -6.6),
    (6.075 + 3 * 8 + 26 + 12.1, 8, -6.6),
    (-6.075, 8, -6.6),
    (-6.075 - 8, 8, -6.6),
    (-6.075 - 8 - 26, 8, -6.6),
    (-6.075 - 2 * 8 - 26, 8, -6.6),
    (-6.075 - 2 * 8 - 26 - 12.1, 8, -6.6),
    (-6.075 - 3 * 8 - 26 - 12.1, 8, -6.6),
    # 2 down from Middle Row
    (6.075, -8, -6.6),
    (6.075 + 8, -8, -6.6),
    (6.075 + 8 + 26, -8, -6.6),
    (6.075 + 2 * 8 + 26, -8, -6.6),
    (6.075 + 2 * 8 + 26 + 12.1, -8, -6.6),
    (6.075 + 3 * 8 + 26 + 12.1, -8, -6.6),
    (-6.075, -8, -6.6),
    (-6.075 - 8, -8, -6.6),
    (-6.075 - 8 - 26, -8, -6.6),
    (-6.075 - 2 * 8 - 26, -8, -6.6),
    (-6.075 - 2 * 8 - 26 - 12.1, -8, -6.6),
    (-6.075 - 3 * 8 - 26 - 12.1, -8, -6.6),
    # Top Row
    (10.07, 9, -6.6),
    (10.07 + 34, 9, -6.6),
    (10.07 + 34 + 20.1, 9, -6.6),
    (-10.07, 9, -6.6),
    (-10.07 - 34, 9, -6.6),
    (-10.07 - 34 - 20.1, 9, -6.6),
    # Bottom Row
    (10.07, -9, -6.6),
    (10.07 + 34, -9, -6.6),
    (10.07 + 34 + 20.1, -9, -6.6),
    (-10.07, -9, -6.6),
    (-10.07 - 34, -9, -6.6),
    (-10.07 - 34 - 20.1, -9, -6.6),
]
socket = importer.read("SocketPin.stl")[0]
sockets = [Pos(pos) * socket for pos in socket_pts]

# INS-1 indicator tubes
indicator_pts = [
    (27.05, 6, 13),
    (27.05, -6, 13),
    (-27.05, 6, 13),
    (-27.05, -6, 13),
]
indicator = import_step("INS1.STEP").rotate(Axis.X, 90).rotate(Axis.Z, 90)
indicators = [Pos(pos) * indicator for pos in indicator_pts]

# Assemble the board
clock = clock.rotate(Axis.X, 90)
tubes = [tube.rotate(Axis.X, 90) for tube in tubes]
sockets = [socket.rotate(Axis.X, 90) for socket in sockets]
indicators = [indicator.rotate(Axis.X, 90) for indicator in indicators]
board = Pos(0, -15, 0) * Compound(children=[*sockets, clock, *tubes, *indicators])

# High Voltage FLyback Converter model
converter = Pos(-60, 14.5, -25) * import_step("converter.step").rotate(Axis.Z, 90)

## Risers
clock_riser_pts = [
    # Clock
    (inner_width / 2 - 20, -16, -inner_height / 2 + (5 / 16 * IN) / 2),
    (-inner_width / 2 + 20, -16, -inner_height / 2 + (5 / 16 * IN) / 2),
]
converter_riser_pts = [
    # High Voltage Flyback Converter
    (37.34 / 2 - 60, 14.5 + 16, -inner_height / 2 + (5 / 16 * IN) / 2),
    (-37.34 / 2 - 60, 14.5 + 16, -inner_height / 2 + (5 / 16 * IN) / 2),
    (-37.34 / 2 - 60, 14.5 - 16, -inner_height / 2 + (5 / 16 * IN) / 2),
    (37.34 / 2 - 60, 14.5 - 16, -inner_height / 2 + (5 / 16 * IN) / 2),
]
riser = Box(10, 10, 5 / 16 * IN)
fillet(riser.edges().filter_by(Axis.Z), radius=2)
clock_risers = [Pos(pos) * riser for pos in clock_riser_pts]
clock_risers = [
    fillet(riser.edges().filter_by(Axis.Z), radius=2) for riser in clock_risers
]
# Need M3 holes below for converter risers -- see below

# Front panel
front_panel = Rectangle(inner_width + -1 / 16 * IN, inner_height - 1 / 16 * IN)
front_panel = fillet(front_panel.vertices(), radius=inner_radius)
indicator_circle = Circle(7.5 / 2)
front_panel -= [
    Pos(pos) * indicator_circle
    for pos in [
        (27.05, 6),
        (27.05, -6),
        (-27.05, 6),
        (-27.05, -6),
    ]
]
arc1 = ThreePointArc((-21, 11.5), (-21 / 2, 15.75), (0, 12.5))
arc2 = ThreePointArc((21, -11.5), (21 / 2, -15.75), (0, -12.5))
arc3 = ThreePointArc((21, 11.5), (21 / 2, 15.75), (0, 12.5))
arc4 = ThreePointArc((-21, -11.5), (-21 / 2, -15.75), (0, -12.5))
l1 = Line((-21, 11.5), (-21, -11.5))
l2 = Line((21, 11.5), (21, -11.5))
tube_cuttout = make_face([arc1, arc2, arc3, arc4, l1, l2])
front_panel -= [
    Pos(pos) * tube_cuttout
    for pos in [
        (0, 0),
        (44.1 + 10.05, 0),
        (-44.1 - 10.05, 0),
    ]
]
m3_hole = Circle(3.25 / 2)
m3_clock_width = 182.88
m3_clock_height = 42.901
front_panel -= [
    Pos(pos) * m3_hole
    for pos in [
        # Anchors
        (inner_width / 2 - 3 / 16 * IN, inner_height / 2 - 3 / 16 * IN),
        (-inner_width / 2 + 3 / 16 * IN, inner_height / 2 - 3 / 16 * IN),
        (-inner_width / 2 + 3 / 16 * IN, -inner_height / 2 + 3 / 16 * IN),
        (inner_width / 2 - 3 / 16 * IN, -inner_height / 2 + 3 / 16 * IN),
        # Clock
        (-m3_clock_width / 2, m3_clock_height / 2),
        (-m3_clock_width / 2, -m3_clock_height / 2),
        (m3_clock_width / 2, m3_clock_height / 2),
        (m3_clock_width / 2, -m3_clock_height / 2),
    ]
]
# Export DXF file for front panel -- uncomment below to export DXF
# exporter = ExportDXF(unit=Unit.MM, line_weight=0.5)
# exporter.add_layer("Layer 1")
# exporter.add_shape(front_panel, layer="Layer 1")
# exporter.write("front_panel.dxf")
front_panel = Pos(0, -length / 2 + 5, 0) * front_panel.rotate(Axis.X, 90)
front_panel = extrude(front_panel, 3, dir=(0, 1, 0))

# Rear panel
rear_panel = Rectangle(inner_width + -1 / 32 * IN, inner_height - 1 / 32 * IN)
rear_panel = fillet(rear_panel.vertices(), radius=inner_radius)
rear_panel -= [
    Pos(pos) * m3_hole
    for pos in [
        (inner_width / 2 - 3 / 16 * IN, inner_height / 2 - 3 / 16 * IN),
        (-inner_width / 2 + 3 / 16 * IN, inner_height / 2 - 3 / 16 * IN),
        (-inner_width / 2 + 3 / 16 * IN, -inner_height / 2 + 3 / 16 * IN),
        (inner_width / 2 - 3 / 16 * IN, -inner_height / 2 + 3 / 16 * IN),
    ]
]
rear_panel -= Pos(0, -inner_height / 2 + 10) * Rectangle(14, 6)
rear_panel = Pos(0, length / 2 - 6, 0) * rear_panel.rotate(Axis.X, 90)
rear_panel = extrude(rear_panel, 3, dir=(0, 1, 0))

# M3 heat set inserts
m3_insert_circle = Circle(2)
riser -= extrude((Pos(0, 0, (5 / 16 * IN) / 2) * m3_insert_circle), 6, dir=(0, 0, -1))
converter_risers = [Pos(pos) * riser for pos in converter_riser_pts]
converter_risers = [
    fillet(riser.edges().filter_by(Axis.Z), radius=2) for riser in converter_risers
]
m3_insert_circle = m3_insert_circle.rotate(Axis.X, 90)
rear_panel_insert_pts = [
    (inner_width / 2 - 3 / 16 * IN, length / 2 - 6, inner_height / 2 - 3 / 16 * IN),
    (-inner_width / 2 + 3 / 16 * IN, length / 2 - 6, inner_height / 2 - 3 / 16 * IN),
    (-inner_width / 2 + 3 / 16 * IN, length / 2 - 6, -inner_height / 2 + 3 / 16 * IN),
    (inner_width / 2 - 3 / 16 * IN, length / 2 - 6, -inner_height / 2 + 3 / 16 * IN),
]
enclosure -= [
    extrude(Pos(pos) * m3_insert_circle, 6, dir=(0, -1, 0))
    for pos in rear_panel_insert_pts
]
front_panel_insert_pts = [
    (inner_width / 2 - 3 / 16 * IN, -length / 2 + 5, inner_height / 2 - 3 / 16 * IN),
    (-inner_width / 2 + 3 / 16 * IN, -length / 2 + 5, inner_height / 2 - 3 / 16 * IN),
    (-inner_width / 2 + 3 / 16 * IN, -length / 2 + 5, -inner_height / 2 + 3 / 16 * IN),
    (inner_width / 2 - 3 / 16 * IN, -length / 2 + 5, -inner_height / 2 + 3 / 16 * IN),
]
enclosure -= [
    extrude(Pos(pos) * m3_insert_circle, 3, dir=(0, -1, 0))
    for pos in front_panel_insert_pts
]
m3_short = import_step("M3_Short.step").rotate(Axis.X, -90)  # 3 mm
m3_standard = import_step("M3_Standard.step")  # 5 mm
short_inserts = [
    (Pos(pos) * m3_short).translate((0, -3, 0)) for pos in front_panel_insert_pts
]
standard_inserts = [
    (Pos(pos) * m3_standard).translate((0, 0, -2)) for pos in converter_riser_pts
]
m3_standard = m3_standard.rotate(Axis.X, -90)
standard_inserts += [
    (Pos(pos) * m3_standard).translate((0, -5.5, 0)) for pos in rear_panel_insert_pts
]

# Power switch cuttout
kcd11_switch = Pos(
    inner_width / 2 + wall_thickness + 1.5, inner_length / 2 - 10, -12.5
) * import_step("KCD11.step").rotate(Axis.Z, 90).rotate(Axis.Y, 90)
enclosure -= Pos(
    inner_width / 2 + wall_thickness / 2, inner_length / 2 - 10, -12.5
) * Box(wall_thickness, 8.5, 14)

# Motion Sensor cuttouts
motion_pts = [
    (-inner_width / 2 - wall_thickness / 2, 0, 0),
    (inner_width / 2 + wall_thickness / 2, 0, 0),
]
motion_sensor = Cylinder(radius=5, height=wall_thickness).rotate(Axis.Y, 90)
motion_sensor = [Pos(pos) * motion_sensor for pos in motion_pts]
enclosure -= motion_sensor

# Speaker model
speaker = Pos(14, 31 / 2 + 5.5, inner_height / 2 - 1.5) * import_step(
    "Speaker.STEP"
).rotate(Axis.X, 90).rotate(Axis.Z, 180)
# Speaker cuttouts
enclosure -= Pos(0, 5.5, inner_height / 2 + wall_thickness / 2) * Box(
    28.5, 31.5, wall_thickness
)
enclosure -= [
    Pos(pos) * Cylinder(radius=1, height=3)
    for pos in [
        (28 / 2 + 4.6, 5.5, inner_height / 2 + 1.5),
        (-28 / 2 - 4.6, 5.5, inner_height / 2 + 1.5),
    ]
]
enclosure -= Pos(0, 31 / 2 + 5.5, inner_height / 2 + wall_thickness / 2) * Cylinder(
    radius=3, height=wall_thickness
)

# Add risers to enclosure
enclosure = enclosure + clock_risers + converter_risers

# Export STL for 3D prints
enclosure.export_stl("enclosure.stl")
front_panel.export_stl("front_panel.stl")
rear_panel.export_stl("rear_panel.stl")

show(
    enclosure,
    short_inserts,
    standard_inserts,
    board,
    converter,
    rear_panel,
    front_panel,
    speaker,
    kcd11_switch,
)
