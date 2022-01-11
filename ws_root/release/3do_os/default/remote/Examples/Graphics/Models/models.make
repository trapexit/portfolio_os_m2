# Copyright (c) 1995, The 3DO Company
#		v1.1 092595 mattm - fixed NewFolder command
#       v1.0 080495 mattm - original version

# This makefile will convert the example models to ascii sdf and then
# optimize them using the Slice & Dice tool.

Name			= Models
3DOAutoDup		= -y
3DS				= 3dstosdf
STRATA			= ssptosdf
SDTOOL			= gcomp
SDTOOL_OPTS		= -o

# Models created by 3D Studio (PC)
3DS_TO_SDF		= stp.sdf

# Models created by Strata (Mac)
STRATA_TO_SDF	= lifting_body.sdf

SDF_TO_CSF		= stp.csf			¶
				  lifting_body.csf
			  
{Name}			Ä	all

all				Ä	Directories {3DS_TO_SDF} {STRATA_TO_SDF} {SDF_TO_CSF}
	echo All Done!
		
Directories		Ä
	If Not "`Exists -d {3DORemote}Examples:`"
		NewFolder {3DORemote}Examples:
	End
	If Not "`Exists -d {3DORemote}Examples:Graphics:`"
		NewFolder {3DORemote}Examples:Graphics:
	End
	If Not "`Exists -d {3DORemote}Examples:Graphics:Models:`"
		NewFolder {3DORemote}Examples:Graphics:Models:
	End
	echo
	
.sdf			Ä	.3ds
	{3DS} -o {default}.sdf {default}.3ds
	
.sdf			Ä	.ssp
	{STRATA} -o {default}.sdf {default}.ssp
	
.csf			Ä	.sdf
	{SDTOOL} {SDTOOL_OPTS} -b {default}.csf {default}.sdf
	move {3DOAutodup} {default}.csf {3DORemote}Examples:Graphics:Models:
