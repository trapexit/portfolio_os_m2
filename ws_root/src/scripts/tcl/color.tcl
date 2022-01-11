#!/usr/local/bin/wish -f
#
# Simple script to change colors of a window.

set red 0
set green 0
set blue 0

option add *Scale.sliderForeground "#cdb79e"
option add *Scale.activeForeground "#ffe4c4"
scale .red -command "color red" -label "Red Intensity"  -from 0 -to 255 \
    -orient horizontal -bg "#ffaeb9" -length 250
scale .green -command "color green" -label "Green Intensity" -from 0 -to 255 \
    -orient horizontal -bg "#43cd80"
scale .blue -command "color blue" -label "Blue Intensity" -from 0 -to 255 \
    -orient horizontal -bg "#7ec0ee"
pack .red .green .blue -side top -expand yes -fill both

proc color {which intensity} {
}

bind . <Control-q> {destroy .}
bind . <Control-c> {destroy .}
focus .

