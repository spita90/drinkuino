// =====================================================================
//  Drinkuino - pendant case for a hydration reminder (ESP32-C3)
//  Parametric OpenSCAD.
//
//  STL export:
//    openscad -o base.stl -D 'part="base"' drinkuino_case.scad
//    openscad -o lid.stl  -D 'part="lid"'  drinkuino_case.scad
//  For the 18650 variant add:  -D 'config="18650"'
//
//  Worn orientation: base against the chest, lid facing out,
//  lug on top (+Y), battery at the bottom (-Y).
//  Print: PETG, 0.2 mm layers, no supports. Print the lid with its
//  outer face on the bed (part="lid" already flips it).
//
//  (c) 2026 Andrea Spitaleri - CC BY 4.0
//  https://creativecommons.org/licenses/by/4.0/
// =====================================================================

/* [Configuration] */
// xiao_lipo = Seeed XIAO ESP32C3 + flat 503035 LiPo (recommended)
// 18650     = ESP32-C3 SuperMini + TP4056 USB-C module + 18650 cell
config = "xiao_lipo"; // [xiao_lipo, 18650]
// assembly = assembled preview, electronics = components only, print = base and lid laid out for printing,
// caps = the two button caps (print OK in green, Zz in yellow),
// inlay = black fill for the lid markings (multi-material printers, load together with the lid)
part = "assembly"; // [assembly, electronics, base, lid, print, caps, inlay]

/* [Preview] */
lid_lift    = 25;    // assembly: lid raised by N mm (0 = closed)
lid_alpha   = 0.35;  // lid transparency in the preview
show_labels = true;  // part labels
case_color  = "#2f5f8a";  // preview color of both base and lid
mark_color  = "Black";    // drop, OK and Zz on the lid

/* [Shell] */
wall     = 2.0;   // side walls
floor_t  = 1.6;   // floor (chest side)
lid_t    = 1.6;   // lid (front side)
r_out    = 4.0;   // outer corner radius
fit      = 0.15;  // lip/wall clearance: increase if the lid is too tight
lip_h    = 2.6;   // lid lip height
lip_t    = 1.2;   // lip thickness
snap_r   = 0.45;  // snap bump radius
snap_len = 8;

/* [Lug] */
lug_t    = 4.0;   // thickness; printed flat on the bed, so it is strong in-plane
lug_w    = 12;
lug_len  = 13;    // how far it sticks out of the wall
lug_hole = 4.5;   // for a 10-12 mm metal jump ring that holds the necklace clasp

/* [Components] */
div_t  = 1.2;     // rib between battery bay and electronics bay
div_h  = 3;
rail_h = 1.5;     // board standoff: room for wires/BAT pads under the XIAO
elec_w = 18.6;    // electronics bay width

buzz_d = 12.2;    // 12 mm passive buzzer
buzz_h = 9.5;

sw_s      = 6.2;  // 6x6 tactile switch body, 6x6x7 mm recommended
sw_hold_h = 2.4;  // switch pocket depth under the lid
stem_hole = 3.9;  // hole for a 3.5 mm round stem

/* [Button caps] */
// Printed caps glued on the switch stems: [0] = OK, [1] = Zz
btn_colors = ["LimeGreen", "Gold"];
cap_d      = 6.5;   // must stay wider than stem_hole: the cap rests above the lid
cap_top    = 1.2;
cap_sleeve = 1.3;   // depth of the socket for the stem; the stem sticks out ~1.9 mm, so the cap clears the lid
cap_fit    = 0.1;   // socket clearance on the 3.5 mm stem

usb_hole   = [9.6, 4.0];  // USB-C through-hole (width, height)
usb_recess = [13, 7.5];   // outer recess for the cable plug body
usb_web    = 0.8;         // wall left around the port

engrave   = true;
engrave_d = 0.6;

$fn = 48;

// ---------------------------------------------------------------------
//  Geometry derived from the configuration
// ---------------------------------------------------------------------
is18650 = (config == "18650");

// battery bay [L, W, H]
batt = is18650 ? [68, 19.2, 19.2]   // 18650 with welded tabs (65 mm + tabs)
               : [38, 31, 6.0];     // LiPo 503035 ~500 mAh with PCM

elec_h = is18650 ? 19.2 : 10.5;

