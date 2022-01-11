
/******************************************************************************
**
**  @(#) matrix.c 96/07/09 1.2
**
**  Calculate transformation matrix.
**
******************************************************************************/

#include <kernel/types.h>
#include <graphics/frame2d/frame2d.h>
#include <graphics/font.h>
#include <string.h>
#include <math.h>

void MatrixMult(FontMatrix fm, FontMatrix tran)
{
    /* fm = fm * tran */
    FontMatrix src;

    memcpy(src, fm, sizeof(FontMatrix));

    fm[0][0] = ((src[0][0] * tran[0][0]) + (src[0][1] * tran[1][0]));
    fm[1][0] = ((src[1][0] * tran[0][0]) + (src[1][1] * tran[1][0]));
    fm[2][0] = ((src[2][0] * tran[0][0]) + (src[2][1] * tran[1][0]) + tran[2][0]);
    fm[0][1] = ((src[0][0] * tran[0][1]) + (src[0][1] * tran[1][1]));
    fm[1][1] = ((src[1][0] * tran[0][1]) + (src[1][1] * tran[1][1]));
    fm[2][1] = ((src[2][0] * tran[0][1]) + (src[2][1] * tran[1][1]) + tran[2][1]);
}

void MatrixRotate(FontMatrix fm, gfloat angle, Point2 *center)
{
    FontMatrix tran;
    gfloat s, c;    /* sine and cosine */

    /* The translation matrix to rotate about an arbitrary point is:
     *
     *                  cos                                           sin
     *                  - sin                                         cos
     * (1 - cos)*center.x + sin * center.y    (1 - cos)*center.y - sin * center.x
     */

    s = sinf(angle * (PI / 180.0));
    c = cosf(angle * (PI / 180.0));

    tran[0][0] = c;    tran[0][1] = s;
    tran[1][0] = -s;  tran[1][1] = c;
    tran[2][0] = (((1 - c) * center->x) + (s * center->y));
    tran[2][1] = (((1 - c) * center->y) - (s * center->x));

    /* Multiply this by the current transformation matrix */
    MatrixMult(fm, tran);
}

void MatrixScale(FontMatrix fm, gfloat scale, Point2 *center)
{
    FontMatrix tran;

    /* The translation matrix to scale about an arbitrary point is:
     *
     *             scale                           0
     *               0                            scale
     *  (1 - scale) * center.x    (1 - scale) * center.y
     */

    tran[0][0] = scale;    tran[0][1] = 0;
    tran[1][0] = 0;          tran[1][1] = scale;
    tran[2][0] = ((1 - scale) * center->x);
    tran[2][1] = ((1 - scale) * center->y);

    /* Multiply this by the current transformation matrix */
    MatrixMult(fm, tran);
}

void MatrixTranslate(FontMatrix fm, gfloat dx, gfloat dy)
{
    FontMatrix tran;

    /* The translation matrix to translate is:
     *
     *   1    0
     *   0    1
     *  dx   dy
     */

    tran[0][0] = 1.0;    tran[0][1] = 0.0;
    tran[1][0] = 0.0;    tran[1][1] = 1.0;
    tran[2][0] = dx;     tran[2][1] = dy;

    /* Multiply this by the current transformation matrix */
    MatrixMult(fm, tran);
}
