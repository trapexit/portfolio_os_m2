proc record_title { fn } {
    display_image $fn
	set frame [get_current_frame]
	for { set i 0 } { $i < 199 } { incr i 1 } {
		vdr-v1000 ${frame}WR 100
        puts [format "writing %s to frame %d" $fn $frame]
        incr frame
    }
}

proc record_pic { fn } {
    global frame
    catch { exec $GROOT/programs/unix/conv/pictorgb $fn /tmp/junk.rgb }
    display_image  /tmp/junk.rgb
    vdr-v1000 ${frame}WR 100
    puts [format "writing %s to frame %d" $fn $frame]
    incr frame
}

proc rgb_to_disk { {pat *.rgb } } {
	set frame [get_current_frame]
	puts [format "testing frame %d" $frame]
    foreach i [lsort [glob $pat]] {
        display_image $i
		vdr-v1000 ${frame}WR 100
    	puts [format "writing %s to frame %d" $i $frame]
    	incr frame
    }
}

proc recordit {} {
    global the_object

    set frame 6000
    for {set i .01} { $i < 2 } { set i [expr $i+.01]} {
        set ang [expr $i*360]
        Char_Rotate $the_object TRANS_YAxis $ang
        display; update idletasks
        puts [format "frame %d" $frame]
        vdr-v1000 ${frame}WR 100
        incr frame
    }
}

proc vdr_v1000_record { {num_frames 1}} {
	set f [get_current_frame]
	if { [scan $num_frames "%d" fn] < 1 } {
		AlertBox [format "invalid frame count %s" $num_frames]
		return 
	}
	set f [expr $f+$num_frames]
	set n [string first . $f]
	if { $n != -1 } { set f [string range $f 0 [expr $n-1]] }
	puts [format "record frames %d, next %d" $num_frames $f]
	if { [catch {vdr-v1000 ${f}WR 10} ret] != 0 } {
        vdr_v1000_report_error [format "vdr-v1000-command: error %s" $ret]
        return
    }
}

proc get_current_mode {} {
	global current_mode

	set m [vdr-v1000 ?P]
	
	switch $m {
	{P00} { set current_mode "Open" }
	{P01} { set current_mode "Park" }
	{P02} { set current_mode "Set Up" }
	{P03} { set current_mode "Reject" }
	{P04} { set current_mode "Play" }
	{P05} { set current_mode "Still" }
	{P06} { set current_mode "Pause" }
	{P07} { set current_mode "Search" }
	{P08} { set current_mode "Scan" }
	{P09} { set current_mode "Multi Speed" }
	}
}

proc vdr_v1000_report_error { error_code } {
global status_message

switch $error_code {
	{E00} {
      set err_message "Communication error"
    }
	{E04} {
      set err_message "Feature not available"
    }
	{E06} {
      set err_message "Missing Argument"
    }
	{E11} {
      set err_message "Disk not loaded"
    }
	{E12} {
      set err_message "Search Error"
    }
	{E13} {
      set err_message "Defocussing Error"
    }
	{E16} {
      set err_message "Interrupt by other device"
    }
	{E40} {
      set err_message "Focus alarm: head A"
    }
	{E41} {
      set err_message "Tracking error: head A"
    }
	{E42} {
      set err_message "Focus alarm: head B"
    }
	{E43} {
      set err_message "Tracking Alarm: head B"
    }
	{E44} {
      set err_message "Disk or hard error: head A (0.3 seconds)"
    }
	{E45} {
      set err_message "Disk or hard error: head B (0.3 seconds)"
    }
	{E46} {
      set err_message "Disk or hard error: head A (0.8 seconds)"
    }
	{E47} {
      set err_message "Disk or hard error: head B (0.8 seconds)"
    }
	{E48} {
      set err_message "Dew detected (head A condensation)"
    }
	{E49} {
      set err_message "Dew detected (head B condensation)"
    }
	{E4A} {
      set err_message "Disk (or hard) error head A focus time elapsed (4 secs)"
    }
	{E4B} {
      set err_message "Disk (or hard) error head B focus time elapsed (4 secs)"
    }
	{E4C} {
      set err_message "Disk (or hard) error head A focus time elapsed (0.1 sec)"
    }
	{E4D} {
      set err_message "Disk (or hard) error head B focus time elapsed (0.1 sec)"
    }
	{E4E} {
      set err_message "Hard Error (head A write gate error)"
    }
	{E4F} {
      set err_message "Hard Error (head B write gate error)"
    }
	{E50} {
      set err_message "Disk (or hard) error, spindle time elapsed (1 minute)"
    }
	{E95} {
      set err_message "Loading not completed"
    }
	{E97} {
      set err_message "Disk (or hard) error, address read time elapsed (4 secs)"
    }
	{E99} {
      set err_message "Panic"
    }
	{default} {
      set err_message $error_code
    }
  }

  set status_message [format "vdr-v1000 error: %s, %s" $error_code $err_message]
  AlertBox $status_message
}

