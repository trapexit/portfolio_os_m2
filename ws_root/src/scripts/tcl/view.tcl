# set scene [view]
# set camera [$scene -get_camera]
# set the_object [$scene -get_object]

set max_dist 10

proc sdf_list { } { SDF_ListSymbols }

proc list_characters {} {
	SDF_ListSymbols scene
    SDF_ListSymbols character
}

proc sdf_load {fileName} {
    set sdf [ SDF_Open $fileName ]
    list_characters
    puts [format "sdf = %s" $sdf]
    return $sdf
}

proc bind2 {w e1 e2 c} {bind $w $e1 $c; bind $w $e2 $c}

proc calc_distance {xf yf zf xs ys zs} {
expr sqrt((pow(($xf - $xs), 2)) + pow(($yf - $ys), 2) + pow(($zf - $zs), 2))
}

proc load_sdf_file {} {
	global env

	sdf_load [FSBox "Select SDF file:" $env(SDFDATA)]
}

proc move {obj  x y z} {
	Char_Translate $obj $x $y $z
	draw_model
}

proc rotate {obj x y z} {
	Char_Rotate $obj TRANS_XAxis $x
	Char_Rotate $obj TRANS_YAxis $y
	Char_Rotate $obj TRANS_ZAxis $z
	draw_model
}

proc scale {obj x y z} {
	Char_Scale $obj $x $y $z
	draw_model
}

proc lookat {obj loc {twist 0}} {
	Char_LookAt $obj $loc $twist
	draw_model
}

proc get_bounds {} {
    global t_delta
    global scene
    global max_dist
    global the_object
    global obj_ctr

    set s [Char_GetBound $the_object]
    set x_min [lindex $s 0]
    set y_min [lindex $s 1]
    set z_min [lindex $s 2]
    set x_max [lindex $s 3]
    set y_max [lindex $s 4]
    set z_max [lindex $s 5]

    set obj_ctr [Char_GetCenter $the_object]

    puts $obj_ctr

    set max_dist [calc_distance $x_min $y_min $z_min $x_max $y_max $z_max]
    set t_delta [expr 0.001*$max_dist]
}

proc recompute_view {} {
	global t_delta
	global scene
	global max_dist
	global the_object
	global obj_ctr

	Char_Reset $the_object

	get_bounds
}

proc view {name} {
    global scene
	global default_scene
	global the_object
	global obj_ctr

	set scene $default_scene
	set the_object [get_char $name]
	Scene_SetGroup $scene $the_object
    recompute_view
	Char_Translate $the_object [expr -[lindex $obj_ctr 0]] [expr -[lindex $obj_ctr 1]] [expr -[lindex $obj_ctr 2]]
    Scene_ShowAll $scene
    init_model
	gdev_set_file_prefix $name
    return $the_object
}

proc sview {name} {
    global scene
	global default_scene
	global the_object
	global obj_ctr
	global camera

    set scene [get_scene $name]
	set the_object [Scene_GetDynamic $scene]
	set camera [Scene_GetCamera $scene]
	recompute_view
    Char_Translate $the_object [expr -[lindex $obj_ctr 0]] [expr -[lindex $obj_ctr 1]] [expr -[lindex $obj_ctr 2]]
    Scene_ShowAll $scene
    init_model
    gdev_set_file_prefix $name
    return $the_object
}

proc fview {filename} {
	global scene
    global the_object

	set the_object [load_group $filename]
#	puts [format "load group 0x%lx" $the_object]
	Scene_SetGroup $scene $the_object
	Scene_ShowAll $scene
	init_model
	recompute_view
	return $the_object
}

proc mult {state delta} {
	global Button1Mask Button2Mask Button3Mask ControlMask ShiftMask

	if { $state & $ControlMask } {
		return [expr $delta*100]
	} elseif { $state & $ShiftMask } {
		return [expr $delta*10]
	} else {
		return $delta
	}
}

proc update_render_mode { rm } {
	global render_mode last_render_mode

	set last_render_mode $rm
	set render_mode $rm
	RenderMode $render_mode
}

