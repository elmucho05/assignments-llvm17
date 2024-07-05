#include "llvm/Transforms/Utils/LocalOpts.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"
#include <llvm/IR/Constants.h>

using namespace llvm;
/*
x+0 = 0+x => x
x*1 = 1*x => x
*/
bool algebraicIdentity(Instruction &I, Instruction::BinaryOps toDoOperation) {
  int pos = 0;//teniamo traccia della posizione di un operando
  for (auto operand = I.op_begin(); operand != I.op_end(); operand++, pos++) {
    ConstantInt *costante = dyn_cast<ConstantInt>(operand); //castiamo l'operando in costante se esiste, sennò la dyncast ritorna un nullptr
    if (costante) {
      APInt value = costante->getValue();

      if ((value.isZero() && toDoOperation == Instruction::Add) || (value.isOne() && toDoOperation == Instruction::Mul)) 
      {
        I.replaceAllUsesWith(I.getOperand(!pos));//rimpiazziamo tutti gli usi dell'istruzione con l'operando non costante
        outs() << "Trovata Algebraic Identity\n  nell'istruzione " << I << "\n  c'è un " << value << " in posizione " << pos<< "\n\n";
        return true;
      }
    }
  }
  return false;
}
//x*16 = 16*x = x << 4
//x/16 = 16*x = x >> 4
bool strengthReduction(Instruction &I, Instruction::BinaryOps toDoOperantion) {
  int pos = 0;

  for (auto operand = I.op_begin(); operand != I.op_end(); operand++, pos++) {
    ConstantInt *costante = dyn_cast<ConstantInt>(operand);

    if (costante) {
      APInt value = costante->getValue();
      if (value.isPowerOf2()) {
        int shiftValue = costante->getValue().exactLogBase2();//calcolo del valore di shift

        Instruction::BinaryOps instructionType;
        //controllo necessario per determinare il tipo di shift(sx o dx)
        if (toDoOperantion == Instruction::Mul)
          instructionType = Instruction::Shl;
        else
          instructionType = Instruction::LShr;

        Instruction *shiftInst = 
        BinaryOperator::Create(instructionType, I.getOperand(!pos), ConstantInt::get(costante->getType(), shiftValue));
        
        shiftInst->insertAfter(&I);//inserisco lo shift subito dopo l'istruzione corrente
        I.replaceAllUsesWith(shiftInst);//rimpizziamo gli uso dell'istruzione con l'istruzione di shift
        outs() << "Trovata Strength Reduction\n  Da: " << I << "\n  A: " << *shiftInst << "\n\n";
        return true;
      }
    }
  }
  return false;
}
//x*15 = 15*x = (x << 4) - x
bool advancedStrengthReduction(Instruction &I) {
  int pos = 0;

  for (auto operand = I.op_begin(); operand != I.op_end(); operand++, pos++) {
      ConstantInt *costante = dyn_cast<ConstantInt>(operand);

    if (costante) {
      APInt value = costante->getValue();
      Instruction::BinaryOps operationType;//utile per determinare se dobbiamo sottrare o aggingere al risultato dello shift
      int shiftValue = 0;

      if ((value + 1).isPowerOf2()) {//se value+1 è una potenza di due (se 15+1 è una potenza di due)
        shiftValue = (value + 1).exactLogBase2();
        operationType = Instruction::Sub;//dovrò fare lo shift con potenza di due e infine sottrare x
      } else if ((value - 1).isPowerOf2()) {
        shiftValue = (value - 1).exactLogBase2();
        operationType = Instruction::Add;
      }else {
        continue;
      }

      Instruction::BinaryOps shiftOperation;
      if (I.getOpcode() == Instruction::Mul) {
        shiftOperation = Instruction::Shl;
      } else if (I.getOpcode() == Instruction::SDiv) {
        shiftOperation = Instruction::LShr;
      } else {
        continue;
      }
      //dovrò generare due istruzioni, una di shift una di add oppure sub
      Instruction *shiftInst = BinaryOperator::Create(shiftOperation, I.getOperand(!pos), ConstantInt::get(costante->getType(), shiftValue));
      Instruction *sumInst = BinaryOperator::Create(operationType, I.getOperand(!pos), shiftInst);

      shiftInst->insertAfter(&I);
      sumInst->insertAfter(shiftInst);
      I.replaceAllUsesWith(sumInst);

      outs() << "Trovata AdvancedStrength Reduction\n  Da:  " << I << "\n  A:   " << *shiftInst << " e " << *sumInst << "\n\n";
      return true;
    }
  }
  return false;
}
//a = b+1; c=a-1; c = b+1-1; c = b
/*
  Itera sugli utenti dell'istruzione I per trovare istruzioni che eseguono l'operazione opposta con lo stesso valore costante. 
  Se trova una corrispondenza, posso ottimizzare il codice sostituendo entrambe le istruzioni con una più semplice.
  Vogliamo identificare pattern di istruzioni che si annullano a vicenda, rimuovere queste istruzioni ridondanti. 
*/
bool multiInstruction(Instruction &I, Instruction::BinaryOps toDoOperation) {
  int pos = 0;
  for (auto opUser = I.op_begin(); opUser != I.op_end(); opUser++, pos++) {
    
    ConstantInt *costante = dyn_cast<ConstantInt>(opUser); //%a = add i32 %b, 1
    if (costante) {

      APInt valueToFind = costante->getValue();
      // Instruction::BinaryOps opToFind =
      //     toDoOperation == Instruction::Sub ? Instruction::Add : Instruction::Sub;
      Instruction::BinaryOps opToFind;
      switch (toDoOperation) {
        case Instruction::Sub:
          opToFind = Instruction::Add;
          break;
        case Instruction::Add:
          opToFind = Instruction::Sub;
          break;
        case Instruction::Mul:
          opToFind = Instruction::SDiv;
          break;
        case Instruction::SDiv:
          opToFind = Instruction::Mul;
          break;
        default:
          continue;
      }

      //iteriamo sugli usi dell'istruzione
      for (auto iter = I.user_begin(); iter != I.user_end(); ++iter) {
        User *userofInstr = *iter;
        BinaryOperator *opUsee = dyn_cast<BinaryOperator>(userofInstr); //%c = sub i32 %a, 1

        if (not opUsee)
          continue;
        //per ogni operando dell'uso dell'istruzione
        for (auto operandUsee = userofInstr->op_begin(); operandUsee != userofInstr->op_end(); operandUsee++) 
        {
          ConstantInt *constantUsee = dyn_cast<ConstantInt>(operandUsee);
          // verifica se l'operazione dell'user è l'operazione opposta e se il valore della costante corrisponde
          if (constantUsee && opUsee->getOpcode() == opToFind && constantUsee->getValue() == valueToFind){
            outs() << "Trovata Multi Instruction Optimization\n  " << I << " e " << *userofInstr << "\n ";
            //sostituisce tutte le occorrenze dell'user con l'operando non costante dell'istruzione originale
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
      // Si controlla prima se è una strength normale o una algebraic 
      if (!algebraicIdentity(inst, Instruction::Mul) && !strengthReduction(inst, Instruction::Mul))
        advancedStrengthReduction(inst);
      // Controlla la multi-instruction optimization
      multiInstruction(inst, Instruction::Mul);
      break;
    case BinaryOperator::Add:
      if (!algebraicIdentity(inst, Instruction::Add))
        multiInstruction(inst, Instruction::Add);
      break;

    case BinaryOperator::Sub:
      multiInstruction(inst, Instruction::Sub);
      break;
    case (BinaryOperator::SDiv):
      if (!strengthReduction(inst, Instruction::SDiv))
        advancedStrengthReduction(inst);
      // Controlla la multi-instruction optimization
      multiInstruction(inst, Instruction::SDiv);
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