proc vdr-v1000-command { cmd } {
	global status_message
	global video_panel

	set status_message [format "Running Command: %s" $cmd]
	$video_panel config -cursor {clock red white}
	update idletasks

	if { [catch {vdr-v1000 $cmd 60} ret] != 0 } {
		vdr_v1000_report_error [format "vdr-v1000-command: error %s" $ret]
		return
	}

	if [string match E* $ret] {
		vdr_v1000_report_error $ret
	} else { set status_message "" }

	$video_panel config -cursor {arrow black white}

	get_current_mode

	return $ret
}

proc set_display {} {
	global display_frame_number display_time
	set ra 0
	if { $display_frame_number == 1 } { set ra [expr $ra+1] }
	if { $display_time == 1 } { set ra [expr $ra+2] }

	if { $ra != 0 } { vdr-v1000-command 1DS } else {vdr-v1000-command 0DS }
	vdr-v1000-command ${ra}RA
}

proc get_current_frame {} {
	set s [vdr-v1000-command ?F]
	scan $s %d i
	return $i
}

proc scan_to_frame { frame } {
	global current_frame
	
	if { [scan $frame "%d" fn] < 1 } return 
	
	if { $frame == $current_frame } {
		return;
	} elseif { $frame < $current_frame } {
		set cmd NR
	} else {
		set cmd NF
	}

	vdr-v1000-command ${frame}$cmd
}

proc search_to_frame { frame } {
	global current_frame
	
	if { [scan $frame "%d" fn] < 1 } return 
	
	vdr-v1000-command ${frame}SE
}