in_L  = batt[0];
in_W  = batt[1] + div_t + elec_w;
in_H  = max(batt[2], elec_h) + 0.3;
e0    = batt[1] + div_t;            // electronics bay start (y)
ey    = e0 + elec_w / 2;            // electronics bay axis
out_L = in_L + 2 * wall;
out_W = in_W + 2 * wall;
H_base = floor_t + in_H;
r_in  = max(r_out - wall, 0.5);

// boards: [L, W, x_start, USB side (-1 = -X wall, +1 = +X wall), USB center z above PCB bottom]
// Measure your own boards with calipers: clones vary.
boards = is18650
  ? [ [22.8, 18.0, 0,           -1, 2.6],    // ESP32-C3 SuperMini
      [26.5, 17.6, in_L - 26.5, +1, 2.6] ]   // TP4056 USB-C with DW01 protection
  : [ [21.5, 18.0, 0,           -1, 2.6] ];  // Seeed XIAO ESP32C3

buzz_pos = is18650 ? [(22.8 + in_L - 26.5) / 2, ey] : [(21.5 + in_L) / 2, ey];

// [0] = "drank", [1] = snooze
btn_pos = is18650 ? [[11, ey], [in_L - 13, ey]]
                  : [[in_L * 0.28, batt[1] / 2], [in_L * 0.72, batt[1] / 2]];
btn_lbl = ["OK", "Zz"];

drop_pos = is18650 ? [in_L / 2, batt[1] / 2] : [21.5 / 2, ey];

// ---------------------------------------------------------------------
//  2D primitives
// ---------------------------------------------------------------------
module rrect(l, w, r) translate([r, r]) offset(r = r) square([l - 2 * r, w - 2 * r]);
module cav2d()   rrect(in_L, in_W, r_in);
module outer2d() translate([-wall, -wall]) rrect(out_L, out_W, r_out);
module drop2d(r) hull() { circle(r = r * 0.75); translate([0, r * 1.3]) circle(r = 0.2); }

// ---------------------------------------------------------------------
//  Base
// ---------------------------------------------------------------------
module lug() {
  linear_extrude(lug_t) hull() {
    translate([in_L / 2 - lug_w / 2 - 3, in_W]) square([lug_w + 6, wall]);
    translate([in_L / 2, in_W + wall + lug_len - lug_w / 2]) circle(d = lug_w);
  }
}

module usb_cut(b) {
  z = floor_t + rail_h + b[4];
  // through-hole
  translate([b[3] < 0 ? -wall - 1 : in_L - 0.1, ey - usb_hole[0] / 2, z - usb_hole[1] / 2])
    cube([wall + 1.1, usb_hole[0], usb_hole[1]]);
  // outer recess: leaves only usb_web of wall, so the plug goes all the way in
  translate([b[3] < 0 ? -wall - 1 : in_L + usb_web, ey - usb_recess[0] / 2, z - usb_recess[1] / 2])
    cube([wall - usb_web + 1, usb_recess[0], usb_recess[1]]);
}

module buzzer_holder() {
  ped = max(in_H - buzz_h - 0.4, 0);   // raises the buzzer up to the lid (sound holes)
  translate([buzz_pos[0], buzz_pos[1], 0]) difference() {
    cylinder(d = buzz_d + 2.4, h = ped + 3);
    translate([0, 0, ped]) cylinder(d = buzz_d + 0.3, h = 10);
    translate([0, 0, -1]) cylinder(d = buzz_d - 2, h = ped + 2);
    // slot towards the boards for pins and wires
    translate([-(buzz_d / 2 + 3), -2, -1]) cube([buzz_d / 2 + 3, 4, ped + 10]);
  }
}

module interior() {
  translate([0, batt[1], 0]) cube([in_L, div_t, div_h]);
  if (is18650)
    for (x = [8, in_L - 12])
      translate([x, 0, 0]) difference() {
        cube([4, batt[1], 8]);
        translate([-1, batt[1] / 2, batt[2] / 2]) rotate([0, 90, 0]) cylinder(d = 18.7, h = 6);
      }
  for (b = boards) {
    y0 = ey - b[1] / 2;
    for (y = [y0, y0 + b[1] - 1.5])
      translate([b[2], y, 0]) cube([b[0], 1.5, rail_h]);
  }
  buzzer_holder();
}

