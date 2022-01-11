#set tex [gdev_load_texture ../data/images/rgb/brick.rgb]

global pts 
set tex [gdev_load_texture ../../data/images/utf/vertical.utf]

proc dq {} {
	global pts

	te_quad $pts(x0) $pts(y0) $pts(x1) $pts(y1) $pts(x2) $pts(y2) \
			$pts(x3) $pts(y3)
}

set pts(x0) 150
set pts(y0) 150

set pts(x1) 300
set pts(y1) 150

set pts(x2) 300
set pts(y2) 300

set pts(x3) 150
set pts(y3) 300

#te_quad 150 150 300 150 300 300 150 300

dq