proc set_render_mode { rm } {
	global render_mode last_render_mode
	global changed

	if { $rm != $last_render_mode } { 
		update_render_mode $rm
		set changed 1
		draw_model
	}
}

proc update_hlhsr_mode { rm } {
	global hlhsr_mode last_hlhsr_mode

	set last_hlhsr_mode $rm
	set hlhsr_mode $rm
#	glib_hlhsr_mode $hlhsr_mode 
}

proc set_hlhsr_mode { rm } {
	global hlhsr_mode last_hlhsr_mode
	global changed

	if { $rm != $last_hlhsr_mode } { 
		update_hlhsr_mode $rm
		set changed 1
		draw_model
	}
}

proc update_draw_mode { pm } {
	global draw_mode last_draw_mode

	set last_draw_mode $pm
	set draw_mode $pm
	draw_mode $draw_mode 
}

proc set_draw_mode { pm } {
	global draw_mode last_draw_mode
	global changed

	if { $pm != $last_draw_mode } { 
		update_draw_mode $pm
		set changed 1
		draw_model
	}
}

proc update_sim_output { pm } {
	global sim_output last_sim_output

	set last_sim_output $pm
	set sim_output $pm
	gdev_draw_mode $sim_output 
}

proc set_sim_output { pm } {
	global sim_output last_sim_output
	global changed

	if { $pm != $last_sim_output } { 
		update_sim_output $pm
		set changed 1
		draw_model
	}
}

proc update_lighting {} {
	global lighting
	global changed

	if { $lighting == 1 } {
		enable GP_Lighting
	} else {
		disable GP_Lighting
	} 
}

proc set_lighting {} {
	global lighting
	global changed

	update_lighting
	set changed 1
	draw_model
}

proc update_culling {} {
	global culling

	if { $culling == 1 } {
		cull_faces GP_Back
	} else {
		cull_faces GP_None
	} 
}

proc set_culling {} {
	global changed

	update_culling
	set changed 1
	draw_model
}

proc set_texturing {} {
	global texturing
	global changed

    if { $texturing == 1 } {
       	enable GP_Texturing
    } else {
		disable GP_Texturing
    }

	set changed 1
	draw_model
}

proc set_dithering {} {
	global dithering
	global changed

    if { $dithering == 1 } {
       	enable GP_Dithering
    } else {
		disable GP_Dithering
    }

	set changed 1
	draw_model
}

proc update_verification {} {
    global verification

    if { $verification == 1 } {
        verify on
    } else {
        verify off
    }
}

proc set_verification {} {
	global verification
	global changed

	update_verification
	set changed 1
	draw_model
}

proc update_trace {} {
    global trace

    if { $trace == 1 } {
        verify on
    } else {
        verify off
    }
}

proc set_trace {} {
    global trace
    global changed

	update_trace
    set changed 1
    draw_model
}

proc mouse_motion {x y state} {
	global oldPointerX oldPointerY
	global Button1Mask Button2Mask Button3Mask ControlMask ShiftMask
	global SMALLMOVEMENT POINTERRATIO
	global shiftX shiftY shiftZ angleX angleY angleZ scaleX scaleY scaleZ
	global changed

	set dx [expr $x-$oldPointerX]
	set dy [expr $y-$oldPointerY]

	if { ($dy * $dy <= $SMALLMOVEMENT) && ($dx * $dx <= $SMALLMOVEMENT) } {
 		if { $state & $Button1Mask } {
            set angleX [expr $angleX - ($dx * $POINTERRATIO)]
            set angleY [expr $angleY - ($dy * $POINTERRATIO)]
            set changed 1
        } elseif { $state & $Button2Mask } {
            set angleY [expr $angleY - ($dy * $POINTERRATIO)]
            set angleZ [expr $angleZ - ($dx * $POINTERRATIO)]
            set changed 1
        } elseif { $state & $Button3Mask } {
            set angleZ [expr $angleZ - ($dy * $POINTERRATIO)]
            set angleX [expr $angleX - ($dx * $POINTERRATIO)]
            set changed 1
        }
    }

    set oldPointerY  $x
    set oldPointerX  $y
	if { $changed } { draw_model }
	return $changed
}