module base() {
  difference() {
    union() {
      linear_extrude(H_base) outer2d();
      lug();
    }
    translate([0, 0, floor_t]) linear_extrude(in_H + 1) cav2d();
    translate([in_L / 2, in_W + wall + lug_len - lug_w / 2, -1]) cylinder(d = lug_hole, h = lug_t + 2);
    for (b = boards) usb_cut(b);
    // grooves for the lid snap bumps
    for (y = [0, in_W])
      translate([in_L / 2, y, H_base - lip_h / 2]) rotate([0, 90, 0])
        cylinder(r = snap_r + 0.1, h = snap_len + 2, center = true);
    // notch to pry the lid open with a fingernail or coin
    translate([in_L * 0.25 - 4, -wall - 1, H_base - 1.2]) cube([8, wall + 1.01, 2]);
  }
  translate([0, 0, floor_t - 0.01]) interior();
}

// ---------------------------------------------------------------------
//  Lid (assembly coordinates: z=0 = top edge of the base)
// ---------------------------------------------------------------------
module lid() {
  difference() {
    union() {
      linear_extrude(lid_t) outer2d();
      translate([0, 0, -lip_h]) linear_extrude(lip_h + 0.01) difference() {
        offset(delta = -fit) cav2d();
        offset(delta = -fit - lip_t) cav2d();
      }
      for (y = [fit, in_W - fit])
        translate([in_L / 2, y, -lip_h / 2]) rotate([0, 90, 0])
          cylinder(r = snap_r, h = snap_len, center = true);
      // switch pockets: the 6x6 body sits here (a drop of glue), the stem goes through the hole
      for (p = btn_pos) translate([p[0], p[1], -sw_hold_h]) difference() {
        translate([-sw_s / 2 - 1, -sw_s / 2 - 1, 0]) cube([sw_s + 2, sw_s + 2, sw_hold_h + 0.01]);
        translate([-sw_s / 2, -sw_s / 2, -0.1]) cube([sw_s, sw_s, sw_hold_h + 0.2]);
      }
    }
    for (p = btn_pos) translate([p[0], p[1], -1]) cylinder(d = stem_hole, h = lid_t + 2);
    // sound holes above the buzzer
    translate([buzz_pos[0], buzz_pos[1], -1]) {
      cylinder(d = 1.8, h = lid_t + 2);
      for (a = [0 : 60 : 359]) rotate(a) translate([3.2, 0, 0]) cylinder(d = 1.6, h = lid_t + 2);
    }
    if (engrave) translate([0, 0, lid_t - engrave_d]) linear_extrude(engrave_d + 1) marks2d();
  }
}

// Markings engraved in the lid: button labels and the drop
module marks2d() {
  for (i = [0 : len(btn_pos) - 1])
    translate([btn_pos[i][0], btn_pos[i][1] - 7.5])
      text(btn_lbl[i], size = 3.5, halign = "center", valign = "center",
           font = "Liberation Sans:style=Bold");
  translate(drop_pos) drop2d(5);
}

// Fills the engravings flush with the lid surface: black in the preview, and a
// separate body for multi-material printing (same coordinates as part="lid")
module inlay() if (engrave) translate([0, 0, lid_t - engrave_d]) linear_extrude(engrave_d) marks2d();

module lid_print() translate([0, 0, lid_t]) rotate([180, 0, 0]) lid();
module inlay_print() translate([0, 0, lid_t]) rotate([180, 0, 0]) inlay();

// ---------------------------------------------------------------------
//  BOM components (preview only, not printed)
//  Inner coordinates: z = 0 on the inner floor of the base.
//  Approximate dimensions: check them on the real parts.
// ---------------------------------------------------------------------
fpc   = [30, 8];   // XIAO FPC antenna (approximate size), glued under the lid
z_top = 5.2;       // level of the free-wired parts: on top of the LiPo (5 mm) + Kapton

R1M   = ["SaddleBrown", "Black", "Green",  "Gold"];
R100k = ["SaddleBrown", "Black", "Yellow", "Gold"];
R1k   = ["SaddleBrown", "Black", "Red",    "Gold"];

module lbl(t, sz = 1.5)
  if (show_labels) color("Black") linear_extrude(0.1)
    text(t, size = sz, halign = "center", valign = "center", font = "Liberation Sans:style=Bold");

module wire(pts, d = 0.8, c = "Red")
  color(c) for (i = [0 : len(pts) - 2])
    hull() { translate(pts[i]) sphere(d = d, $fn = 8); translate(pts[i + 1]) sphere(d = d, $fn = 8); }

