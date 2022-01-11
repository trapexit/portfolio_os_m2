proc init_gl_window { {width 640} {height 480}} {
	global gfx

# make sure window is out of the way

	wm geometry . +20+20 

# create GL window

	toplevel .gltop 
	wm title .gltop "GL Renderer"
	wm geometry .gltop ${width}x${height}+20+530
#	bind .gltop <Enter> {focus .gltop}
	.gltop config -cursor {crosshair black white}
	
	set gfx [glxwin .gltop.glxWindow -width $width -height $height -rgb true -db true]
	pack append .gltop $gfx {expand fill}
	.gltop.glxWindow ref "gl_window"
	.gltop.glxWindow winset
	.gltop.glxWindow islinked
}
