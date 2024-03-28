//===-- LocalOpts.cpp - Example Transformations --------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "llvm/Transforms/Utils/LocalOpts.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/InstrTypes.h"
// L'include seguente va in LocalOpts.h
#include <llvm/IR/Constants.h>
#include "llvm/ADT/APInt.h"

#include "llvm/IR/IRBuilder.h"
#include <math.h>
#include <iostream>
#include <stack>

using namespace llvm;
int find_exponential(int64_t n)
      {
          int ct = 0;
          int st = 0;
          while(n)
          {
              if(n & 1)
              {
                  st++;
              }
              n = n >> 1;
              ct++;
          }
          if(st ==  1)
          {
              return ct - 1;
          }
          else
          {
              return -1;
          }
          
      }
bool runOnBasicBlock(BasicBlock &B) {
  using namespace std;

  Value *z, *op1, *op2, *op3, *o, *temp;
  ConstantInt *x, *y;
  stack<Instruction*>delList;
  int algebraic_identities_count = 0;
  int constant_folding_count = 0;
  int strength_reduction_count = 0;

  //loop through each instruction
  for(Instruction &II : B)
  {
    Instruction *I = &II;
    if(dyn_cast<BinaryOperator>(I)) // check if instruction is a binary operator instruction
    {
      z = ConstantInt::get(I->getOperand(0)->getType(),0); //Create zero constant
      o = ConstantInt::get(I->getOperand(0)->getType(),1); //create 1 constant
      op1 = I->getOperand(0); //extract 1st operand
      op2 = I->getOperand(1); //extract 2nd operand
      x = dyn_cast<ConstantInt>(op1); //Check if any operands are constant
      y = dyn_cast<ConstantInt>(op2);
      IRBuilder<> buildInst(I); // to generate new instruction
      if(I->getOpcode() == Instruction::Sub)
      {
        if(op1 == op2)
        {
          I->replaceAllUsesWith(z); // replace the instruction
          delList.push(I); // push the instruction into a stack for later removal
          algebraic_identities_count++;
        }
        else if((y != NULL) && (y->isZero()))
        {
          I->replaceAllUsesWith(op1);
          delList.push(I);
          algebraic_identities_count++;
        }
        else if((x != NULL) && (y != NULL)) // both the operands are constant
        {
          auto temp_val = x->getSExtValue() - y->getSExtValue(); 
          temp = ConstantInt::get(I->getOperand(0)->getType(),temp_val);
          I->replaceAllUsesWith(temp);
          delList.push(I);
          constant_folding_count++;
        }
      }
      else if(I->getOpcode() == Instruction:: Add)
      {
        if((x != NULL) && (x->isZero()))
        {
          I->replaceAllUsesWith(op2);
          delList.push(I);
          algebraic_identities_count++;
        }
        else if((y != NULL) && (y->isZero()))
        {
          I->replaceAllUsesWith(op1);
          delList.push(I);
          algebraic_identities_count++;
        }
        else if((x != NULL) && (y != NULL))
        {
          auto temp_val = x->getSExtValue() + y->getSExtValue();
          temp = ConstantInt::get(I->getOperand(0)->getType(),temp_val);
          I->replaceAllUsesWith(temp);
          delList.push(I);
          constant_folding_count++; 
        }
      }
      else if(I->getOpcode() == Instruction::Mul)
      {
        if((x != NULL) && (x->isZero()))
        {
          I->replaceAllUsesWith(z);
          delList.push(I);
          algebraic_identities_count++;
        }
        else if((y != NULL) && (y->isZero()))
        {
          I->replaceAllUsesWith(z);
          delList.push(I);
          algebraic_identities_count++;
        }
        else if((x != NULL) && (x->isOne()))
        {
          I->replaceAllUsesWith(op2);
          delList.push(I);
          algebraic_identities_count++;
        }
        else if((y != NULL) && (y->isOne()))
        {
          I->replaceAllUsesWith(op1);
          delList.push(I);
          algebraic_identities_count++;
        }
        else if((x != NULL) /*&& (x->equalsInt(2))*/)
        {
          auto exp = find_exponential(y->getSExtValue());
          if(exp != -1)
          {
            Value *e = ConstantInt::get(I->getOperand(0)->getType(),exp);
            op3 = buildInst.CreateShl(op2,e);
            for(auto& it : I->uses())
            {
                User* user = it.getUser();
                user->setOperand(it.getOperandNo(),op3);
            }
            delList.push(I);
            strength_reduction_count++;
          }
        }
          else if((y != NULL) /*&& (y->equalsInt(2))*/)
          {
            auto exp = find_exponential(y->getSExtValue());
            if(exp != -1)
            {
              Value *e = ConstantInt::get(I->getOperand(0)->getType(),exp);
              op3 = buildInst.CreateShl(op1,e);
              for(auto& it : I->uses())
              {
                  User* user = it.getUser();
                  user->setOperand(it.getOperandNo(),op3);
              }
              delList.push(I);
              strength_reduction_count++;
            }
          }
          else if((x != NULL) && (y != NULL))
          {
            auto temp_val = x->getSExtValue() * y->getSExtValue();
            temp = ConstantInt::get(I->getOperand(0)->getType(),temp_val);
            I->replaceAllUsesWith(temp);
            delList.push(I);
            constant_folding_count++;
          }
      }
      else if((I->getOpcode() == Instruction:: SDiv) || (I->getOpcode() == Instruction:: UDiv))
      {
        if((I->getOperand(0)) == (I->getOperand(1)))
        {
          I->replaceAllUsesWith(o);
          delList.push(I);
          algebraic_identities_count++;
        }
        else if((x != NULL) && (x->isZero()))
        {
          I->replaceAllUsesWith(z);
          delList.push(I);
          algebraic_identities_count++;
        }
        else if((y != NULL) && (y->isOne()))
        {
          I->replaceAllUsesWith(op1);
          delList.push(I);
          algebraic_identities_count++;
        }
        else if((y != NULL) /*&& (y->equalsInt(2))*/)
        {
          auto exp = find_exponential(y->getSExtValue());
          if(exp != -1)
          {
            Value *e = ConstantInt::get(I->getOperand(0)->getType(),exp);
            op3 = buildInst.CreateLShr(op1,e);
            for(auto& it : I->uses())
            {
                User* user = it.getUser();
                user->setOperand(it.getOperandNo(),op3);
            }
            delList.push(I);
            strength_reduction_count++;
          }
        }
        else if((x != NULL) && (y != NULL))
        {
          auto temp_val = x->getSExtValue() / y->getSExtValue();
          temp = ConstantInt::get(I->getOperand(0)->getType(),temp_val);
          I->replaceAllUsesWith(temp);
          delList.push(I);
          constant_folding_count++;
        }
      }                      
    }
  }
   while(!delList.empty()) // delete the instructions that are no more required.
  {
      delList.top()->eraseFromParent();
      delList.pop();
  }
  outs() << "Transformations Applied:\n";
  outs() << "Algebraic Indentities: "<< algebraic_identities_count <<"\n";
  outs() << "Constant Folding: "<< constant_folding_count <<"\n";
  outs() << "Strength Reduction: "<< strength_reduction_count <<"\n";
  return true;
}