// 1/4 W resistor lying along X, centered
module resistor(bands, l = 6.3, d = 2.4, leg = 1.8) translate([0, 0, d / 2]) {
  color("Tan") rotate([0, 90, 0]) cylinder(d = d, h = l, center = true, $fn = 16);
  for (i = [0 : len(bands) - 1])
    color(bands[i]) translate([-l / 2 + [0.9, 1.9, 2.9, 5.0][i], 0, 0]) rotate([0, 90, 0])
      cylinder(d = d + 0.05, h = 0.6, $fn = 16);
  color("Silver") rotate([0, 90, 0]) cylinder(d = 0.5, h = l + 2 * leg, center = true, $fn = 8);
}

// 1N4148 lying along X, centered (black band = cathode)
module diode(l = 3.8, d = 1.8, leg = 1.8) translate([0, 0, d / 2]) {
  color("OrangeRed", 0.85) rotate([0, 90, 0]) cylinder(d = d, h = l, center = true, $fn = 16);
  color("Black") translate([l / 2 - 0.9, 0, 0]) rotate([0, 90, 0]) cylinder(d = d + 0.05, h = 0.5, $fn = 16);
  color("Silver") rotate([0, 90, 0]) cylinder(d = 0.5, h = l + 2 * leg, center = true, $fn = 8);
}

// TO-92 NPN lying on its flat side, leads towards +X
module to92(leg = 4) {
  color("#222222") intersection() {
    translate([0, 0, 1.2]) rotate([0, 90, 0]) cylinder(d = 4.8, h = 4.5, $fn = 24);
    translate([-0.1, -3, 0]) cube([4.7, 6, 4]);
  }
  color("Silver") for (y = [-1.27, 0, 1.27])
    translate([4.5, y, 0.8]) rotate([0, 90, 0]) cylinder(d = 0.45, h = leg, $fn = 8);
}

// ceramic disc capacitor, lying flat
module cap_disc(d = 4, t = 2) {
  color("Orange") translate([0, 0, t / 2]) scale([1, 1, t / d]) sphere(d = d, $fn = 20);
  color("Silver") for (x = [-1.25, 1.25])
    translate([x, -d / 2 - 1, 0.4]) rotate([-90, 0, 0]) cylinder(d = 0.45, h = 1.2, $fn = 8);
}

// low-profile 10 uF electrolytic, lying along X
module cap_elec(d = 4, l = 5, leg = 1.5) translate([0, 0, d / 2]) {
  color("RoyalBlue") rotate([0, 90, 0]) cylinder(d = d, h = l, center = true, $fn = 20);
  color("LightGray") translate([0, 0, d / 2 - 0.3]) cube([l, 0.9, 0.62], center = true);
  color("Silver") for (y = [-0.75, 0.75])
    translate([l / 2, y, 0]) rotate([0, 90, 0]) cylinder(d = 0.45, h = leg, $fn = 8);
}

// 6x6x7 THT tactile switch, leads bent flat
module tact(h_tot = 7, stem = "#4a3020") {
  color("#202020") translate([-3, -3, 0]) cube([6, 6, 3.3]);
  color("Silver") translate([-3, -3, 3.3]) cube([6, 6, 0.2]);
  color(stem) cylinder(d = 3.5, h = h_tot, $fn = 20);
  color("Silver") for (x = [-2.25, 2.25], s = [-1, 1]) {
    translate([x - 0.35, s * 3 - 0.15, -1]) cube([0.7, 0.3, 1]);
    translate([x - 0.35, s > 0 ? 2.85 : -5, -1]) cube([0.7, 2.15, 0.3]);
  }
}

module buzzer() {
  color("#151515") difference() {
    cylinder(d = buzz_d - 0.2, h = buzz_h, $fn = 40);
    translate([0, 0, buzz_h - 1.5]) cylinder(d = 2, h = 2, $fn = 16);
  }
  color("Silver") for (x = [-3.25, 3.25]) translate([x, 0, -1]) cylinder(d = 0.6, h = 1, $fn = 8);
  translate([0, 4, buzz_h]) lbl("+", 2);
}