proc do_draw_model {} {
	global scene
	global camera
	global shiftX shiftY shiftZ angleX angleY angleZ scaleX scaleY scaleZ
	global changed
	global render_mode
	global hlhsr_mode
	global gfx
	global the_object
	global obj_ctr

	if [ expr ! [info exists the_object] ] {
		set changed 0
		return
	}

	if {  [ cequal $render_mode GP_SoftRender ] } {
#		if { $changed == 0 } {
#			glib_update_from_memory 
#			return
#		}

		fb_cursor {clock red white}
		update idletasks
	}
	
#	puts "in draw model"
#	puts [format "object center %f %f %f" [lindex $obj_ctr 0] [lindex $obj_ctr 1] [lindex $obj_ctr 2]]
#	puts [format "move to %f %f %f" $shiftX $shiftY $shiftZ]
#	puts [format "orient %f %f %f" $angleX $angleY $angleZ]
#	puts [format "scale %f %f %f" $scaleX $scaleY $scaleZ]

	Char_Reset $the_object
	Char_Translate $the_object [expr -[lindex $obj_ctr 0]] [expr -[lindex $obj_ctr 1]] [expr -[lindex $obj_ctr 2]]
	Char_Scale $the_object $scaleX $scaleY $scaleZ
	Char_Rotate $the_object TRANS_XAxis $angleX
	Char_Rotate $the_object TRANS_YAxis $angleY
	Char_Rotate $the_object TRANS_ZAxis $angleZ
	Char_Translate $the_object $shiftX $shiftY $shiftZ

	display $scene
	if {  [ cequal $render_mode GP_SoftRender ] } {
		fb_cursor {crosshair black white}
		update idletasks
	}

	set changed 0
}

proc draw_model {} {
	catch {do_draw_model}
}

proc update_view {} {
	set changed 1
	do_draw_model 
}

proc init_model {} {
	global gfx
	global camera
	global argv
	global shiftX shiftY shiftZ angleX angleY angleZ scaleX scaleY scaleZ
	global oldPointerX oldPointerY
	global Button1Mask Button2Mask Button3Mask ControlMask ShiftMask
	global SMALLMOVEMENT POINTERRATIO
	global changed
	global culling
	global last_render_mode
	global last_hlhsr_mode
	global last_draw_mode
	global last_sim_output
	global lighting
	global texturing
	global dithering
	global verification
	global trace
	global t_delta s_delta r_delta
	global win_mode

	set s_delta .001
	set r_delta .225

	set angleX 0.0
	set angleY 0.0
	set angleZ 0.0

	set scaleX 1.0
	set scaleY 1.0
	set scaleZ 1.0

	set shiftX 0.0
	set shiftY 0.0
	set shiftZ 0.0

	set oldPointerX 0
	set oldPointerY 0

	set ControlMask [expr (1<<2)]
	set ShiftMask [expr (1<<0)]
	set Button1Mask [expr (1<<8)]
	set Button2Mask [expr (1<<9)]
	set Button3Mask [expr (1<<10)]

	set SMALLMOVEMENT 40000
	set POINTERRATIO 0.1

	set changed 0

	set culling 1
	update_culling
	set last_render_mode -1

	if { $win_mode == "GLX" } {
		update_render_mode GP_GLRender
	} else {
		update_render_mode GP_SoftRender
	}


	set last_hlhsr_mode -1
	update_hlhsr_mode GLIB_HLHSR_ZBUFFER
	set lighting 1
	update_lighting
    enable GP_Texturing
	set texturing 1
	set dithering 1
	set verification 0; update_verification
	set trace 0; update_trace

	update_view
}

