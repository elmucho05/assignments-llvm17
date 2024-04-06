//===-- LocalOpts.cpp - Example Transformations --------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "llvm/Transforms/Utils/LocalOpts.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include <llvm/IR/Constants.h>

using namespace llvm;

// clang -emit-llvm -S -c file.c .o file.ll
// opt

bool strengthReduction(Instruction &I, Instruction::BinaryOps toDoOperantion) {
  int pos = 0;

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

bool advStrengthReduction(Instruction &I) {
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

bool multiInstOpt(Instruction &I, Instruction::BinaryOps toDoOperation) {
  int pos = 0;
  for (auto operandUser = I.op_begin(); operandUser != I.op_end(); operandUser++, pos++) {
    ConstantInt *CUser = dyn_cast<ConstantInt>(operandUser); //%a = add i32 %b, 1

    if (CUser) {

      APInt valueToFind = CUser->getValue();
      Instruction::BinaryOps opToFind =
          toDoOperation == Instruction::Sub ? Instruction::Add : Instruction::Sub;

      for (auto iter = I.user_begin(); iter != I.user_end(); ++iter) {

        User *instUser = *iter;
        BinaryOperator *opUsee = dyn_cast<BinaryOperator>(instUser); //%c = sub i32 %a, 1

        if (not opUsee)
          continue;

        for (auto operandUsee = instUser->op_begin(); operandUsee != instUser->op_end(); operandUsee++) 
        {
          ConstantInt *CUsee = dyn_cast<ConstantInt>(operandUsee);
          if (CUsee && opUsee->getOpcode() == opToFind && CUsee->getValue() == valueToFind){
            outs() << "Trovata Multi Instruction Optimization\n  " << I << " e " << *instUser << "\n ";
            instUser->replaceAllUsesWith(I.getOperand(!pos));

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
      if (!algebraicIdentity(inst, Instruction::Mul) && !strengthReduction(inst, Instruction::Mul))
        advStrengthReduction(inst);
      break;
    case BinaryOperator::Add:
      if (!algebraicIdentity(inst, Instruction::Add))
        multiInstOpt(inst, Instruction::Add);
      break;

    case BinaryOperator::Sub:
      multiInstOpt(inst, Instruction::Sub);
      break;

    case (BinaryOperator::UDiv):
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