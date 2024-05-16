#include "llvm/Transforms/Utils/LoopWalk.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/InstrTypes.h"
#include <llvm/IR/Dominators.h>
#include <llvm/IR/Constants.h>

using namespace llvm;

bool isLoopInvariantValue(Value*, Loop &);
bool isLoopInvariantInstruction(Instruction &I, Loop &L);

PreservedAnalyses LoopWalk::run(Loop &L, LoopAnalysisManager &LAM,
            LoopStandardAnalysisResults &LAR, LPMUpdater &LU) {
  outs() << "\nLOOPPASS INIZIATO...\n"; 

//DA METTERE A POSTO GLI INPUT aRGUMENTS e le istruzion di phi 
  //TOGLI DAL FILE .ll le direttive optnone
  //Loop.getHeader.getParent()
  if (!L.isLoopSimplifyForm())
  {
    outs() << "Il Loop non è in forma normale\n";
    return PreservedAnalyses::all();
  }
      // BasicBlock* preheder = L->getLoopPreheader();
      // if(preheder){
      //   outs() << "Il loop ha un preheader:\n\nIstruzioni Preheader\n";
      // }
      // else
      //   return false; 

  // outs() << "Il Loop è in forma normale, procediamo \n";
  // BasicBlock *head = L.getHeader();
  // Function *F = head->getParent();

  // //STAMPO IL CFG
  // outs() << "\n*********IL CFG della funzione : ****************\n";
  // for( auto iter = F->begin(); iter != F->end(); ++iter)
  // {
  //   BasicBlock &BB = *iter;
  //   outs() << BB << "\n"; 
  // }

  // outs() << "\n****************************************\n";

  //POI STAMPO IL LOOP 
  outs() << "\n*********IL LOOP: ***********************\n";
  for(auto iterloop = L.block_begin(); iterloop != L.block_end(); iterloop++)
  {
    BasicBlock *BB = *iterloop;
    for(auto iterInstr=BB->begin(); iterInstr!=BB->end(); iterInstr++)
    {
      Instruction &i = *iterInstr;
      outs() << i << "\n";
      if(isLoopInvariantInstruction(i, L))
        outs() << "L'istruzione" << i << "è loop invariant";
      else
        outs() << "L'istruzione" << i << "NON è loop invariant";

      outs() << "***************************" << "\n";
    }
  }

  // for(auto BI = L.block_begin(); BI != L.block_end(); ++BI)
  // {
  //   BasicBlock *BB = *BI;
  //   for (auto &i : *BB){
  //      if(checkInvariance(&L, &i))
  //       {
  //         outs() << i << " è loop-invariant\n";
          
  //       }
  //   }
  // }



   return PreservedAnalyses::all();
}


bool isLoopInvariantInstruction(Instruction &I, Loop &L) {
    for (auto *iterOperand=I.op_begin();iterOperand!=I.op_end(); iterOperand++)
    {
      Value *Operand = *iterOperand;
      if(isLoopInvariantValue(Operand, L))
        return true;
    }
    return false;
    
    // if(dyn_cast<BinaryOperator>(I))
    // {
    //   Value* op_right = I->getOperand(1);
    //   Value* op_left = I->getOperand(0);
    //   return _isLoopInvariant(L, op_right) && _isLoopInvariant(L, op_left);
    // }
    // return false;
  }

bool isLoopInvariantValue(Value* V, Loop &L) {
  if(Instruction *inst = dyn_cast<Instruction>(V))
  {
    outs() << *inst << "\n";
    if(!L.contains(inst))
    {
      outs() << "L'istruzione sta fuori dal loop\n";
      return true;
    }else{
      outs() << "altra instructions \n";
      if(isLoopInvariantInstruction(*inst, L))
        return true;
      else
        return false;
    }
  }else if (Constant *C = dyn_cast<Constant>(V))
  {
    outs() << "costante trovata \n";
    return true;
  }

    // if(dyn_cast<Constant>(V) != nullptr)
    //   return true;

    // Instruction* inst = dyn_cast<Instruction>(V);
    // if(inst)
    //   if(!(L->contains(inst)))
    //     return true;
    //   else
    //     return checkInvariance(L, inst);


    // return true;
  }