proc init_bindings {w} {
	global gfx
	global camera
	global argv
	global shiftX shiftY shiftZ angleX angleY angleZ scaleX scaleY scaleZ
	global oldPointerX oldPointerY
	global Button1Mask Button2Mask Button3Mask ControlMask ShiftMask
	global SMALLMOVEMENT POINTERRATIO
	global changed
	global culling
	global last_render_mode
	global last_hlhsr_mode
	global last_draw_mode
	global lighting
	global verification
	global trace
	global t_delta s_delta r_delta

	bind $w <Escape> {exit}
	bind . <F1> {fb_normal}
    bind . <F2> {fb_magnify}
    bind . <F3> {fb_report}
	bind $w <Any-B1-Motion> {mouse_motion %x %y %s}
	bind $w <Any-B2-Motion> {mouse_motion %x %y %s}
	bind $w <Any-B3-Motion> {mouse_motion %x %y %s}

	bind $w <Any-Left> { set t [ mult %s $t_delta]; 
					global changed
				    set shiftX [ expr $shiftX - $t]; 
					set changed 1
				    draw_model 
				}
	bind $w <Any-Right> { set t [ mult %s $t_delta];
					global changed
                    set shiftX [ expr $shiftX + $t];
					set changed 1
                    draw_model
                }
	bind $w <Any-Down> { set t [ mult %s $t_delta];
					global changed
                    set shiftY [ expr $shiftY - $t];
					set changed 1
                    draw_model
                }
	bind $w <Any-Up> { set t [ mult %s $t_delta];
					global changed
                    set shiftY [ expr $shiftY + $t];
					set changed 1
                    draw_model
                }
	bind2 $w <Any-N> <Any-n>  { set t [ mult %s $t_delta];
					global changed
                    set shiftZ [ expr $shiftZ + $t];
					set changed 1
                    draw_model
                }
	bind2 $w <Any-Key-M> <Any-Key-m>  { set t [ mult %s $t_delta];
					global changed
                    set shiftZ [ expr $shiftZ - $t];
					set changed 1
                    draw_model
                }

# scaling bindings 

	bind2 $w <Any-Key-Q> <Any-Key-q> { set t [ mult %s $s_delta];
					global changed
                    set scaleX [ expr $scaleX - $t];
					set changed 1
                    draw_model
                }
    bind2 $w <Any-Key-W> <Any-Key-w> { set t [ mult %s $s_delta];
					global changed
                    set scaleX [ expr $scaleX + $t];
					set changed 1
                    draw_model
                }
    bind2 $w <Any-Key-A> <Any-Key-a> { set t [ mult %s $s_delta];
					global changed
                    set scaleY [ expr $scaleY - $t];
					set changed 1
                    draw_model
                }
    bind2 $w <Any-Key-S> <Any-Key-s> { set t [ mult %s $s_delta];
					global changed
                    set scaleY [ expr $scaleY + $t];
					set changed 1
                    draw_model
                }
    bind2 $w <Any-Key-Z> <Any-Key-z> { set t [ mult %s $s_delta];
					global changed
                    set scaleZ [ expr $scaleZ + $t];
					set changed 1
                    draw_model
                }
    bind2 $w <Any-Key-X> <Any-Key-x>  { set t [ mult %s $s_delta];
					global changed
                    set scaleZ [ expr $scaleZ - $t];
					set changed 1
                    draw_model
                }

# rotation bindings

    bind2 $w <Any-Key-E> <Any-Key-e> { set t [ mult %s $r_delta];
					global changed
                    set angleX [ expr $angleX - $t];
					set changed 1
                    draw_model
                }
    bind2 $w <Any-Key-R> <Any-Key-r> { set t [ mult %s $r_delta];
					global changed
                    set angleX [ expr $angleX + $t];
					set changed 1
                    draw_model
                }
    bind2 $w <Any-Key-D> <Any-Key-d> { set t [ mult %s $r_delta];
					global changed
                    set angleY [ expr $angleY - $t];
					set changed 1
                    draw_model
                }
    bind2 $w <Any-Key-F> <Any-Key-f> { set t [ mult %s $r_delta];
					global changed
                    set angleY [ expr $angleY + $t];
					set changed 1
                    draw_model
                }
    bind2 $w <Any-Key-C> <Any-Key-c> { set t [ mult %s $r_delta];
					global changed
                    set angleZ [ expr $angleZ + $t];
					set changed 1
                    draw_model
                }
    bind2 $w <Any-Key-V> <Any-Key-v>  { set t [ mult %s $r_delta];
					global changed
                    set angleZ [ expr $angleZ - $t];
					set changed 1
                    draw_model
                }
}

# frame mframe holds the menu bar, f2 holds the graphics window

