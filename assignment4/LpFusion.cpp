#include "llvm/Transforms/Utils/LpFusion.h"
#include <llvm/IR/PassManager.h>
#include "llvm/IR/Function.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/IR/Dominators.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/DependenceAnalysis.h"


using namespace llvm;
/*
  Controllo di Control Flow Equivalence per verificare dominanza di L1 su L2 e post dominanza di L2 su L1 
  usando preheader quando non sono guarded, invece uso il guard block quando sono guarded.
*/
bool controlFlowEquivalence(Loop *L1,Loop *L2,Function &F,FunctionAnalysisManager &AM)
{
  DominatorTree &DT = AM.getResult<DominatorTreeAnalysis>(F);
  PostDominatorTree &PDT = AM.getResult<PostDominatorTreeAnalysis>(F);
  if(L1->isGuarded()){
    //usiamo il blocco guard dei due loop per verificare le condizioni di dominanza e post dominanza
    //getLoopGuardBranch restituisce l'istruzione di branch, ma a noi serve il BB nel quale è contenuta
    if(DT.dominates(L1->getLoopGuardBranch()->getParent(),L2->getLoopGuardBranch()->getParent()) && PDT.dominates(L2->getLoopGuardBranch()->getParent(),L1->getLoopGuardBranch()->getParent())){
        outs()<<"Control Flow equivalence\n";
        return true;
    }else{
        return false;
    }
  }else{
      //usiamo il preheader per controllare la dominanza e post dominanza
      if(DT.dominates(L1->getLoopPreheader(),L2->getLoopPreheader()) && PDT.dominates(L2->getLoopPreheader(),L1->getLoopPreheader())){
          outs()<<"Control Flow equivalence\n";
          return true;
      }else{
          return false;
      }
  }
  return false;
}
/*
  Verifica di adiacenza per loop unguarded. Si verifica se l'exit block di L1 è il preheader di L2.
*/
bool areUnguardedLoopsAdjacent(Loop *L1, Loop *L2){
    SmallVector <BasicBlock*> exitBB;
    for(auto IterL=L1->block_begin();IterL!=L1->block_end();IterL++){
        BasicBlock *BB=*IterL;
        if(L1->isLoopExiting(BB)){
            outs () << *BB << "\n";
            exitBB.push_back(BB);
        }
    }
    outs() <<"verifica adiacenza\n";
    for(auto exit : exitBB){
        BasicBlock *pre=L2->getLoopPreheader();
        if(exit == pre->getUniquePredecessor() ){//se exit == al singolo entry point di L2
          //in sostanza getUniquePredecessor() restituisce un BB se il bocco preheader ha solo un predecessore, infatti è vero
          //avere un singolo predecessore significa che al preheader ci posso arrivare soltanto da un'altro blocco
          outs() <<"dove inizia uno finisce l'altro\n"; 
          return true;
        }
    }
    return false;
}
/*
  Verifica di adiacenza per loop guarded. Si verifica se il successore non loop del guard branch di L1 è l'entry block di L2.  
*/
bool areGuardedLoopsAdjacent(Loop* L1, Loop* L2){
  SmallVector <BasicBlock*> exitBB;
  BranchInst *bi= L1->getLoopGuardBranch();//se volessi il BB, avrei dovuto aggiungere .getParent()
  for(auto IterE=0;IterE!= bi->getNumSuccessors();IterE++)
  {
    //iteriamo fra i successori dell'istruzione di branch che funge da guard. Controlliamo se uno di questi successori è l'entry point di L2
    BasicBlock *BB=bi->getSuccessor(IterE);
    //if(N>0)
        //loop1
    //if(N>0)
        //loop2
    if(!L1->contains(BB))//l'esempio sopra mostra perché è necessario questo controllo
    //se L1 non contiene il BB successore, dobbiamo controllare se è l'entry di L2.
    {
      for(auto IterBranch=BB->begin();IterBranch!=BB->end();IterBranch++)
      {
        Instruction &i = *IterBranch;
        //se troviamo un'altra istruzione di guard(e dovrei), controllo i suoi successori per vedere se uno di loro è l'entry point di L2
        if(isa<BranchInst>(i))
        {
          for(auto IterE2=0;IterE2!= i.getNumSuccessors();IterE2++)
          {
            BasicBlock *BB2=i.getSuccessor(IterE2);
            if(L2->contains(BB2->getSingleSuccessor()))//ci basta controllare se il successore del BB è unico e che sia contenuto in L2 
                return true; //FIXME
          }
        }
      }
    }
  }
  return false;
}
/*
  Utilizzando due oggetti Scalar Evolution Analysis Type si verifica se i loop hanno un  numero di iterazioni prevedibile e uguale.
  La Scalar Evolution offre una rappresentazione matematica di come le variabili scalari cambiano nelle varie iterazioni del loop
*/
bool checkLoopTripCount(Loop* L1, Loop *L2, Function& F, FunctionAnalysisManager &AM){
  ScalarEvolution &SE = AM.getResult<ScalarEvolutionAnalysis>(F);
  //Dalla Documentazione :
  // If the specified loop has a predictable backedge-taken count, return it,
  // otherwise return a SCEVCouldNotCompute object. The backedge-taken count is
  // the number of times the loop header will be branched to from within the
  // loop, assuming there are no abnormal exists like exception throws. This is
  // one less than the trip count of the loop, since it doesn't count the first
  // iteration, when the header is branched to from outside the loop.
  auto countL1= SE.getBackedgeTakenCount(L1);
  auto countL2= SE.getBackedgeTakenCount(L2);
  // Se non è stato ritornato un SCEVCouldNotCompute, allora posso restituire true
  if(countL1->getSCEVType()!=SCEVCouldNotCompute().getSCEVType() && countL2->getSCEVType()!=SCEVCouldNotCompute().getSCEVType()){
      if (countL1==countL2){
          outs() << "trip count uguali" <<"\n";
          return true;
      }
  }
  return false;
}
/* 
  Si verifica la presenza di dipendenze negative tra Loop 1 e Loop 2 iterando tutte le istruzione presenti nei loop.
  for (i=0; i<N; i++){
    A[i] = … ;
    }
  for (i=0; i<N; i++){
    B[i] = A[i+3] + …; //se fondessi i due loop, all'iterazione 'i', A[i+3] non sarebbe ancora disponibile
}
*/
bool checkDistanceDependencies(Loop *L1,Loop* L2, Function &F, FunctionAnalysisManager&AM){
  DependenceInfo &DI = AM.getResult<DependenceAnalysis>(F);
  
  for(auto IterL1=L1->block_begin();IterL1!=L1->block_end();IterL1++)
  {
    BasicBlock *BBL1= *IterL1;//iteriamo sulle istruzioni del BB corrente di L1
    for(auto IterL1I=BBL1->begin();IterL1I!=BBL1->end();IterL1I++)
    {
      Instruction &iL1= *IterL1I;//si salva il puntatore all'istruzione I di L1
      //si itera nello stesso modo tra i BB e istruzioni di L2
      for(auto IterL2=L2->block_begin();IterL2!=L2->block_end();IterL2++)
      {
        BasicBlock *BBL2= *IterL2;
        for(auto IterL2I=BBL2->begin();IterL2I!=BBL2->end();IterL2I++)
        {
          Instruction &iL2= *IterL2I;
          //Test se c'è dipendeza tra le due istruzioni
          // se non c'è dipendenza return NULL, se c'è restituisce un'oggetto Dependece
          auto dep= DI.depends(&iL1,&iL2,true);
          if(dep){//if dep != NULL
              outs() <<" Dipendenza tra \n"<< iL1 <<"\n"<<iL2<<"\n";
              if(dep->isDirectionNegative()){
                //una Negative Direction significa che l'istruzione 1 deve
                // essere completata prima che venga eseguita l'istruzione 2
                outs() <<" NEGATIVA \n";
                return false;
              }
          }
        }
      }
    }
    
  }
  return true;
}
/*Vengono uniti i loop connettendo tra loro le seguenti parti:
Unguarded:hL1->exitL2 bodyL1->bodyL2->LatchL1 hL2->LatchL2
Guarded: guardL1->exitL2 headerL1->headerL2->LatchL1->exitL2*/
bool loopFuse(Loop* L1,Loop* L2){
    BasicBlock * headerL1 =L1 -> getHeader();
    BasicBlock * latchL1 =L1 -> getLoopLatch();
    BasicBlock * bodyL1;
    BasicBlock * exitL1= L1 -> getExitBlock();
    BasicBlock * preHeaderL2 = L2-> getLoopPreheader();
    BasicBlock * headerL2 =L2 -> getHeader();
    BasicBlock * latchL2 = L2 -> getLoopLatch();
    BasicBlock * bodyL2;
    BasicBlock * exitL2= L2 -> getExitBlock();

    // L'header contine inizializzazioni e controllo delle condizioni, quindi può avere diversi successori,
    // di conseguenza è necessario controllare quale di questi successori fa effettivamente parte del loop 
    // cosìché possiamo identificare il body del loop (loop2 per primo)
    if(L2->contains(headerL2->getTerminator()->getSuccessor(0))){
      bodyL2 = headerL2->getTerminator()->getSuccessor(0);//getTerminator() mi restituisce un'istruzione e prendo il primo BB successore
    }else{
      bodyL2 = headerL2->getTerminator()->getSuccessor(1);
    }
    //identificare il body del loop 1
    if(L1->contains(headerL1->getTerminator()->getSuccessor(0))){
      bodyL1 = headerL1->getTerminator()->getSuccessor(0); 
    }else{
      bodyL1 = headerL1->getTerminator()->getSuccessor(1);
    }
    if(!L1 ->isGuarded()){ //if loop is ungarded
      //Unguarded:hL1->exitL2 bodyL1->bodyL2->LatchL1 hL2->LatchL2
      //header1->getTerminator()->replaceSuccorWith  (oldBB, newBB)
      headerL1->getTerminator()->replaceSuccessorWith(preHeaderL2,exitL2);

      //prima : bodyL1->latchL1               //ora : bodyL1->bodyL2
      bodyL1->getTerminator()->replaceSuccessorWith(latchL1,bodyL2);

      //prima : bodyL2->latchL2              //ora bodyL2->latchL1
      bodyL2->getTerminator()->replaceSuccessorWith(latchL2,latchL1);

      //prima : headerL2->bodyL2             //ora headerL2->latchL2
      headerL2->getTerminator()->replaceSuccessorWith(bodyL2,latchL2);
    }else{
      //Guarded: guardL1->exitL2 headerL1->headerL2->LatchL1->exitL2*/

      BasicBlock * guardL1 =  L1->getLoopGuardBranch()->getParent();
      BasicBlock * guardL2 =  L2->getLoopGuardBranch()->getParent();
      //prima : guardL1-> guardL2            //ora guardL1 -> exitL2
      guardL1->getTerminator()->replaceSuccessorWith(guardL2,exitL2);

      //prima : headerL1-> latchL1           // headerL1 -> headerL2
      headerL1->getTerminator()->replaceSuccessorWith(latchL1,headerL2);

      //prima : headerL2-> latchL2           // headerL2 -> latchL1
      headerL2->getTerminator()->replaceSuccessorWith(latchL2,latchL1);

      //prima : latchL1->exitL1              //latchL1  -> exitL2
      latchL1->getTerminator()->replaceSuccessorWith(exitL1,exitL2);
    }
    return true;
}
/*
  Viene presa l'Induction Variable per entrambi i loop e tutti gli usi della seconda IV sono sostituiti con la prima IV. 
  La seconda viene poi eliminata.
  La IV è una tipo di Scalar Variable che è incrementata or decremenetata di un valore fisso ad ogni iterazione del loop
*/
void changeIV(Loop* L1, Loop* L2){
  //Dalla Documentazione della funzione getCanonicalInductionVariable:
  //Check to see if the loop has a canonical induction variable: an integer
  //recurrence that starts at 0 and increments by one each time through the
  //loop. If so, return the phi node that corresponds to it.
  PHINode * IV1 = L1->getCanonicalInductionVariable();
  PHINode * IV2 = L2->getCanonicalInductionVariable();
  IV2->replaceAllUsesWith(IV1);
  IV2->eraseFromParent();//slega IV2 dal BB dove si trova e la elimina
}
//Funzione che prende due loop alla volta dal vettore e 
//verifica se rispettano le condizioni di loop fusion chiamando i metodi di verifica.
bool isLoopFusionable(SmallVector<Loop *>Loops,Function &F, FunctionAnalysisManager &AM){
  SmallVector<Loop*> LoopV; //Loop vector
  for(auto *L: Loops){
      LoopV.push_back(L);
      outs() << *L <<"\n";
  }
  for(int i=0;i<LoopV.size();i++){
    Loop *L1=LoopV[i];
    outs() << "preso loop 1\n";
    for(int j=i+1;j<LoopV.size();j++){
      Loop *L2=LoopV[j];
      outs() <<"preso loop 2\n";
      //Verifica se sono sullo stesso livello.
      if(L1->getLoopDepth()==L2->getLoopDepth())
      { //se sono sullo stesso livello, getLoopDepth = 1 per un loop di un singolo livello (non contiene altri loop)
        //questa condizione infatti si verfica tipicamente per i loop guarded, ma devo cmq controllare se il loop è guarded
        if(L1->isGuarded())
        {
          outs() << "loop 1 è guarded\n";
          //verifiche per loop guarded.
          if(areGuardedLoopsAdjacent(L1,L2) && checkLoopTripCount(L1,L2,F,AM) && controlFlowEquivalence(L1,L2,F,AM) && checkDistanceDependencies(L1,L2,F,AM))
          {
            outs() << "I loop sono adiacenti, hanno lo stesso trip count e non hanno dipendenze negative. Procediamo con la fusione\n";
            //sostituzione della Induction Variable.
            changeIV(L1,L2);
            outs() <<"IV sostituita \n";
            //metodo per la fusione dei due loop.
            loopFuse(L1,L2);
            i+=1;
          }
          else{
            return false;
          }
        }
        else
        {
          outs() << "loop 1 non è guarded\n";
          //verifiche per loop non guarded.
          if(areUnguardedLoopsAdjacent(L1,L2)  && checkLoopTripCount(L1,L2,F,AM) && controlFlowEquivalence(L1,L2,F,AM) && checkDistanceDependencies(L1,L2,F,AM)){
            outs() << "I loop sono adiacenti, hanno lo stesso trip count e non hanno dipendenze negative. Procediamo con la fusione\n";
            //sostituzione della Induction Variable.
            changeIV(L1,L2);
            //setodo per la fusione dei due loop.
            loopFuse(L1,L2);
            i+=1;
          }else{
              return false;
          }
        }
      }
    }
  }
  return true;
}

PreservedAnalyses LpFusion::run(Function &F, FunctionAnalysisManager &AM){
  //Estrazione dei loop e inserimento in smallVector che sarà passato alla funzione.
  LoopInfo &LI= AM.getResult<LoopAnalysis>(F);
  SmallVector<Loop*> LoopsOrdered=LI.getLoopsInPreorder();
  isLoopFusionable(LoopsOrdered,F,AM);
  return PreservedAnalyses::all();
}