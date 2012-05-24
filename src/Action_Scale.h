#ifndef INC_ACTION_SCALE_H
#define INC_ACTION_SCALE_H
#include "Action.h"
class Action_Scale : public Action {
  public:
    Action_Scale();
  private:
    int init();
    int setup();
    int action();

    AtomMask mask_;
    double sx_;
    double sy_;
    double sz_;
};
#endif