proc set_speed_dialog {} {
	global speed_panel current_speed tmp_speed change_speed

	set tmp_speed $current_speed

	set speed_panel [toplevel .speed_panel]
	bind $speed_panel <Destroy> {set change_speed 0}
	wm geometry $speed_panel 320x150+700+500
	wm title $speed_panel "Select Multi Speed Playback Rate"
	set speed_frame0 [frame $speed_panel.speed_frame0 -borderwidth {2} -relief {raised}]
	set speed_frame1 [frame $speed_frame0.speed_frame1 -borderwidth {2} -relief {raised}]
	set speed_frame2 [frame $speed_frame0.speed_frame2 -borderwidth {2} -relief {raised}]
	set speed_frame3 [frame $speed_frame0.speed_frame3 -borderwidth {2} -relief {raised}]
	set button_frame [frame $speed_panel.button_frame -borderwidth {2} -relief {raised}]
	pack $speed_frame0 -side top  -fill x
	pack $speed_frame1 -side left -fill x 
	pack $speed_frame2 -side left -fill x
	pack $speed_frame3 -side left -fill x 
	pack $button_frame -side bottom -fill x
	radiobutton $speed_frame1.b1 -text "x3" -variable tmp_speed -value 180
	radiobutton $speed_frame1.b4 -text "1/2" -variable tmp_speed -value 30
	radiobutton $speed_frame1.b7 -text "1/16" -variable tmp_speed -value 3
	radiobutton $speed_frame2.b2 -text "x2" -variable tmp_speed -value 120
	radiobutton $speed_frame2.b5 -text "1/4" -variable tmp_speed -value 15
	radiobutton $speed_frame2.b8 -text "STEP 1" -variable tmp_speed -value 2
	radiobutton $speed_frame3.b3 -text "x1" -variable tmp_speed -value 60
	radiobutton $speed_frame3.b6 -text "1/8" -variable tmp_speed -value 7
	radiobutton $speed_frame3.b9 -text "STEP 2" -variable tmp_speed -value 1

	pack append $speed_frame1 $speed_frame1.b1 {fill expand top} \
		$speed_frame1.b4 {fill expand top} $speed_frame1.b7 {fill expand top}

	pack append $speed_frame2 $speed_frame2.b2 {fill expand top} \
		$speed_frame2.b5 {fill expand top} $speed_frame2.b8 {fill expand top}

	pack append $speed_frame3 $speed_frame3.b3 {fill expand top} \
		$speed_frame3.b6 {fill expand top} $speed_frame3.b9 {fill expand top}

	set ok_button [button $button_frame.ok -text "OK" -relief sunken -command {set change_speed 1 } ]
	set apply_button [button $button_frame.apply -text "Apply" -command {global speed_panel tmp_speed; vdr-v1000-command ${tmp_speed}SP}]
    set cancel_button [button $button_frame.cancel -text "CANCEL" -command "set change_speed 0 "]
    pack append $button_frame $apply_button {fill expand left} $ok_button {fill expand left} $cancel_button {fill expand left}

	bind $speed_panel <Any-Key-Return> "$ok_button flash; set change_speed 1"

	set oldFocus [focus]
	grab $speed_panel
	focus $speed_panel

	tkwait variable change_speed
	if { $change_speed == 1 } { set current_speed $tmp_speed }
	vdr-v1000-command ${current_speed}SP
	catch { destroy $speed_panel}
	set $speed_panel ""
	focus $oldFocus
}

proc scan_to_dialog {} {
	global new_frame current_frame do_scan
	global scan_to_panel
	global do_update

	set scan_to_panel [toplevel .scan_to_panel]
	bind $scan_to_panel <Destroy> {set do_scan 0}
	wm geometry $scan_to_panel +700+500
	wm title $scan_to_panel "Scan To"
	set scan_to_frame [frame $scan_to_panel.scan_to_frame -borderwidth {2} -relief {raised}]
	set button_frame [frame $scan_to_panel.button_frame -borderwidth {2} -relief {raised}]
	pack $scan_to_frame -side top -fill x
	pack $button_frame -side bottom -fill x

	set current_frame [get_current_frame]
	set new_frame $current_frame
	set fl [label $scan_to_frame.fl -text "Frame:"]
	set fe [entry $scan_to_frame.fe -textvariable new_frame]
	grab set $scan_to_panel

	pack append $scan_to_frame $fl {left} $fe {fill expand left}

	set ok_button [button $button_frame.ok -text "OK" -relief sunken -command "set do_scan 1"]
	set cancel_button [button $button_frame.cancel -text "CANCEL" -command "set do_scan 0"] 
	pack append $button_frame $ok_button {fill expand left} $cancel_button {fill expand left}

	bind $fe <Any-Key-Return> "$ok_button flash; set do_scan 1"
	set oldFocus [focus]
    grab $scan_to_panel
	focus $fe

    tkwait variable do_scan
	set do_update $do_scan
    catch {destroy $scan_to_panel}
    set $scan_to_panel ""
    focus $oldFocus
	update idletasks
    if { $do_update == 1 } { 
		if { [scan $new_frame "%d" fn] >= 1 } {
			scan_to_frame $new_frame;
		}
	}
}

