#ifndef __MOCOMP_H__
#define __MOCOMP_H__

#include "common.h"
#include "mode_pred.h"

void MotionCompensateTB(frame *this_frame, frame *ref, int org_x, int org_y,
                        int mvx, int mvy);

void MotionCompensateMB(frame *this_frame, frame *ref, mode_pred_info *mpi,
                        int org_x, int org_y);

#endif /*__MOCOMP_H__*/