// 1027 coin vibration motor, wires towards +X
module motor() {
  color("Silver") cylinder(d = 10, h = 2.7, $fn = 40);
  translate([0, 0, 2.7]) lbl("M", 2.5);
  wire([[4.5,  1, 1.3], [6.3,  2, 1.6]], 0.5, "Red");
  wire([[4.5, -1, 1.3], [6.3, -2, 1.6]], 0.5, "Blue");
}

// LiPo 503035 with PCM at one end and Kapton on top (origin: pouch corner)
module lipo() {
  color("Silver") hull() for (x = [1, 31], y = [1, 29]) translate([x, y, 0]) cylinder(r = 1, h = 5, $fn = 16);
  color("Gold", 0.85) translate([32, 0.5, 0.5]) cube([3, 29, 4]);
  color("Orange", 0.35) translate([0.3, 0.3, 5]) cube([31.5, 29.4, 0.15]);
}

// Seeed XIAO ESP32C3 (origin: PCB corner, PCB bottom)
module xiao() {
  color("#1c1c1c") cube([21, 17.8, 1.0]);
  color("Silver") translate([-0.6, 17.8 / 2 - 4.47, 1.0]) cube([7.35, 8.94, 3.26]);      // USB-C
  color("#5a5a5a") translate([10.5, 17.8 / 2 - 2.5, 1.0]) cube([5, 5, 0.9]);            // ESP32-C3
  color("Silver") translate([15.8, 17.8 / 2 - 4.5, 1.0]) cube([2, 1.6, 0.6]);           // crystal
  color("Gold") translate([18, 13.2, 1.0]) cube([2.6, 2.6, 1.25]);                       // U.FL
  for (y = [1.3, 14.5]) {                                                                // BOOT / RESET
    color("Silver") translate([1.5, y, 1.0]) cube([3, 2, 0.6]);
    color("Black") translate([2.5, y + 0.5, 1.6]) cube([1, 1, 0.5]);
  }
  color("Orange") translate([8, 14.5, 1.0]) cube([1, 0.6, 0.4]);                          // charge LED
  color("SaddleBrown") for (p = [[9, 4], [9, 12], [17, 3], [12.5, 2], [12.5, 14.5], [16.5, 12]])
    translate([p[0], p[1], 1.0]) cube([1, 0.5, 0.5]);
  color("Gold") for (i = [0 : 6], y = [0, 17.8 - 1.0])                                   // castellated pads
    translate([10.5 + (i - 3) * 2.54 - 0.7, y, -0.02]) cube([1.4, 1.0, 1.06]);
  color("Gold") for (y = [6, 10.3]) translate([13, y, -0.05]) cube([2.2, 1.5, 0.05]);    // BAT pads (bottom)
  translate([14, 16.2, 1.01]) lbl("XIAO ESP32C3", 1.1);
}

module fpc_antenna() color("#b8860b", 0.5) cube([fpc[0], fpc[1], 0.2]);

// xiao_lipo variant: every BOM component in place.
// lift = lid lift (switches and antenna are attached to the lid).
module electronics(lift = 0) {
  translate([1, 0.5, 0]) lipo();
  translate([0.25, ey - 8.9, rail_h]) xiao();
  translate([buzz_pos[0], buzz_pos[1], max(in_H - buzz_h - 0.4, 0)]) buzzer();
  translate([31, 6, z_top]) motor();

  // above the battery, rib side: buzzer (Q1) and motor (Q2) drivers
  for (r = [[23.2, "Q1"], [28.2, "Q2"]]) {
    translate([3, r[0], z_top])    { to92();          translate([2.25, 0, 3.65]) lbl(r[1]); }
    translate([17, r[0], z_top])   { resistor(R1k);   translate([0, 0, 2.45]) lbl("1k"); }
    translate([28.5, r[0], z_top]) { resistor(R100k); translate([0, 0, 2.45]) lbl("100k"); }
  }
  // above the battery, bottom side: battery divider (1M) and button pull-ups (100k)
  for (x = [7.5, 19]) {
    translate([x, 4, z_top])   { resistor(R1M);   translate([0, 0, 2.45]) lbl("1M"); }
    translate([x, 8.6, z_top]) { resistor(R100k); translate([0, 0, 2.45]) lbl("100k"); }
  }
  translate([25.2, 6, z_top]) rotate(90) { diode(); translate([0, 0, 1.85]) lbl("D2", 1); }   // motor
  translate([17.5, 36.5, 4.2])           { diode(); translate([0, 0, 1.85]) lbl("D1", 1); }   // buzzer
  // between the switches: 100 nF ADC and 3V3 filters, 10 uF buffer for motor spikes
  for (x = [17, 21.2]) translate([x, 13, z_top]) { cap_disc(); translate([0, 0, 2.05]) lbl("100n", 1); }
  translate([19, 17.5, z_top]) { cap_elec(); translate([0, 0, 4.05]) lbl("10u", 1.2); }

