#include "llvm/Transforms/Utils/LocalOpts.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include <llvm/IR/Constants.h>

using namespace llvm;

bool algebraicIdentity(Instruction &I, Instruction::BinaryOps toDoOperation) {
  int pos = 0;
  for (auto operand = I.op_begin(); operand != I.op_end(); operand++, pos++) {
    ConstantInt *costante = dyn_cast<ConstantInt>(operand);
    if (costante) {
      APInt value = costante->getValue();

      if ((value.isZero() && toDoOperation == Instruction::Add) || (value.isOne() && toDoOperation == Instruction::Mul)) {
        I.replaceAllUsesWith(I.getOperand(!pos));
        outs() << "Trovata Algebraic Identity\n  nell'istruzione " << I << "\n  c'è un" << value << " in posizione " << pos<< "\n\n";
        return true;
      }
    }
  }
  return false;
}

bool strengthReduction(Instruction &I, Instruction::BinaryOps toDoOperantion) {
  int pos = 0;//per tenere conto della posizione degli operandi 

  for (auto operand = I.op_begin(); operand != I.op_end(); operand++, pos++) {
    ConstantInt *costante = dyn_cast<ConstantInt>(operand);
    
    if (costante) {
      APInt value = costante->getValue();
      if (value.isPowerOf2()) {
        int shiftValue = costante->getValue().exactLogBase2();

        Instruction::BinaryOps instructionType;

        if (toDoOperantion == Instruction::Mul)
          instructionType = Instruction::Shl;
        else
          instructionType = Instruction::LShr;

        Instruction *shiftInst = 
          BinaryOperator::Create(instructionType, I.getOperand(!pos), ConstantInt::get(costante->getType(), shiftValue));

        shiftInst->insertAfter(&I);
        I.replaceAllUsesWith(shiftInst);
        outs() << "Trovata Strength Reduction\n  Da: " << I << "\n  A: " << *shiftInst << "\n\n";
        return true;
      }
    }
  }
  return false;
}

bool advancedStrengthReduction(Instruction &I) {
  int pos = 0;

  for (auto operand = I.op_begin(); operand != I.op_end(); operand++, pos++) {

    ConstantInt *costante = dyn_cast<ConstantInt>(operand);

    if (costante) {
      APInt value = costante->getValue();
      Instruction::BinaryOps operationType;
      int shiftValue = 0;

      if ((value + 1).isPowerOf2()) {
        shiftValue = (value + 1).exactLogBase2();
        operationType = Instruction::Sub;
      } else if ((value - 1).isPowerOf2()) {
        shiftValue = (value - 1).exactLogBase2();
        operationType = Instruction::Add;
      }else
        continue;

      Instruction *shiftInst =
          BinaryOperator::Create(BinaryOperator::Shl, I.getOperand(!pos), ConstantInt::get(costante->getType(), shiftValue));

      Instruction *sumInst =
          BinaryOperator::Create(operationType, I.getOperand(!pos), shiftInst);

      shiftInst->insertAfter(&I);
      sumInst->insertAfter(shiftInst);
      I.replaceAllUsesWith(sumInst);

      outs() << "Trovata AdvancedStrength Reduction\n  Da:  " << I << "\n  A:   " << *shiftInst << " e " << *sumInst << "\n\n";
      return true;
    }
  }
  return false;
}


bool multiInstruction(Instruction &I, Instruction::BinaryOps toDoOperation) {
  int pos = 0;
  for (auto opUser = I.op_begin(); opUser != I.op_end(); opUser++, pos++) {
    
    ConstantInt *costante = dyn_cast<ConstantInt>(opUser); //%a = add i32 %b, 1
    if (costante) {

      APInt valueToFind = costante->getValue();
      Instruction::BinaryOps opToFind =
          toDoOperation == Instruction::Sub ? Instruction::Add : Instruction::Sub;

      for (auto iter = I.user_begin(); iter != I.user_end(); ++iter) {

        User *userofInstr = *iter;
        BinaryOperator *opUsee = dyn_cast<BinaryOperator>(userofInstr); //%c = sub i32 %a, 1

        if (not opUsee)
          continue;

        for (auto operandUsee = userofInstr->op_begin(); operandUsee != userofInstr->op_end(); operandUsee++) 
        {
          ConstantInt *constantUsee = dyn_cast<ConstantInt>(operandUsee);
          if (constantUsee && opUsee->getOpcode() == opToFind && constantUsee->getValue() == valueToFind){
            outs() << "Trovata Multi Instruction Optimization\n  " << I << " e " << *userofInstr << "\n ";
            userofInstr->replaceAllUsesWith(I.getOperand(!pos));
            return true;
          }
        }
      }
    }
  }
  return false;
}

bool runOnBasicBlock(BasicBlock &B) {

  for (auto &inst : B) {

    BinaryOperator *op = dyn_cast<BinaryOperator>(&inst);

    if (not op)
      continue;

    switch (op->getOpcode()) {
    case BinaryOperator::Mul:
      //si controlla prima se è una strength normale o una algebraic 
      if (!algebraicIdentity(inst, Instruction::Mul) && !strengthReduction(inst, Instruction::Mul))
        advancedStrengthReduction(inst);
      break;
    case BinaryOperator::Add:
      if (!algebraicIdentity(inst, Instruction::Add))
        multiInstruction(inst, Instruction::Add);
      break;

    case BinaryOperator::Sub:
      multiInstruction(inst, Instruction::Sub);
      break;
    case (BinaryOperator::SDiv):
      strengthReduction(inst, Instruction::SDiv);
      break;

    default:

      break;
    }
  }
  return true;
}

bool runOnFunction(Function &F) {
  bool Transformed = false;

  for (auto Iter = F.begin(); Iter != F.end(); ++Iter) {
    if (runOnBasicBlock(*Iter)) {
      Transformed = true;
    }
  }

  return Transformed;
}

PreservedAnalyses LocalOpts::run(Module &M, ModuleAnalysisManager &AM) {
  for (auto Fiter = M.begin(); Fiter != M.end(); ++Fiter)
    if (runOnFunction(*Fiter))
      return PreservedAnalyses::none();

  return PreservedAnalyses::all();
}