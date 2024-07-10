#ifndef LLVM_TRANSFORMS_LPFUSION_H
#define LLVM_TRANSFORMS_LPFUSION_H

#include "llvm/IR/PassManager.h"
#include <llvm/IR/Constants.h>

namespace llvm {
    class LpFusion : public PassInfoMixin<LpFusion> {
    public:
        PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);
    };
}

#endif