//   for(auto &I : B){
//     if(I.getOpcode() == Instruction::Mul){
//       outs() << "Trovata una moltiplicazione" << I << "\n";

//       ConstantInt *constPotenzaDue = nullptr;
//       bool scambia = false;
//       //Controllo subito il secondo operando
//       if(ConstantInt *secondConst = dyn_cast<ConstantInt>(I.getOperand(1))){
//         if(secondConst->getValue().isPowerOf2()){
//           constPotenzaDue = secondConst;
//         }
//       }
//       //se il secondo operando nn è una costante, controllo il primo.
//       if(constPotenzaDue==nullptr){
//         if(ConstantInt *firstConst = dyn_cast<ConstantInt>(I.getOperand(0))){
//           if(firstConst->getValue().isPowerOf2()){
//             constPotenzaDue = firstConst;
//             scambia=true;
//           }
//         }
//       }
//  // Check that at least one of the operands is a constant power of 2
//     if (constPotenzaDue != nullptr) {
//         ConstantInt *shiftConst = ConstantInt::get(constPotenzaDue->getType(), constPotenzaDue->getValue().exactLogBase2());
//         outs() << "Costante : " << constPotenzaDue->getValue() << "\n";

//         Instruction *NewInst;

//         // If the first operand is the power of 2, we need to swap the operands
//         if (scambia) {
//           NewInst = BinaryOperator::Create(Instruction::Shl, I.getOperand(1), shiftConst);
//         } else {
//           NewInst = BinaryOperator::Create(Instruction::Shl, I.getOperand(0), shiftConst);
//         }

//         NewInst->insertAfter(&I);
//         I.replaceAllUsesWith(NewInst);
//     } else {
//         outs() << "No power of 2 constant found\n";
//       }
//     }


bool runOnFunction(Function &F) {
  bool Transformed = false;

  for (auto Iter = F.begin(); Iter != F.end(); ++Iter) {
    if (runOnBasicBlock(*Iter)) {
      Transformed = true;
    }
  }

  return Transformed;
}


PreservedAnalyses LocalOpts::run(Module &M,
                                      ModuleAnalysisManager &AM) {
  for (auto Fiter = M.begin(); Fiter != M.end(); ++Fiter)
    if (runOnFunction(*Fiter))
      return PreservedAnalyses::none();
  
  return PreservedAnalyses::all();
}