#
#  Show a help window
#
proc mkHelp {} {
    set w .help

    catch "destroy $w"
        toplevel $w
        wm title $w "SDF Viewer Help"

        set t [mkText $w.t {top} -height 25]
        set f [mkFrame $w.buttons {fillx bottom} -relief raised -borderw 1]
        mkButton $f.close  " Close " "destroy $w" {right padx 7 pady 7}
        $t insert current {

General purpose viewer for SDF:

	To load an object use:

		sdf_load <filename>

	To get a list of displayable characters use:

		list_characters

	To display a character use:

		view <character>

	To get a list of all named objects use:

		print_symbols

	To redraw the display use:

		display $scene

	Bindings for object manipulation
		<Escape>    Exit Program

		Left Arrow  Translate in X (Negative)
		Right Arrow Translate in X (Positive)
		Up Arrow    Translate in Y (Negative)
		Down Arrow  Translate in Y (Positive)
		N           Translate in Z (Negative)
		M           Translate in Z (Positive)

		Q           Scale in X (Negative)
		W           Scale in X (Positive)
		A           Scale in Y (Negative)
		S           Scale in Y (Positive)
		X           Scale in Z (Negative)
		Z           Scale in Z (Positive)

		E           Rotate in X (Negative)
		R           Rotate in X (Positive)
		D           Rotate in Y (Negative)
		F           Rotate in Y (Positive)
		C           Rotate in Z (Negative)
		V           Rotate in Z (Positive)

		Control and Shift Modify amount translated/scaled/rotated

		Mouse Buttons:-

			Button 1: 
				rotate in X (horizontal) 
				rotate in Y (vertical) 

			Button 2: 
				rotate in Z (horizontal) 
				rotate in Y (vertical) 

			Button 3: 
				rotate in X (horizontal) 
				rotate in Z (vertical) 

    }
}

