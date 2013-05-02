#ifndef INC_ACTION_GRID_H
#define INC_ACTION_GRID_H
#include "Action.h"
#include "DataSet_GridFlt.h"
#include "GridAction.h"
class Action_Grid : public Action, private GridAction {
  public:
    Action_Grid();
    static DispatchObject* Alloc() { return (DispatchObject*)new Action_Grid(); }
    static void Help();
  private:
    Action::RetType Init(ArgList&, TopologyList*, FrameList*, DataSetList*,
                          DataFileList*, int);
    Action::RetType Setup(Topology*, Topology**);
    Action::RetType DoAction(int, Frame*, Frame**);
    void Print();

    void PrintPDB(double);

    double max_;
    double madura_;
    double smooth_;
    bool invert_;
    AtomMask mask_;
    std::string pdbname_;
    DataSet_GridFlt* grid_;
};
#endif
