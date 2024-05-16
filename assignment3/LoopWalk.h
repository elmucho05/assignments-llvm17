#ifndef LLVM_TRANSFORMS_LOOPWALK_H
#define LLVM_TRANSFORMS_LOOPWALK_H

#include "llvm/IR/PassManager.h"
#include "llvm/Transforms/Scalar/LoopPassManager.h"


namespace llvm {

class LoopWalk : public PassInfoMixin<LoopWalk> {
public:
        PreservedAnalyses run(Loop &L, LoopAnalysisManager &LAM,
            LoopStandardAnalysisResults &LAR, LPMUpdater &LU);
};

} // namespace llvm

#endif // LLVM_TRANSF ORMS_LOCALOPTS_H



