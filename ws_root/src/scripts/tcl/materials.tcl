proc init_materials {} {
	global emerald jade obsidian pearl ruby turquoise brass bronze chrome 
	global copper gold silver black_plastic cyan_plastic green_plastic 
	global red_plastic white_plastic yellow_plastic black_rubber cyan_rubber 
	global green_rubber red_rubber white_rubber

set emerald [material -ambient { 0.021500 0.174500 0.021500} -diffuse { 0.075680 0.614240 0.075680} -specular { 0.633000 0.727811 0.633000} -shine 0.600000]
set jade [material -ambient { 0.135000 0.222500 0.157500} -diffuse { 0.540000 0.890000 0.630000} -specular { 0.316228 0.316228 0.316228} -shine 0.100000]
set obsidian [material -ambient { 0.053750 0.050000 0.066250} -diffuse { 0.182750 0.170000 0.225250} -specular { 0.332741 0.328634 0.346435} -shine 0.300000]
set pearl [material -ambient { 0.250000 0.207250 0.207250} -diffuse { 1.000000 0.829000 0.829000} -specular { 0.296648 0.296648 0.296648} -shine 0.088000]
set ruby [material -ambient { 0.174500 0.011750 0.011750} -diffuse { 0.614240 0.041360 0.041360} -specular { 0.727811 0.626959 0.626959} -shine 0.600000]
set turquoise [material -ambient { 0.100000 0.187250 0.174500} -diffuse { 0.396000 0.741510 0.691020} -specular { 0.297254 0.308290 0.306678} -shine 0.100000]
set brass [material -ambient { 0.329412 0.223529 0.027451} -diffuse { 0.780392 0.568627 0.113725} -specular { 0.992157 0.941176 0.807843} -shine 0.217949]
set bronze [material -ambient { 0.212500 0.127500 0.054000} -diffuse { 0.714000 0.428400 0.181440} -specular { 0.393548 0.271906 0.166721} -shine 0.200000]
set chrome [material -ambient { 0.250000 0.250000 0.250000} -diffuse { 0.400000 0.400000 0.400000} -specular { 0.774597 0.774597 0.774597} -shine 0.600000]
set copper [material -ambient { 0.191250 0.073500 0.022500} -diffuse { 0.703800 0.270480 0.082800} -specular { 0.256777 0.137622 0.086014} -shine 0.100000]
set gold [material -ambient { 0.247250 0.199500 0.074500} -diffuse { 0.751640 0.606480 0.226480} -specular { 0.628281 0.555802 0.366065} -shine 0.400000]
set silver [material -ambient { 0.192250 0.192250 0.192250} -diffuse { 0.507540 0.507540 0.507540} -specular { 0.508273 0.508273 0.508273} -shine 0.400000]
set black_plastic [material -ambient { 0.000000 0.000000 0.000000} -diffuse { 0.010000 0.010000 0.010000} -specular { 0.500000 0.500000 0.500000} -shine 0.250000]
set cyan_plastic [material -ambient { 0.000000 0.100000 0.060000} -diffuse { 0.000000 0.509804 0.509804} -specular { 0.501961 0.501961 0.501961} -shine 0.250000]
set green_plastic [material -ambient { 0.000000 0.000000 0.000000} -diffuse { 0.100000 0.350000 0.100000} -specular { 0.450000 0.550000 0.450000} -shine 0.250000]
set red_plastic [material -ambient { 0.000000 0.000000 0.000000} -diffuse { 0.500000 0.000000 0.000000} -specular { 0.700000 0.600000 0.600000} -shine 0.250000]
set white_plastic [material -ambient { 0.000000 0.000000 0.000000} -diffuse { 0.550000 0.550000 0.550000} -specular { 0.700000 0.700000 0.700000} -shine 0.250000]
set yellow_plastic [material -ambient { 0.000000 0.000000 0.000000} -diffuse { 0.500000 0.500000 0.000000} -specular { 0.600000 0.600000 0.500000} -shine 0.250000]
set black_rubber [material -ambient { 0.020000 0.020000 0.020000} -diffuse { 0.010000 0.010000 0.010000} -specular { 0.400000 0.400000 0.400000} -shine 0.078125]
set cyan_rubber [material -ambient { 0.000000 0.050000 0.050000} -diffuse { 0.400000 0.500000 0.500000} -specular { 0.040000 0.700000 0.700000} -shine 0.078125]
set green_rubber [material -ambient { 0.000000 0.050000 0.000000} -diffuse { 0.400000 0.500000 0.400000} -specular { 0.040000 0.700000 0.040000} -shine 0.078125]
set red_rubber [material -ambient { 0.050000 0.000000 0.000000} -diffuse { 0.500000 0.400000 0.400000} -specular { 0.700000 0.040000 0.040000} -shine 0.078125]
set white_rubber [material -ambient { 0.050000 0.050000 0.050000} -diffuse { 0.500000 0.500000 0.500000} -specular { 0.700000 0.700000 0.700000} -shine 0.078125]
set yellow_rubber [material -ambient { 0.050000 0.050000 0.000000} -diffuse { 0.500000 0.500000 0.400000} -specular { 0.700000 0.700000 0.040000} -shine 0.078125]
}
