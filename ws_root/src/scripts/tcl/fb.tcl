global mag

set mag 5

proc fb_normal {} {
    pix_size 1
    x_off 0
    y_off 0
    fb_output_bitmap
}

proc fb_magnify {} {
    global mag

    pix_size $mag
    set pos [fb_query_pointer]
    puts $pos
    x_off [lindex $pos 0]
    y_off [lindex $pos 1]
    fb_output_bitmap
}

proc fb_report {} {
	set pos [fb_query_pointer]
    set x [expr [x_off]+([lindex $pos 0]/[pix_size])]
    set y [expr [y_off]+([lindex $pos 1]/[pix_size])]
	
	puts [format "pointer at %d %d" $x $y]
}

proc fb_init {} {
}

