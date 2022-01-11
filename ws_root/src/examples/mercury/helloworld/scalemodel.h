/* @(#) scalemodel.h 96/05/01 1.1 */

void Model_Scale(Pod* pod, float scale);
void BBox_LocalToWorld(const BBox* local, const Matrix* mtx, BBox* world);
void Model_ComputeTotalBoundingBox(const Pod* firstPod, int32 podCount, BBox* bbox);