proc viewer_init {} {
	global scene
	global default_scene
	global camera
	global gfx
	global win_mode
	global env
	global sim_output last_sim_output
	global draw_mode last_draw_mode
	
	if [ catch { set win_mode $env(WIN_MODE) } ] {
		set win_mode X
	} 

	wm title . "Software Renderer"
	wm geometry . +20+20
    set default_scene [Scene_Create]
	set scene $default_scene
	puts [format "created scene = 0x%lx" $scene]
	set camera [Scene_GetCamera $scene]
	puts [format "got camera = 0x%lx" $camera]
	set controls [toplevel .controls]
	wm geometry $controls 300x200+700+20
	wm title $controls "SDF Controls"
	
	init_bindings .
	catch { init_bindings .gltop }

	set mframe [frame $controls.mframe -borderwidth {2} -relief {raised}]
	pack $mframe -side top -fill x
	
	menubutton $mframe.help -text "Help" -menu $mframe.help.menu
	pack append $mframe $mframe.help {right}
	menu $mframe.help.menu
	$mframe.help.menu add command -label "Help!" -command {mkHelp}
	
	menubutton $mframe.file -text "File" -underline 0 -menu $mframe.file.menu -anchor nw
	pack $mframe.file -side left
	
	menu $mframe.file.menu
	
	$mframe.file.menu add command -label "VDR-V100 ..." -command {create_video_panel}
	
	$mframe.file.menu add command -label "Load SDF File..." -command {load_sdf_file}
	
	$mframe.file.menu add command -label "Exit" -command {destroy .} -accelerator <Esc>
	
	# create rendering menu
	
	menubutton $mframe.rendering -text "Rendering" -underline 0 -menu $mframe.rendering.menu -anchor nw
	pack $mframe.rendering -side left
	
	menu $mframe.rendering.menu
	
	# render mode
	
	if { $win_mode == "GLX" } {
		$mframe.rendering.menu add cascade -label "Render Mode" -menu $mframe.rendering.menu.render_mode
		menu $mframe.rendering.menu.render_mode 
	
		$mframe.rendering.menu.render_mode add radiobutton -label "GL" -command { set_render_mode GP_GLRender } -value GP_GLRender -variable render_mode
		$mframe.rendering.menu.render_mode add radiobutton -label "Soft" -command { set_render_mode GP_SoftRender } -value GP_SoftRender -variable render_mode 
	}
	
	# hlhsr mode
	
	$mframe.rendering.menu add cascade -label "HLHSR Mode" -menu $mframe.rendering.menu.hlhsr_mode
	menu $mframe.rendering.menu.hlhsr_mode 
	
	$mframe.rendering.menu.hlhsr_mode add radiobutton -label "Off" -command { set_hlhsr_mode GLIB_HLHSR_OFF } -value GLIB_HLHSR_OFF -variable hlhsr_mode
	$mframe.rendering.menu.hlhsr_mode add radiobutton -label "Painters" -command { set_hlhsr_mode GLIB_HLHSR_PAINTERS } -value GLIB_HLHSR_PAINTERS -variable hlhsr_mode 
	$mframe.rendering.menu.hlhsr_mode add radiobutton -label "Z Buffer" -command { set_hlhsr_mode GLIB_HLHSR_ZBUFFER } -value GLIB_HLHSR_ZBUFFER -variable hlhsr_mode
	
	# draw_mode
	
	set last_draw_mode -1
	update_draw_mode GP_Lines

	menu $mframe.rendering.menu.draw_mode 
	
	$mframe.rendering.menu add cascade -label "Draw Mode" -menu $mframe.rendering.menu.draw_mode
	$mframe.rendering.menu.draw_mode add radiobutton -label "Point" -command { set_draw_mode GP_Points } -value GP_Points -variable draw_mode
	$mframe.rendering.menu.draw_mode add radiobutton -label "Line" -command { set_draw_mode GP_Lines } -value GP_Lines -variable draw_mode 
	$mframe.rendering.menu.draw_mode add radiobutton -label "Fill" -command { set_draw_mode GP_Filled } -value GP_Filled -variable draw_mode 
	
	# lighting enable
	
	$mframe.rendering.menu add separator
	$mframe.rendering.menu add checkbutton -label "Lighting" -command { set_lighting} -variable lighting
	
	# culling enable
	$mframe.rendering.menu add checkbutton -label "Culling" -command { set_culling} -variable culling 
	
	# texture enable
	
	$mframe.rendering.menu add checkbutton -label "Texturing" -command { set_texturing} -variable texturing 
	
	# dithering enable
	
	$mframe.rendering.menu add checkbutton -label "Dithering" -command { set_dithering} -variable dithering 
	
	# create simulator menu
	
	menubutton $mframe.simulator -text "Simulator" -underline 0 -menu $mframe.simulator.menu -anchor nw
	pack $mframe.simulator -side left
	
	menu $mframe.simulator.menu

	$mframe.simulator.menu add checkbutton -label "Verification" -command { set_verification} -variable verification 
	
	$mframe.simulator.menu add checkbutton -label "Tracing" -command { set_trace} -variable trace 

	# simulator output

	set last_sim_output -1
	update_sim_output GDEV_WIREFRAME
    menu $mframe.simulator.menu.simulator_output

    $mframe.simulator.menu add cascade -label "Simulator Output" -menu $mframe.simulator.menu.simulator_output
    $mframe.simulator.menu.simulator_output add radiobutton -label "Wireframe" -command { set_sim_output GDEV_WIREFRAME } -value GDEV_WIREFRAME -variable sim_output
    $mframe.simulator.menu.simulator_output add radiobutton -label "Shaded" -command { set_sim_output GDEV_SHADED } -value GDEV_SHADED -variable sim_output
    $mframe.simulator.menu.simulator_output add radiobutton -label "Pixel Exact" -command { set_sim_output GDEV_PIXEL_EXACT } -value GDEV_PIXEL_EXACT -variable sim_output
    $mframe.simulator.menu.simulator_output add radiobutton -label "Command List" -command { set_sim_output GDEV_SIM_OUTPUT } -value GDEV_SIM_OUTPUT -variable sim_output
	
	# set up menu traversal
	
	tk_menuBar $mframe $mframe.file $mframe.rendering $mframe.simulator
	
	# initialize lights
	
#	$scene -ambient {.4 .4 .4 1.0} 
		
#	init_materials

 	init_model
}
