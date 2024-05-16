#include "llvm/Transforms/Utils/LoopWalk.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/InstrTypes.h"
#include <llvm/IR/Dominators.h>
#include <llvm/IR/Constants.h>

using namespace llvm;

bool isLoopInvariant(Instruction &Inst, Loop &L) {
    for(auto *opIter = Inst.op_begin(); opIter != Inst.op_end(); ++opIter){
        Value *op = opIter->get();
      //se non e' un PHINode
      if(isa<PHINode>(Inst) || isa<BranchInst>(Inst))
        return false;
      //Se non e' una const
      if (Instruction *arg = dyn_cast<Instruction>(op)) {
        if (L.contains(arg)){// dichiarato dentro loop
          if(!isLoopInvariant(*arg,L))
            return false;
        }
      }
    }
    return true;
  }

  bool dominatesExits(Instruction *inst, DominatorTree &DT, Loop &L) {

    SmallVector<BasicBlock*> exits;

    for (auto *block : L.getBlocks()) {
      if (block != L.getHeader() && L.isLoopExiting(block))
        exits.push_back(block);
    }

    for (auto *exit : exits) {
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
  outs() << "\nLOOPPASS INIZIATO...\n"; 

  DominatorTree &DT = LAR.DT;
  SmallVector<Instruction*> Invariants;
  SmallVector<Instruction*> Movable;

  // verificare la forma normalizzata
  if (L.isLoopSimplifyForm()){
    outs() << "\nLoop in forma normalizzata\n";

    // itero sui basic blocks del loop
    int i = 1;
    for (Loop::block_iterator BI = L.block_begin(); BI != L.block_end(); ++BI){
      BasicBlock *BB = *BI;
      outs() << "\nBasic block n. " << i << ": " << *BB << "\n";
      i++;

      outs() << "Scrorrendo le istruzioni del BB: \n";
      for(auto InstIter = BB->begin(); InstIter != BB->end(); ++InstIter){
        Instruction &Inst = *InstIter;

        if(isLoopInvariant(Inst, L)){
          outs() << Inst << "E' loop invariant " << "\n";
          Invariants.push_back(&Inst);
        }
        else
          outs() << "Non è loop invariant" <<"\n";
      }
    }

    for (auto *inst : Invariants)
    {
      if (dominatesExits(inst,DT ,L ) && dominatesUseBlocks(DT, inst))
        Movable.push_back(inst);
    }

    BasicBlock *preHeader = L.getLoopPreheader();
    for (auto elem : Movable)
    {
      outs()<<"Trovata istruzione movable: "<<*elem<<"\n";
      elem->moveBefore(&preHeader->back());
    }

  }

   return PreservedAnalyses::all();
}
