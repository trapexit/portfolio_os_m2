/* @(#) generic.c 95/10/20 1.6 */

/* A simple program which talks to the Event Broker and
 * issues a generic command for a specific pod.
 */

#include <kernel/types.h>
#include <kernel/item.h>
#include <kernel/task.h>
#include <kernel/mem.h>
#include <kernel/nodes.h>
#include <kernel/debug.h>
#include <kernel/list.h>
#include <kernel/device.h>
#include <kernel/driver.h>
#include <kernel/msgport.h>
#include <kernel/kernelnodes.h>
#include <kernel/io.h>
#include <kernel/operror.h>
#include <misc/event.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define messageSize 2048
#define numGenerics 6


int32 main(int argc, char **argv)
{
  Item msgPortItem;
  Item brokerPortItem;
  Item msgItem, eventItem;
  uint8 pod, group, command, val;
  struct {
    PodData  pd;
    uint8    morevals[32];
  } cmd;
  int32 sent;
  Message *event;
  int i;
  char *s;

  TagArg msgTags[3];

  if (argc < 4) {
  usage: printf("Usage: generic podnumber group command [val val...]\n");
    return -1;
  }

  pod = (uint8) strtol(argv[1], &s, 0);
  if (s == argv[1]) goto usage;

  group = (uint8) strtol(argv[2], &s, 0);
  if (s == argv[2]) goto usage;

  command = (uint8) strtol(argv[3], &s, 0);
  if (s == argv[3]) goto usage;

  cmd.pd.pd_Header.ebh_Flavor = EB_IssuePodCmd;
  cmd.pd.pd_PodNumber = pod;
  cmd.pd.pd_WaitFlag = 1;
  cmd.pd.pd_Data[0] = group;
  cmd.pd.pd_Data[1] = command;
  cmd.pd.pd_DataByteCount = 2;

  for (i = 0; i < argc - 4 ; i++) {
    val = (uint8) strtol(argv[i+4], &s, 0);
    if (s == argv[i+4]) goto usage;
    cmd.pd.pd_Data[i+2] = val;
    cmd.pd.pd_DataByteCount ++;
  }

  brokerPortItem = FindNamedItem(MKNODEID(KERNELNODE,MSGPORTNODE),
				 EventPortName);

  if (brokerPortItem < 0) {
    printf("Can't find Event Broker port: ");
    PrintfSysErr(brokerPortItem);
    return 0;
  }
  
  msgPortItem = CreateMsgPort(argv[0], 0, 0);

  if (msgPortItem < 0) {
    printf("Cannot create event-listener port: ");
    PrintfSysErr(msgPortItem);
    return 0;
  }

  msgTags[0].ta_Tag = CREATEMSG_TAG_REPLYPORT;
  msgTags[0].ta_Arg = (void *) msgPortItem;
  msgTags[1].ta_Tag = CREATEMSG_TAG_DATA_SIZE;
  msgTags[1].ta_Arg = (void *) messageSize;
  msgTags[2].ta_Tag = TAG_END;

  msgItem = CreateItem(MKNODEID(KERNELNODE,MESSAGENODE), msgTags);

  if (msgItem < 0) {
    printf("Cannot create message: ");
    PrintfSysErr(msgItem);
    return 0;
  }

  sent = SendMsg(brokerPortItem, msgItem, &cmd, sizeof cmd);

  if (sent < 0) {
    printf("Error sending issue-pod-command message: ");
    PrintfSysErr(sent);
    return 0;
  }

  eventItem = WaitPort(msgPortItem, msgItem);

  if (eventItem < 0) {
    printf("Error getting issue-pod-command reply: ");
    PrintfSysErr(eventItem);
    return 0;
  }

  event = (Message *) LookupItem(eventItem);

  if ((int32) event->msg_Result < 0) {
    printf("generic says broker bounced the command: ");
    PrintfSysErr((int32) event->msg_Result);
    return (int32) event->msg_Result;
  }

  return 0;
}
