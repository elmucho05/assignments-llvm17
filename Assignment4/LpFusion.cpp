#include "llvm/Transforms/Utils/LpFusion.h"
#include "llvm/IR/Dominators.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/DependenceAnalysis.h"
#include "llvm/IR/TypedPointerType.h"

void foundPair(llvm::Loop* &L1, llvm::Loop* &L2, std::set<std::pair<llvm::Loop*, llvm::Loop*>> &set) {
  //L1->print(llvm::outs());
  //L2->print(llvm::outs());
  set.insert(std::make_pair(L1, L2));
}


void findAdjacentLoops(std::set<std::pair<llvm::Loop*, llvm::Loop*>> &adjacentLoops, llvm::LoopInfo &LI) {
  for (auto *L1 : LI) {
    for (auto *L2: LI) {
      if (L1->isGuarded() && L2->isGuarded()) {
        if (L1->getExitBlock()->getSingleSuccessor() == L2->getLoopGuardBranch()->getParent()) {
          llvm::outs() << "Trovata coppia di loop guarded adiacenti!\n";
          foundPair(L1, L2, adjacentLoops);
        }
      } else {
        if (L1->getExitBlock() == L2->getLoopPreheader()) {
          llvm::outs() << "Trovata coppia di loop unguarded adiacenti!\n";
          foundPair(L1, L2, adjacentLoops);
        }
      }}
    }
}


 
bool checkEquivalence(std::pair<llvm::Loop*, llvm::Loop*> loop, llvm::DominatorTree &DT, llvm::PostDominatorTree &PDT) {
 
  if (loop.first->isGuarded()) 
    if (auto FC0CmpInst = llvm::dyn_cast<llvm::Instruction>(loop.first->getLoopGuardBranch()->getCondition()))
        if (auto FC1CmpInst = llvm::dyn_cast<llvm::Instruction>(loop.second->getLoopGuardBranch()->getCondition()))
          if (!FC0CmpInst->isIdenticalTo(FC1CmpInst))
            return 0;

  if(loop.first->isGuarded()){
    if(DT.dominates(loop.first->getLoopGuardBranch()->getParent(), loop.second->getLoopGuardBranch()->getParent()) && PDT.dominates(loop.second->getLoopGuardBranch()->getParent(), loop.first->getLoopGuardBranch()->getParent())){
      llvm::outs() << "\nLoops are control flow equivalent\n";
      return 1;
    }
  } else {
    if (DT.dominates(loop.first->getHeader(), loop.second->getHeader()) && PDT.dominates(loop.second->getHeader(), loop.first->getHeader())) {
      llvm::outs() << "\nLoops are control flow equivalent\n";
      return 1;
    }
  }
  return 0;
}

 
bool checkSameTripCount(std::pair<llvm::Loop*, llvm::Loop*> loop, llvm::ScalarEvolution &SE) {
  auto l1Backedges = SE.getBackedgeTakenCount(loop.first);
  auto l2Backedges = SE.getBackedgeTakenCount(loop.second);

  if (l1Backedges->getSCEVType() == llvm::SCEVCouldNotCompute().getSCEVType() || l2Backedges->getSCEVType() == llvm::SCEVCouldNotCompute().getSCEVType()) return 0;

  if (l1Backedges == l2Backedges) {
    llvm::outs() << "\nLoops have the same backedge taken count\n";
    return 1;
  }
  return 0;
}


 
bool checkNegativeDependencies(std::pair<llvm::Loop*, llvm::Loop*> loop) {

 
  std::set<llvm::Instruction*> dependantInstructions {};

  for (auto &BB : loop.first->getBlocks()) {
    for (llvm::Instruction &I : *BB) {
 
      if (I.getOpcode() == llvm::Instruction::GetElementPtr){
        
 

        // Check if pointer is used in the second loop
        for (auto &use : I.getOperand(0)->uses()) {
          if (auto inst = llvm::dyn_cast<llvm::Instruction>(use.getUser())) {
            if (loop.second->contains(inst)) {
              if (auto sext = llvm::dyn_cast<llvm::Instruction>(inst->getOperand(1))) {
                
                if (auto phyInstruction = llvm::dyn_cast<llvm::Instruction>(sext->getOperand(0))) {
                  switch(phyInstruction->getOpcode()) {
                    case llvm::Instruction::PHI:
                    case llvm::Instruction::Sub: 
                      break;
                    default:
                      dependantInstructions.insert(phyInstruction);  
                  }
                }
              }
            }
          }
        }
    }}
  }

 
  if (!dependantInstructions.empty()) {
    llvm::outs() << "\n\nThe loop cannot be fused because the following instructions violates negative depencence distance:\n";
    for (auto inst : dependantInstructions) {
      llvm::outs() << "Instruction: " << *inst << "\n";
    }
    return 0;
  }
  return 1;
}

 
void loopFusion(llvm::Loop* &L1, llvm::Loop* &L2){
  
  auto firstLoopIV = L1->getCanonicalInductionVariable();
  auto secondLoopIV = L2->getCanonicalInductionVariable();

 
  secondLoopIV->replaceAllUsesWith(firstLoopIV);

 
  auto header1 = L1->getHeader();
  auto header2 = L2->getHeader();
  auto latch1 = L1->getLoopLatch();
  auto latch2 = L2->getLoopLatch();
  auto exit = L2->getUniqueExitBlock();

  if (!L1->isGuarded()) {
 
    llvm::BasicBlock* lastL1BodyBB = L1->getBlocks().drop_back(1).back();
    lastL1BodyBB->getTerminator()->setSuccessor(0, L2->getBlocks().drop_front(1).drop_back(1).front());
    L2->getBlocks().drop_front(1).drop_back(1).back()->getTerminator()->setSuccessor(0, latch1);
    llvm::BranchInst::Create(latch2, header2->getTerminator());
    header2->getTerminator()->eraseFromParent();

    llvm::BranchInst::Create(L1->getBlocks().drop_front(1).front(), exit, header1->back().getOperand(0), header1->getTerminator());
    header1->getTerminator()->eraseFromParent();
  } else {

    auto guard1 = L1->getLoopGuardBranch()->getParent();
    auto guard2 = L2->getLoopGuardBranch()->getParent();

    llvm::BranchInst::Create(L1->getLoopPreheader(), exit, guard1->back().getOperand(0), guard1->getTerminator());
    guard1->getTerminator()->eraseFromParent();
    llvm::BranchInst::Create(L1->getBlocks().front(), exit, latch1->back().getOperand(0), latch1->getTerminator());
    latch1->getTerminator()->eraseFromParent();
    L1->getBlocks().drop_back(1).back()->getTerminator()->setSuccessor(0, L2->getBlocks().front());
    L2->getBlocks().drop_back(1).back()->getTerminator()->setSuccessor(0, latch1);
    header2->front().eraseFromParent();
  }
}

