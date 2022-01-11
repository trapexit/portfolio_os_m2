/*
 *	@(#) notsysrom.h 96/07/22 1.34
 *	Copyright 1994,1995,1996 The 3DO Company
 *
 * Macros for calling system ROM dipir functions from driver or device dipir.
 */

#if 0
/* Helper macros for calling routines */
/* Not in use now since everything is in sync */
/* But will be needed when additions are made after first release */
#define	DipirRoutinesVersionBefore(n) \
	(dipr->DipirRoutinesVersion < DIPIR_ROUTINES_VERSION_0+(n))
#endif

#ifdef DEBUG
#define	PRINTF(args)		(*dipr->dr_printf) args
#else
#define	PRINTF(args)
#endif

#ifdef BUILD_STRINGS
#define	EPRINTF(args)		(*dipr->dr_printf) args
#else
#define	EPRINTF(args)
#endif

#ifdef DO_TIME
#define	TIMESTAMP(s,v)		(*dipr->dr_TimeStamp)(s,(void*)(v))
#else
#define	TIMESTAMP(s,v)
#endif

#define	ResetTimer(tm)		(*dipr->dr_ResetTimer)(tm)
#define	ReadMilliTimer(tm)	(*dipr->dr_ReadMilliTimer)(tm)
#define	ReadMicroTimer(tm)	(*dipr->dr_ReadMicroTimer)(tm)
#define	ResetDevAndExit()	(*dipr->dr_ResetDevAndExit)()
#define	Reboot(addr,arg1,arg2,arg3) \
				(*dipr->dr_Reboot)(addr,arg1,arg2,arg3)
#define	HardBoot()		(*dipr->dr_HardBoot)()
#define	DisplayImage(image,pos,expand,a1,a2) \
				(*dipr->dr_DisplayImage)(image,pos,expand,a1,a2)
#define	AppExpectingDataDisc()	(*dipr->dr_AppExpectingDataDisc)()

/* In romtag.c */
#define	NextRomTag(fd,pos,rt)	(*dipr->dr_NextRomTag)(fd,pos,rt)
#define	FindRomTag(fd,subsys,type,pos,rt) \
				(*dipr->dr_FindRomTag)(fd,subsys,type,pos,rt)

/* In alloc.c */
#define	DipirAlloc(size,flags)	(*dipr->dr_DipirAlloc)(size,flags)
#define	DipirFree(ptr)		(*dipr->dr_DipirFree)(ptr)

/* In dbuffer.c */
#define	ReadDoubleBuffer(fd,startBlock,readSize,trailerSize,buf1,buf2,bufSize,Digest,digestArg,Final,finalArg) \
				(*dipr->dr_ReadDoubleBuffer)(fd,startBlock,readSize,trailerSize,buf1,buf2,bufSize,Digest,digestArg,Final,finalArg)
#define	ReadSigned(fd,offset,size,buffer,key) \
				(*dipr->dr_ReadSigned)(fd,offset,size,buffer,key)

/* In ntgdigest */
#define DipirInitDigest()	(*dipr->dr_DipirInitDigest)()
#define DipirUpdateDigest(buf,len) \
				(*dipr->dr_DipirUpdateDigest)(buf,len)
#define DipirFinalDigest()	(*dipr->dr_DipirFinalDigest)()

/* In rsadipir.c */
#define	RSAInit(indipir)	(*dipr->dr_RSAInit)(indipir)
#define	RSAFinalWithKey(key,sig,sigLen) \
				(*dipr->dr_RSAFinalWithKey)(key,sig,sigLen)
#define	RSAFinal(sig,key)	(*dipr->dr_RSAFinal)(sig,key)
#define	GenRSADigestInit(info)	(*dipr->dr_GenRSADigestInit)(info)
#define	GenRSADigestUpdate(info,buf,len) \
				(*dipr->dr_GenRSADigestUpdate)(info,buf,len)
#define	GenRSADigestFinal(info,sig,sigLen) \
				(*dipr->dr_GenRSADigestFinal)(info,sig,sigLen)

/* In swecc.c */
#define	SectorECC(buf)		(*dipr->dr_SectorECC)(buf)

/* In display.c */
#define	CanDisplay()		(*dipr->dr_CanDisplay)()
#define	DisplayIcon(icon,attr,attr2) \
				(*dipr->dr_DisplayIcon)(icon,attr,attr2)
#define	DisplayBanner(image,attr,attr2) \
				(*dipr->dr_DisplayBanner)(image,attr,attr2)
#define	FillRect(rect,color)	(*dipr->dr_FillRect)(rect,color)
#define	SetPixel(pos,color)	(*dipr->dr_SetPixel)(pos,color)
#define ReplaceIcon(icon,dev)	(*dipr->dr_ReplaceIcon)(icon,dev,newIcon,newSize)
#define InternalIcon(iconID,newIcon) \
				(*dipr->dr_InternalIcon)(iconID,newIcon,newSize)

