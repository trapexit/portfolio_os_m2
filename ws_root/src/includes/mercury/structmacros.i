	
		.macro		define name,value
		.equ		\name,\value
		.endm
	
		.macro		struct name
		.set		structtemp,0
		.endm
	
		.macro		stlong label,count
		.set		\label,structtemp
		.set		structtemp,structtemp+(4*\count)
		.endm
	
		.macro		stbyte label,count
		.set		\label,structtemp
		.set		structtemp,structtemp+(\count)
		.endm

		.macro		stword label,count
		.set		\label,structtemp
		.set		structtemp,structtemp+(2*\count)
		.endm
	
		.macro		ends name
		.set		\name,structtemp
		.endm

