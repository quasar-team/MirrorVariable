# MirrorVariable
MirrorVariables, as the name suggests, are variablesthat are precise mirrors of some other variable in the address space. Consider an address space containing variable root.x.y.z.Original, then this optional module can be used to create a mirror variable root.p.q.r.Mirror. Updates to the value (here value being taken to mean value + status + timestamps) of root.x.y.z.Original will be reflected in the value of root.p.q.r.Mirror. Furthermore, if a value is written to root.p.q.r.Mirror, this write will update root.x.y.z.Original.

TODO: API documentation, basically something like
    #include "MirrorVariableEngine.h"
    MirrorVariable::MirrorVariableEngine::instantiateMirrorVariable(AddressSpace::ASNodeManager* nm, 
      SomeType& originalNode, 
      SomeType& mirrorVariableParent, 
      const std::string& mirrorVariableName) 