proc search_to_dialog {} {
	global new_frame current_frame do_search
	global search_to_panel

	set search_to_panel [toplevel .search_to_panel]
	bind $search_to_panel <Destroy> {set do_search 0}
	wm geometry $search_to_panel +700+500
	wm title $search_to_panel "Go To"
	set search_to_frame [frame $search_to_panel.search_to_frame -borderwidth {2} -relief {raised}]
	set button_frame [frame $search_to_panel.button_frame -borderwidth {2} -relief {raised}]
	pack $search_to_frame -side top -fill x
	pack $button_frame -side bottom -fill x

	set current_frame [get_current_frame]
	set new_frame $current_frame
	set fl [label $search_to_frame.fl -text "Frame:"]
	set fe [entry $search_to_frame.fe -textvariable new_frame]

	pack append $search_to_frame $fl {left} $fe {fill expand left}

	set ok_button [button $button_frame.ok -text "OK" -relief sunken -command "set do_search 1"]
	set cancel_button [button $button_frame.cancel -text "CANCEL" -command "set do_search 0"] 
	pack append $button_frame $ok_button {fill expand left} $cancel_button {fill expand left}

	set oldFocus [focus]
    grab $search_to_panel
	bind $fe <Any-Key-Return> "$ok_button flash; set do_search 1"
	focus $fe

    tkwait variable do_search

    if { $do_search == 1 } { 
		if { [scan $new_frame "%d" fn] >= 1 } {
    		search_to_frame $new_frame;
		}
	}

    catch { destroy $search_to_panel }
    set $search_to_panel ""
    focus $oldFocus
}

# check if vdr-v1000 online

proc probe_vdr_v1000 {} {

	if { [catch {vdr-v1000 ?X 2} ret] != 0 } {
        AlertBox [format "vdr-v1000-command: error %s" $ret]
		return 0
    }

	if { [string first P15 $ret] == -1 } {
        AlertBox [format "vdr-v1000-command: error unknown device, %s" $ret]
		return 0
	}
}

# create video panel