  // battery wires: from the PCM, over the rib, under the XIAO to the BAT pads
  wire([[36.2, 28.5, 2.5], [37, 29, 3], [37, 31.6, 4.2], [37, 32.7, 3.6], [36, 33.0, 1.2],
        [22.5, 33.0, 0.8], [20, 38, 0.7], [14.3, 39.3, 1.2]], 0.8, "Red");
  wire([[36.2, 26.5, 2.5], [37.2, 27, 3], [37.4, 31.6, 4.6], [37.4, 33.3, 3.6], [36.4, 33.7, 1.2],
        [22.5, 33.7, 0.8], [20, 42, 0.7], [14.3, 43.6, 1.2]], 0.8, "Black");

  // attached to the lid
  translate([0, 0, lift]) {
    for (i = [0 : len(btn_pos) - 1]) translate([btn_pos[i][0], btn_pos[i][1], in_H - 3.5]) tact(7, btn_colors[i]);
    translate([4, 21.5, in_H - 0.2]) fpc_antenna();
  }
  // antenna coax -> U.FL, along the +X wall and over the XIAO edge
  wire([[33, 28.5, in_H - 0.5 + lift], [36.3, 31.5, 8], [37, 36, 6], [37, 49, 5],
        [24, 49.2, 5], [20.5, 47.8, 4.2], [19.5, 47.1, 3.8]], 1.13, "#303030");
}

// 18650 variant: bounding volumes only
module ghosts_18650() {
  color("SteelBlue", 0.5) translate([1.5, batt[1] / 2, batt[2] / 2]) rotate([0, 90, 0]) cylinder(d = 18.4, h = 65);
  for (b = boards)
    color("DarkGreen", 0.6) translate([b[2], ey - b[1] / 2, rail_h]) cube([b[0], b[1], 1.2]);
  color("Black", 0.6) translate([buzz_pos[0], buzz_pos[1], max(in_H - buzz_h - 0.4, 0)])
    cylinder(d = buzz_d, h = buzz_h);
}

module components(lift = 0) if (is18650) ghosts_18650(); else electronics(lift);

// ---------------------------------------------------------------------
//  Button caps
// ---------------------------------------------------------------------
module btn_cap() difference() {
  union() {
    cylinder(d = cap_d, h = cap_sleeve);
    translate([0, 0, cap_sleeve]) cylinder(d1 = cap_d, d2 = cap_d - 1, h = cap_top);
  }
  translate([0, 0, -0.01]) cylinder(d = 3.5 + cap_fit, h = cap_sleeve);
}

// top face down: the socket faces up, no supports needed
module caps_print()
  for (i = [0, 1]) color(btn_colors[i]) translate([i * (cap_d + 4), 0, cap_sleeve + cap_top]) rotate([180, 0, 0]) btn_cap();

// on the stems: a 7 mm switch whose body touches the lid has its stem top at H_base + 3.5
module caps_assembled(lift = 0)
  for (i = [0 : len(btn_pos) - 1])
    color(btn_colors[i]) translate([btn_pos[i][0], btn_pos[i][1], H_base + 3.5 - cap_sleeve + lift]) btn_cap();

// ---------------------------------------------------------------------
if (part == "base") base();
else if (part == "lid") lid_print();
else if (part == "print") { base(); translate([0, -12, 0]) lid_print(); }
else if (part == "caps") caps_print();
else if (part == "inlay") inlay_print();
else if (part == "electronics") { translate([0, 0, floor_t]) components(0); caps_assembled(0); }
else {
  color(case_color) base();
  translate([0, 0, floor_t]) components(lid_lift);
  caps_assembled(lid_lift);
  translate([0, 0, H_base + 0.01 + lid_lift]) {
    color(case_color, lid_alpha) lid();
    color(mark_color) inlay();
  }
}

echo(str("Outer size: ", out_L, " x ", out_W, " x ", H_base + lid_t,
         " mm (+ lug ", lug_len, " mm)"));
