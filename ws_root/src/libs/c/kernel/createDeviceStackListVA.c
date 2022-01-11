/* @(#) createDeviceStackListVA.c 95/12/05 1.1 */

#include <kernel/types.h>
#include <varargs.h>
#include <kernel/mem.h>
#include <kernel/operror.h>
#include <kernel/list.h>
#include <kernel/device.h>

Err
CreateDeviceStackListVA(List **pList, ...)
{
	va_list ap;
	uint32 numQueries;
	char *name;
	DDFOp op;
	uint32 flags;
	uint32 numValues;
	DDFQuery *query;
	DDFQuery *qp;
	Err ret;

	/* First, count the number of queries. */
	numQueries = 0;
	va_start(ap, pList);
	for (;;)
	{
		name = va_arg(ap, char *);
		if (name == NULL)
			break;
		op = va_arg(ap, DDFOp);
		flags = va_arg(ap, uint32);
		numValues = va_arg(ap, uint32);
		if (op == DDF_DEFINED || op == DDF_NOTDEFINED)
			numQueries++;
		while (numValues-- > 0)
		{
			if (flags & DDF_INT)
				(void) va_arg(ap, int32);
			else if (flags & DDF_STRING)
				(void) va_arg(ap, char *);
			else
				return MAKEKERR(ER_SEVER,ER_C_STND,ER_ParamError);
			numQueries++;
		}
	}
	va_end(ap);

	/* Allocate space for the query array. */
	query = AllocMem((numQueries + 1) * sizeof(DDFQuery), MEMTYPE_NORMAL);
	if (query == NULL)
		return NOMEM;
	qp = query;

	/* Now run thru the args again and construct the query array. */
	va_start(ap, pList);
	for (;;)
	{
		name = va_arg(ap, char *);
		if (name == NULL)
			break;
		op = va_arg(ap, DDFOp);
		flags = va_arg(ap, uint32);
		numValues = va_arg(ap, uint32);
		if (op == DDF_DEFINED || op == DDF_NOTDEFINED)
		{
			qp->ddfq_Name = name;
			qp->ddfq_Operator = op;
			qp->ddfq_Flags = 0;
			qp++;
		}
		while (numValues-- > 0)
		{
			qp->ddfq_Name = name;
			qp->ddfq_Operator = op;
			qp->ddfq_Flags = flags;
			if (flags & DDF_STRING)
				qp->ddfq_Value.ddfq_String = va_arg(ap, char *);
			else 
				qp->ddfq_Value.ddfq_Int = va_arg(ap, int32);
			qp++;
		}
	}
	qp->ddfq_Name = NULL;
	va_end(ap);

	ret = CreateDeviceStackList(pList, query);
	FreeMem(query, (numQueries + 1) * sizeof(DDFQuery));
	return ret;
}
