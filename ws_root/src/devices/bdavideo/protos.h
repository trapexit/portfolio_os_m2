/* bdavdl.c */
Err update(struct Projector *p, int32 reason, void *ob);
Err processtransitions(struct Transition *curtrans, int32 transtop, struct PTState *pts);
Err cvu_16_32(struct View *v, struct PTState *pts);
Err cvu_16_32_lace(struct View *v, struct PTState *pts);
Err cvu_blank(struct View *v, struct PTState *pts);
Err mvu_16_32_dual(struct View *v, struct Transition *t, int32 field, int32 delta);
/* bdavideo.c */
int main(void);
/* db_ntsc.c */
struct ViewTypeInfo *nextvti_ntsc(struct Projector *p, struct ViewTypeInfo *vti);
/* db_ntsc_nl.c */
struct ViewTypeInfo *nextvti_ntsc_nl(struct Projector *p, struct ViewTypeInfo *vti);
/* db_pal.c */
struct ViewTypeInfo *nextvti_pal(struct Projector *p, struct ViewTypeInfo *vti);
/* db_pal_nl.c */
struct ViewTypeInfo *nextvti_pal_nl(struct Projector *p, struct ViewTypeInfo *vti);
/* firq.c */
int32 dispmodeFIRQ(void);
int32 beamFIRQ(void);
void dispatchlinesignal(struct PerProj *pp, struct lineintr *li);
Err installlineintr(struct PerProj *pp, struct lineintr *node, int field);
void removelineintr(struct PerProj *pp, struct lineintr *li);
int32 disp2hw(struct PerProj *pp, int32 val);
Err inittrip(void);
void trip(uint32 key, uint32 val);
/* stompomatic.c */
Err stomplists(struct Projector *p, struct View *v, struct View *ov);
