#!/udir/payne/bin.mips/wish -f
#
#  general.tcl
#
#	General purpose routines for creating tk widgets, version 2
#
#	Andrew C. Payne
#	payne@crl.dec.com
#

option add *Entry*BorderWidth	2
option add *Entry*Background	white
option add *Entry*Relief	sunken
option add *Entry*Font		-*-courier-bold-r-*-*-14-*-*-*-*-*-*-*

option add *Text.Font		fixed

#----------------------------------------------------------------------------
#  Create a frame and pack into parent
#----------------------------------------------------------------------------
proc mkFrame {w {pack {top expand fill}} args} {
	eval frame $w $args
	pack append [winfo parent $w] $w $pack
	return $w
}

#------------------------------------------------------------------------------
#  Create a frame inside of a frame for trim effects
#------------------------------------------------------------------------------
proc mkTrim {w {pack {top expand fill}} {border 1} {width 0} {relief sunken}} {
        if {$relief == "sunken"} {
                set arelief raised
        } {
                set arelief sunken
        }
        mkFrame $w $pack -relief $relief -bd $border
        return [mkFrame $w.frame "top expand fill padx $width pady $width" \
			-relief $arelief -bd $border]
}

#----------------------------------------------------------------------------
#  Make a label and pack into parent.
#----------------------------------------------------------------------------
proc mkLabel {w {text ""} {pack {top fillx}} args} {
	eval label $w -text \"$text\" $args
	pack append [winfo parent $w] $w $pack
	return $w
}

#----------------------------------------------------------------------------
#  Make a message and pack into parent.
#----------------------------------------------------------------------------
proc mkMessage {w {text ""} {pack {top fillx}} args} {
	eval message $w -text \"$text\" $args
	pack append [winfo parent $w] $w $pack
	return $w
}

#----------------------------------------------------------------------------
#  Make a entry and pack into parent.
#----------------------------------------------------------------------------
proc mkEntry {w {pack {top expand fillx}} args} {
	eval entry $w $args
	pack append [winfo parent $w] $w $pack
	return $w
}

#------------------------------------------------------------------------------
#  Create a trim area with a label, return innermost frame.
#------------------------------------------------------------------------------
proc mkTrimLabel {w text {pack {top padx 10 pady 10}}} {
        mkFrame $w $pack
        mkLabel $w.label $text {top frame w}
        return [mkTrim $w.subframe {top expand fill frame w}]
}


#----------------------------------------------------------------------------
#  Create a new button, pack into parent.
#----------------------------------------------------------------------------
proc mkButton {w label command {pack {left padx 10 pady 10}} args} {
	eval button $w -text \"$label\" -command \{$command\} $args
	pack append [winfo parent $w] $w $pack
	return $w
}

#----------------------------------------------------------------------------
#  Create a new radio button, pack into parent.
#----------------------------------------------------------------------------
proc mkRadioButton {w label var {pack {top frame w padx 5 pady 5}} args} {
	eval radiobutton $w -anchor w -text \"$label\" -variable $var \
		-relief flat $args
	pack append [winfo parent $w] $w $pack
	return $w
}

#----------------------------------------------------------------------------
#  Create a new check button, pack into parent.
#----------------------------------------------------------------------------
proc mkCheckButton {w label var {pack {top fillx }} args} {
	checkbutton $w -anchor w -text $label -variable $var \
		-relief flat $args
	pack append [winfo parent $w] $w $pack
	return $w
}

#----------------------------------------------------------------------------
#  Create a new menubutton, pack into parent
#----------------------------------------------------------------------------
proc mkMenubutton {w label menu {pack left} args} {
	eval menubutton $w -menu $menu -text $label $args
	pack append [winfo parent $w] $w $pack
	return $w
}

#----------------------------------------------------------------------------
#  Create a drop-down menu:  menubutton and menu, return path to menu
#----------------------------------------------------------------------------
proc mkDropMenu {w label {pack left} args} {
	eval menubutton $w -menu $w.menu -text $label $args
	pack append [winfo parent $w] $w $pack
	return [menu $w.menu]
}

#----------------------------------------------------------------------------
#  Create a listbox with scrollbar and pack into parent.  Return path to 
#  list widget.
#----------------------------------------------------------------------------
proc mkListbox {w {pack {top expand fill}} args} {
	frame $w
	pack append $w \
		[scrollbar $w.scroll -command "$w.list yview"] {right filly} \
		[eval listbox $w.list -yscroll \"$w.scroll set\" -relief raised \
			-bd 1 $args] {left expand fill}

	pack append [winfo parent $w] $w $pack
	return $w.list
}

#----------------------------------------------------------------------------
#  Make a labeled entry and pack into parent.
#----------------------------------------------------------------------------
proc mkLabelEntry {w {text ""} {pack {top fillx}}} {
	frame $w
	pack append $w \
		[label $w.label -text $text] {left fillx}  \
		[entry $w.entry -relief sunk -width 40] {right padx 10}

	pack append [winfo parent $w] $w $pack
	return $w.entry
}

#-----------------------------------------------------------------------------
# Make a text object with a scrollbar.  Return name of text object
#-----------------------------------------------------------------------------
proc mkText {w {pack {top filly}} args} {
	frame $w
        pack append $w \
                [scrollbar $w.scroll -relief flat -command "$w.text yview"] \
                        {right filly} \
                [eval text $w.text -bd 1 -relief raised \
			-yscrollcommand \"$w.scroll set\" \
                        -width 80 -height 40  -wrap none $args] {expand fill}

        pack append [winfo parent $w] $w $pack
        return $w.text
}

#-----------------------------------------------------------------------------
# Make a new toplevel window.
#-----------------------------------------------------------------------------
proc mkWindow {w title} {
	catch "destroy $w"
	wm title [toplevel $w] $title
	return $w
}
	
#-----------------------------------------------------------------------------
#  Run a command and make the top level window go "busy"
#	(stolen from the Tk FAQ file)
#-----------------------------------------------------------------------------
proc runBusy {args} {
    global errorInfo

    set busy {.app .root}
    set list [winfo children .]
    while {$list != ""} {
        set next {}
        foreach w $list {
            set class [winfo class $w]
            set cursor [lindex [$w config -cursor] 4]
            if {[winfo toplevel $w] == $w || $cursor != ""} {
                lappend busy [list $w $cursor]
            }
            set next [concat $next [winfo children $w]]
        }
        set list $next
    }

    foreach w $busy {
        catch {[lindex $w 0] config -cursor watch}
    }

    update idletasks

    set error [catch {uplevel eval [list $args]} result]
    set ei $errorInfo

    foreach w $busy {
        catch {[lindex $w 0] config -cursor [lindex $w 1]}
    }

    if $error {
        error $result $ei
    } else {
        return $result
    }
}
