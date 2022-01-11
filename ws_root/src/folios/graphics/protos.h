/* bitmap.c */
Err modifybitmap(struct Bitmap *clientbm, struct TagArg *args);
Item createbitmap(struct Bitmap *bm, struct TagArg *ta);
Err deletebitmap(struct Bitmap *bm);
Item findbitmap(struct TagArg *ta);
Item openbitmap(struct Bitmap *bm, struct TagArg *ta);
Err closebitmap(struct Bitmap *bm, struct Task *t);
Err InternalInvalidateBitmap(struct Bitmap *bm);
Err bmsize(struct Bitmap *bm);
Err lrformsize(struct Bitmap *bm);
void *PixelAddress(Item bmitem, uint32 x, uint32 y);
void *InternalPixelAddr(struct Bitmap *bm, uint32 x, uint32 y);
void *simple_addr(struct Bitmap *bm, uint32 x, uint32 y);
void *lrform_addr(struct Bitmap *bm, uint32 x, uint32 y);
void memlockhandler(struct Task *t, const void *addr, uint32 length);
/* compiler.c */
Err SuperInternalCompile(struct ViewList *vl, int32 nviews, struct Transition **tr_retn, int32 tr_size, int32 *starty, uint32 clearflags);
int32 ordinateviews(struct ViewList *vl, struct ViewEdge **edges, int32 ordinal, int32 yref, uint32 clearflags);
Err SuperInternalMarkView(struct View *v, int32 top, int32 bot);
/* context.c */
void AbortIOReqsUsingItem(Item itemNumber, uint32 itemType);
/* database.c */
struct ViewTypeInfo *NextViewTypeInfo(Item pi, const struct ViewTypeInfo *vti);
Err lookupviewtype(int32 vid, struct ViewTypeInfo **vtiptr, struct Projector **pptr);
/* dbase.bm.c */
/* gfxfolio.c */
Err WaitForProjectorLoader(void);
int main(int32 op, Item it);
Item SuperModifyGraphicsItem(Item it, struct TagArg *ta);
/* init.c */
Err initgbase(void);
/* projector.c */
Err modifyprojector(struct Projector *clientp, struct TagArg *args);
Item createprojector(struct Projector *p, struct TagArg *ta);
Err deleteprojector(struct Projector *p);
Item findprojector(struct TagArg *ta);
Item openprojector(struct Projector *p, const struct TagArg *ta);
Err closeprojector(struct Projector *p, struct Task *t);
Err setownerprojector(struct Projector *n, Item newOwner, struct Task *task);
Item NextProjector(Item pi);
Item SuperActivateProjector(Item pi);
Err SuperDeactivateProjector(Item pi);
Err SuperSetDefaultProjector(Item pi);
/* query.c */
Err QueryGraphics(const struct TagArg *ta);
/* util.c */
Err SuperLockDisplay(Item display);
Err SuperUnlockDisplay(Item display);
Err unimplemented(void);
/* view.c */
Item createview(struct View *v, struct TagArg *ta);
Err modifyview(struct View *cview, struct TagArg *ta);
Err deleteview(struct View *v);
Item findview(struct TagArg *ta);
Item openview(struct View *v, struct TagArg *ta);
Err closeview(struct View *v, struct Task *t);
Err SuperAddViewToViewList(Item viewItem, Item viewListItem);
Err backpropogate(struct ViewList *vl, struct Projector *p);
Err SuperRemoveView(Item viewItem);
Err removeview(struct View *v);
Err SuperOrderViews(Item victim, int32 op, Item radix);
void SuperInternalSetRendCallBack(register struct View *v, void (*vector)(struct View *, void *), void *ptr);
void SuperInternalSetDispCallBack(register struct View *v, void (*vector)(struct View *, void *), void *ptr);
/* viewlist.c */
Item createviewlist(struct ViewList *vl, struct TagArg *ta);
Item modifyviewlist(struct ViewList *clientvl, struct TagArg *ta);
Err deleteviewlist(struct ViewList *vl);
Item findviewlist(struct TagArg *ta);
Item openviewlist(struct ViewList *vl, struct TagArg *ta);
Err closeviewlist(struct ViewList *v, struct Task *t);