proc create_video_panel {} {
	global video_panel scan_to_panel search_to_panel speed_panel
	global current_mode status_message
	global display_frame_number display_time
	global current_frame new_frame current_speed tmp_speed
	global bitmap_dir

	# check if panel allready exists

	set bitmap_dir /var/3do/upgrade/data/bitmaps

	set l 0

	catch {set l [clength $video_panel]}

	if { $l != 0 } return

	if { [probe_vdr_v1000] == 0 } {
		return;
	}

	set scan_to_panel ""
	set search_to_panel ""
	set speed_panel ""
	
	set video_panel [toplevel .video_panel]
	wm geometry $video_panel 400x200+700+500
	wm title $video_panel "VDR-V1000 Controls"
	$video_panel config -cursor {arrow black white}
	
	bind $video_panel <Enter> {focus $video_panel}
	bind $video_panel <Escape> {exit}
	bind $video_panel <Any-Key-f> {vdr-v1000-command SF}
	bind $video_panel <Any-Key-F> {vdr-v1000-command NF}
	bind $video_panel <Any-Key-b> {vdr-v1000-command SR}
	bind $video_panel <Any-Key-B> {vdr-v1000-command NR}
	bind $video_panel <Any-Key-s> {scan_to_dialog}
	bind $video_panel <Any-Key-S> {scan_to_dialog}
	bind $video_panel <Any-Key-g> {search_to_dialog}
	bind $video_panel <Any-Key-G> {search_to_dialog}
	bind $video_panel <Destroy> {set video_panel ""}
	
	set vm_frame [frame $video_panel.vm_frame -borderwidth {2} -relief {raised}]
	set vc1_frame [frame $video_panel.vc1_frame -borderwidth {2} -relief {raised}]
	set vc2_frame [frame $video_panel.vc2_frame -borderwidth {2} -relief {raised}]
	set status_frame1 [frame $video_panel.status_frame1 -borderwidth {2} -relief {raised}]
	set status_frame2 [frame $video_panel.status_frame2 -borderwidth {2} -relief {raised}]
	
	pack $vm_frame -side top -fill x
	pack $vc1_frame -side top -fill x
	pack $vc2_frame -side top -fill x
	pack $status_frame1 -side bottom -fill x
	pack $status_frame2 -side bottom -fill x
	
	
	# create disk menu
	
	menubutton $vm_frame.disk -text "Disk" -menu $vm_frame.disk.menu -anchor nw
	pack $vm_frame.disk -side left
	
	menu $vm_frame.disk.menu
	$vm_frame.disk.menu add command -label "Eject Disk" -command {vdr-v1000-command OP}
	$vm_frame.disk.menu add command -label "Park Disk" -command {vdr-v1000-command RJ}
	$vm_frame.disk.menu add command -label "Start Disk" -command {vdr-v1000-command SA}
	$vm_frame.disk.menu add command -label "Play Disk" -command {vdr-v1000-command PL}
	$vm_frame.disk.menu add command -label "Pause Disk" -command {vdr-v1000-command PA}
	$vm_frame.disk.menu add command -label "Still Disk" -command {vdr-v1000-command ST}
	
	# create display menu
	
	menubutton $vm_frame.display -text "Display" -menu $vm_frame.display.menu -anchor nw
	pack $vm_frame.display -side left
	
	menu $vm_frame.display.menu
	$vm_frame.display.menu add checkbutton -label "Frame Number" -command { set_display} -variable display_frame_number
	$vm_frame.display.menu add checkbutton -label "Time" -command {set_display} -variable display_time
	
	# create search menu
	
	menubutton $vm_frame.search -text "Search" -menu $vm_frame.search.menu -anchor nw
	pack $vm_frame.search -side left
	menu $vm_frame.search.menu
	$vm_frame.search.menu add command -label "Scan Forward" -command {vdr-v1000-command NF} -accelerator f
	$vm_frame.search.menu add command -label "Scan Backward" -command {vdr-v1000-command NR} -accelerator b
	$vm_frame.search.menu add command -label "Scan To Frame..." -command {scan_to_dialog} -accelerator s
	$vm_frame.search.menu add command -label "Go to frame..." -command {search_to_dialog} -accelerator g
	$vm_frame.search.menu add command -label "Set Speed..." -command {set_speed_dialog}
	
	# create control pannel 1
	
	set fwd_button [button $vc1_frame.fwd_button -bitmap @$bitmap_dir/fwd.xbm -command { vdr-v1000-command PL}] 
	set ffwd_button [button $vc1_frame.ffwd_button -bitmap @$bitmap_dir/ffwd.xbm -command {vdr-v1000-command PL; vdr-v1000-command MF}]
	set fbwd_button [button $vc1_frame.fbwd_button -bitmap @$bitmap_dir/fbwd.xbm -command {vdr-v1000-command PL; vdr-v1000-command MR}]
	set stop_button [button $vc1_frame.stop_button -bitmap @$bitmap_dir/stop.xbm -command {vdr-v1000-command RJ}]
	set pause_button [button $vc1_frame.pause_button -bitmap @$bitmap_dir/pause.xbm -command {vdr-v1000-command ST}]
	
	pack append $vc1_frame $fbwd_button {fill expand left} $stop_button {fill expand left} $pause_button {fill expand left} $fwd_button {fill expand left} $ffwd_button {fill expand left}
	
	# create control pannel 2
	
	set step_label [label $vc2_frame.step -text "Step:"]
	set fwd_frame_button [button $vc2_frame.fwd_frame_button -text "Fwd" -command { vdr-v1000-command SF}]
	set bwd_frame_button [button $vc2_frame.bwd_frame_button -text "Bwd" -command { vdr-v1000-command SR}]
	
	pack append $vc2_frame $step_label {fill expand left} $fwd_frame_button {fill expand left} $bwd_frame_button {fill expand left}
	
	# create status frame
	
	set fl [label $status_frame1.fl -text "Status:"]
	set fe [label $status_frame1.fe -textvariable status_message]
	
	pack append $status_frame1 $fl {left} $fe {fill expand left}
	
	set ml [label $status_frame2.ml -text "Mode:"]
	set me [label $status_frame2.me -textvariable current_mode]
	
	pack append $status_frame2 $ml {left} $me {fill expand left}
	
	get_current_mode
	set current_speed 180
	vdr-v1000-command ${current_speed}SP;
}
