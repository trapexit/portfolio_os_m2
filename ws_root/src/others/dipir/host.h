#ifndef _HOST_H

typedef void *		RefToken;
#define	NULL_TOKEN	((RefToken)0)

extern void InitHost(DipirTemp *dt);
extern void SendHostCmd(DipirHWResource *dev, HostFSReq *cmd);
extern void GetHostReply(DipirHWResource *dev, HostFSReply *reply);

#endif /* _HOST_H */