/* In boot.c */
#define	InitBoot()		(*dipr->dr_InitBoot)()
#define	BootAppPrio()		(*dipr->dr_BootAppPrio)()
#define	SetBootApp(label,prio)	(*dipr->dr_SetBootApp)(label,prio)
#define	SetBootRomAppMedia(dev)	(*dipr->dr_SetBootRomAppMedia)(dev)
#define	BootRomAppMedia()	(*dipr->dr_BootRomAppMedia)()
#define	BootOSVersion(osflags)	(*dipr->dr_BootOSVersion)(osflags)
#define	BootOSAddr(osflags)	(*dipr->dr_BootOSAddr)(osflags)
#define	BootOSFlags(osflags)	(*dipr->dr_BootOSFlags)(osflags)
#define	WillBoot()		(*dipr->dr_WillBoot)()
#define	SetBootOS(label,version,addr,entry,osflags,kreserve,devicePerms,dipirDev,dddID,dipirID) \
				(*dipr->dr_SetBootOS)(label,version,addr,entry,osflags,kreserve,devicePerms,dipirDev,dddID,dipirID)
#define	SetBootVar(var,value)	(*dipr->dr_SetBootVar)(var,value)
#define	GetBootVar(var)		(*dipr->dr_GetBootVar)(var)

/* In rommisc.c */
#define	strncpyz(to,from,tolen,fromlen) \
				(*dipr->dr_strncpyz)(to,from,tolen,fromlen)
#define	strcmp(s1,s2)		(*dipr->dr_strcmp)(s1,s2)
#define	strncmp(s1,s2,n)	(*dipr->dr_strncmp)(s1,s2,n)
#define	strcat(a,b)		(*dipr->dr_strcat)(a,b)
#define	strcpy(a,b)		(*dipr->dr_strcpy)(a,b)
#define	strlen(a)		(*dipr->dr_strlen)(a)
#define	memcmp(s1,s2,n)		(*dipr->dr_memcmp)(s1,s2,n)
#define	memcpy(s1,s2,n)		(*dipr->dr_memcpy)(s1,s2,n)
#define	memset(s1,c,n)		(*dipr->dr_memset)(s1,c,n)
#define	strcmp(s1,s2)		(*dipr->dr_strcmp)(s1,s2)

/* In loader.c */
#define RelocateBinary(buffer, alloc, free, allocArg) \
				(*dipr->dr_RelocateBinary)(buffer, alloc, free, allocArg)
#define	CallBinary(entry,a1,a2,a3,a4,a5) \
				(*dipr->dr_CallBinary)(buffer, a1,a2,a3,a4,a5)
#define	Get3DOBinHeader(buffer)	(*dipr->dr_Get3DOBinHeader)(buffer)
#define	GetCurrentOSVersion()	(*dipr->dr_GetCurrentOSVersion)()

/* In chan.powerbus.c */
#define	PowerSlot(devType)	(*dipr->dr_PowerSlot)(devType)
#define	ReadPowerBusRegister(devType,regaddr) \
				(*dipr->dr_ReadPowerBusRegister)(devType,regaddr)
#define	WritePowerBusRegister(devType,regaddr,value) \
				(*dipr->dr_WritePowerBusRegister)(devType,regaddr,value)
#define	SetPowerBusBits(devType,regaddr,bits) \
				(*dipr->dr_SetPowerBusBits)(devType,regaddr,bits)
#define	ClearPowerBusBits(devType,regaddr,bits) \
				(*dipr->dr_ClearPowerBusBits)(devType,regaddr,bits)

/* In ddd.c */
#define	FindDDDFunction(ddd,cmd) (*dipr->dr_FindDDDFunction)(ddd,cmd)
#define	ReadBytes(fd,offset,size,buffer) \
				(*dipr->dr_ReadBytes)(fd,offset,size,buffer)
#define	ReadAsync(fd,blk,nblks,buffer) \
				(*dipr->dr_ReadAync)(fd,blk,nblks,buffer)
#define	WaitRead(fd,id)		(*dipr->dr_WaitRead)(fd,id)
#define	ReadSync(fd,blk,nblks,buffer) \
				(*dipr->dr_ReadSync)(fd,blk,nblks,buffer)

/* In tiny.c */
#define FindTinyRomTag(fd,type,pos,pResult) \
				(*dipr->dr_FindTinyRomTag)(fd,type,pos,pResult)
#define	DipirValidateTinyDevice(fd,DefaultName) \
				(*dipr->dr_DipirValidateTinyDevice)(fd,DefaultName)

/* Misc */
#define	FlushDCacheAll()	(*dipr->dr_FlushDCacheAll)()
#define	FlushDCache(addr,len)	(*dipr->dr_FlushDCache)(addr,len)
#define	InvalidateDCache(addr,len) \
				(*dipr->dr_InvalidateDCache)(addr,len)
#define	InvalidateICache()	(*dipr->dr_InvalidateICache)()
#define	ScaledRand(scale)	(*dipr->dr_ScaledRand)(scale)
#define	srand(seed)		(*dipr->dr_srand)(seed)
#define	ReadRomTagFile(fd,rt,flags) \
				(*dipr->dr_ReadRomTagFile)(fd,rt,flags)
#define	Delay(ms)		(*dipr->dr_Delay)(ms)
#define	SPutHex(value,buf,len)	(*dipr->dr_SPutHex)(value,buf,len)