llvm::PreservedAnalyses llvm::LpFusion::run(Function &F, FunctionAnalysisManager &AM) {

  llvm::LoopInfo &LI = AM.getResult<llvm::LoopAnalysis>(F);
  llvm::DominatorTree &DT = AM.getResult<DominatorTreeAnalysis>(F);
  llvm::PostDominatorTree &PDT = AM.getResult<PostDominatorTreeAnalysis>(F);
  llvm::ScalarEvolution &SE = AM.getResult<ScalarEvolutionAnalysis>(F);


  //llvm::DependenceInfo &DI = AM.getResult<llvm::DependenceAnalysis>(F);
  

  std::set<std::pair<llvm::Loop*, llvm::Loop*>> adjacentLoops {};
  
  findAdjacentLoops(adjacentLoops, LI);

  bool modified = 0;

  
  for (std::pair<llvm::Loop*, llvm::Loop*> loop : adjacentLoops) {
    if (!checkEquivalence(loop, DT, PDT)) continue;
    if (!checkSameTripCount(loop, SE)) continue;
    if (!checkNegativeDependencies(loop)) continue;
    

    llvm::outs() << "\nLoops can be fused\n";
    loopFusion(loop.first, loop.second);

    /* llvm::outs() << "Function after fusion:\n";
    for (auto &BB : F) {
      llvm::outs() << BB << "\n";
    } */

    modified = 1;
  }
  if (modified) return llvm::PreservedAnalyses::none();
  else return llvm::PreservedAnalyses::all();
}