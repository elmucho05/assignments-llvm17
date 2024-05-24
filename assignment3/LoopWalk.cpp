#include "llvm/Transforms/Utils/LoopWalk.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/InstrTypes.h"
#include <llvm/IR/Dominators.h>
#include <llvm/IR/Constants.h>

using namespace llvm;

bool isLoopInvariant(Instruction &Inst, Loop &L) {
  for(auto *opIter = Inst.op_begin(); opIter != Inst.op_end(); ++opIter){
      Value *op = opIter->get();

    if(isa<PHINode>(Inst) || isa<BranchInst>(Inst))
      return false;

    if (Instruction *arg = dyn_cast<Instruction>(op)) {
      if (L.contains(arg)){
        if(!isLoopInvariant(*arg,L))
          return false;
      }
    }
  }
  return true;
}

bool dominatesExits(Instruction *inst, DominatorTree &DT, Loop &L) {

  SmallVector<BasicBlock*> uscite;

  for (auto *block : L.getBlocks()) {
    if (block != L.getHeader() && L.isLoopExiting(block))
      uscite.push_back(block);
  }

  for (auto *exit : uscite) {
    if (!DT.dominates(inst->getParent(), exit))
      return false;
  }

return true;
}

bool dominatesUseBlocks(DominatorTree &DT, Instruction *inst){
  for (auto Iter = inst->user_begin(); Iter != inst->user_end(); ++Iter) {
    if (!DT.dominates(inst, dyn_cast<Instruction>(*Iter))) {
      return false;
    }
  }
  return true;
}


PreservedAnalyses LoopWalk::run(Loop &L, LoopAnalysisManager &LAM,
            LoopStandardAnalysisResults &LAR, LPMUpdater &LU) {
  
  outs() << "\n********* Inizio del Loop ********...\n"; 

  DominatorTree &DT = LAR.DT;
  SmallVector<Instruction*> invariantInstructions;
  SmallVector<Instruction*> movableInstructions;

  if (L.isLoopSimplifyForm()){
    outs() << "\nLoop in forma normalizzata\n";

    int i = 1; //counter del bb, solo per controllo output
    for (Loop::block_iterator BI = L.block_begin(); BI != L.block_end(); ++BI){
      BasicBlock *BB = *BI;
      outs() << "\nBasic block n. " << i << ": " << *BB << "\n";
      i++;

      outs() << "Scrorro le istruzioni del BB: \n";
      for(auto InstIter = BB->begin(); InstIter != BB->end(); ++InstIter){
        Instruction &Inst = *InstIter;

        if(isLoopInvariant(Inst, L)){
          outs() << Inst << "E' loop invariant " << "\n";
          invariantInstructions.push_back(&Inst);
        }
        else
          outs() << "Non è loop invariant" <<"\n";
      }
    }

    for (auto *inst : invariantInstructions)
    {
      if (dominatesExits(inst,DT ,L ) && dominatesUseBlocks(DT, inst))
        movableInstructions.push_back(inst);
    }

    BasicBlock *preHeader = L.getLoopPreheader();
    for (auto elem : movableInstructions)
    {
      outs()<<"Trovata istruzione movable: "<<*elem<<"\n";
      elem->moveBefore(&preHeader->back());
    }

  }

   return PreservedAnalyses::all();
}
