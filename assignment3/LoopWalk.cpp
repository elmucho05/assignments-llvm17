#include "llvm/Transforms/Utils/LoopWalk.h"
#include <llvm/IR/PassManager.h>
#include "llvm/IR/Dominators.h"

using namespace llvm;

/*
    Funzione che controlla se un determinato opernado viene utilizzato da una Instruction.
    Questo controllo viene fatto attraverso gli usi(use) dell'operando.
*/
bool isUsedIn(Value *v,Instruction &i) {
	for(auto IterU=v->use_begin();IterU!=v->use_end();IterU++){//si preleva l'istruzione che usa quel determinato operando
		if (i.isIdenticalTo(dyn_cast <Instruction>(IterU->getUser()))){//se coincide con l'istruzione in ingreso, return true
            return true;
        }
	}
    return false;
}

bool isPhi(Instruction &i){
    if(PHINode *phiNode = dyn_cast<PHINode>(&i)){
        return true;
    }
    return false;
}
/*
    Funzione che verifica se l'operando (v) di un'istruzione(i) è parametro della funzione.
    Usata quindi per determinare se un'istruzione nel loop utilizza un parametro della funzione.
*/
bool checkArgs(Value* v,Loop &L,Instruction &i){
    Function *F=(L.getHeader())->getParent();
	for(auto IterArg=F->arg_begin();IterArg!=F->arg_end();IterArg++){
        Value *arg=IterArg;
        if(isUsedIn(arg,i)){ //se un parametro di funzione è usato in un'istruzione, return true
            return true;
        }
    }
    return false;
}
// Prototipo della funzione per verificare se l'operando di una istruzione la rende LoopInvariant.
bool isLoopInvariantValue(Value*, Loop&, Instruction&);

/*
    Funzione che verifica se un'istruzione(i) all'interno del Loop(L) è LoopInvariant.
*/
bool isLoopInvariantInstr(Instruction &i, Loop &L){
    outs() << i << "\n";
    if (isPhi(i)){
        outs() << "-- L'istruzione è una PHI node e quindi non è loop-invariant.\n";
        return false;
    } else {
        for (auto *IterO = i.op_begin(); IterO != i.op_end(); IterO++){
            Value *Operand = *IterO;//devo controllare gli operandi prima, se anche loro non sono loop invariant, return false
            if (!isLoopInvariantValue(Operand, L, i)){
                outs() << "-- Uno degli operandi di"<< *Operand << " non è loop-invariant.\n";
                return false;
            }
        }
        outs() << "-- L'istruzione è loop-invariant.\n";
    }
    return true;
}
/* 
    Funzione che verifica se gli operandi(v) di un'istruzione(i) non la rendono LoopInvariant. 
*/
bool isLoopInvariantValue(Value* v, Loop &L, Instruction &i){
    if (checkArgs(v,L,i)){
        return true;
    }
    //se uno degli operandi è un'istruzione, dobbiamo controllare se è loop invariant
    if (Instruction *inst = dyn_cast<Instruction>(v)){
        outs() << "Analizzando l'operando: " << *inst << "\n";
        if(!L.contains(inst)){
            outs() << "-- L'istruzione si trova fuori dal loop.\n";
            return true;
        }else{
            outs() << "-- L'istruzione si trova dentro il loop.\n";
            if(!isLoopInvariantInstr(*inst, L)){
                outs() << "-- L'istruzione "<< *inst << " non è loop-invariant.\n";
                return false;
            }else{
                outs() << "-- L'istruzione" << *inst << " è loop-invariant.\n";
                return true;
            }
        }
    }//se uno degli operandi è una costante, è loop invariant
    else if(Constant *C = dyn_cast<Constant>(v)){
        outs() << "Analizzando l'operando: " << *C << "\n";
        outs() << "-- L'operando è una costante.\n";
        return true;
    }
    outs() << "-- L'operando non è un'istruzione né una costante.\n";
    return false;
}
/*
    Funzione che verifica se il Basic Block in cui è contenuta l'istruzione domina tutte le uscite del Loop.
    Prima sono estratte le uscite del Loop e inserite in uno SmallVector poi una ad una si verifica se sono 
    dominate dal Basic Block dell'istruzione.
    
    Si controlla quindi che l'istruzione sia eseguita in qualsiasi percorso che porta fuori dal loop
*/
bool dominatesExit(Instruction* i,DominatorTree& DT, Loop& L){
    SmallVector <BasicBlock*> exitBB;
    
    //trovo tutti i blocchi di uscita del loop
    for(auto IterL=L.block_begin();IterL!=L.block_end();IterL++){
        BasicBlock *BB=*IterL;
        if(L.isLoopExiting(BB)){
            exitBB.push_back(BB);//prende i blocchi che hanno almeno un arco che porta fuori dal mio loop
        }
    }
    // controllo se l'istruzione domina ogni blocco di uscita
    for(auto block : exitBB){
        if(!DT.dominates(i->getParent(),block)){ //i.getParent() mi restituisce il puntatore al blocco dov'è contenuta
            outs()<<"non domina le uscite "<<*i<<" "<< block->getName() <<"\n";
            return false;
        }
    }
    return true;
}

/*
    Funzione che verifica se un'istruzione candidata alla code motion domina tutti i suoi usi. 
    Si verificano gli usi e se uno di questi non è dominato ritorna falso.
*/
bool dominatesUse(Instruction* i, DominatorTree& DT, Loop &L){
    for(auto IterU=i->use_begin();IterU!=i->use_end();IterU++){
        if(PHINode *phiNode = dyn_cast<PHINode>(IterU->getUser())){
            //Itera gli incoming value e se il blocco della PHI domina tutti gli incoming block ritorna vero 
            for(auto IterPHI=0 ;IterPHI<phiNode->getNumIncomingValues();IterPHI++){

                if(phiNode->getIncomingValue(IterPHI)== i){ //significa che l'istruzione i è un operando di una PHI node. 
                    BasicBlock *IncomingBlock=phiNode->getIncomingBlock(IterPHI); //prendo il blocco di provenienza dell'oprendo phi node attuale
                    //se il blocco dove l'istruzione(i) è contenuta, domina il blocco di provenienza, allora return true
                    if(!DT.dominates(i->getParent(),IncomingBlock)){
                        outs() <<"non domina gli gli incoming block "<<"\n";
                        return false;
                    }
                }
            }    
        }else if(Instruction* use = dyn_cast <Instruction>(IterU->getUser())){
            if(!DT.dominates(i,use)){
                outs() <<"non domina gli usi " << *use <<"\n";
                return false;
            }
        }
    }
    return true;
}
/*
    Funzione che verifica se un'istruzione è viva solo all'interno del loop controllando i suoi usi.
    In tal caso è candidata alla code motion.
*/
bool isInstrDead(Instruction *i, Loop  &L){
    for(auto IterU=i->use_begin();IterU!=i->use_end();IterU++){
        if (Instruction* use = dyn_cast <Instruction>(IterU->getUser())){
            //controllo sul BB che contiene l'uso ,getParent() restitusice il puntatore al BB dell'uso. 
            if(!L.contains(use->getParent())){ //se questo BB non è contenuto nel Loop, allora ha affetti anche all'esterno
                outs() <<"non dead " << *i <<"\n";
                return false;
            }
        }
    }
    return true;
}

/*
    Funzione che stampa tutte le istruzioni del Loop(L). Usata per far vedere la diffierenza tra prima e dopo
*/
void printInstr(Loop& L){
    for(auto IterL=L.block_begin();IterL!=L.block_end();IterL++){
        BasicBlock *BB=*IterL;
        for(auto IterI=BB->begin();IterI!=BB->end();IterI++){
            Instruction &i = *IterI;
            outs() << i << "\n";
        }
    }
}
PreservedAnalyses LoopWalk::run(Loop &L, LoopAnalysisManager &LAM, LoopStandardAnalysisResults &LAR, LPMUpdater &LU){
    SmallVector <Instruction*> invariants, preHeader, movedInstructions;

    DominatorTree &DT = LAR.DT;
    DT.print(outs());
    BasicBlock *PreHeader = L.getLoopPreheader();
    //se il Loop è in forma semplificata si potrà eseguire il Loop Invariant Code Motion.
    if(L.isLoopSimplifyForm()){
        //si iterano i BB del Loop.
        for(auto IterL=L.block_begin();IterL!=L.block_end();IterL++){
            BasicBlock *BB=*IterL;
            //si iterano le Instruction in ogni BB.
            for(auto IterI=BB->begin();IterI!=BB->end();IterI++){
                Instruction &i = *IterI;
                // outs() << i << "\n";
                //si verifica se un'istruzione è Loop Invariant. In tal caso sarà aggiunta al vettore. 
                if (isLoopInvariantInstr(i,L)){
                    invariants.push_back(&i);
                }else{
                    outs() <<" Istruzione "<< i << " NON Loop Invariant"<<"\n";
                }
                outs () << "--------------------------------------------"<< "\n";
            }
        }
        /* 
            Si verifica se le istruzioni Loop Invariant sono vive solo all'interno del Loop 
            oppure se il loro BB domina tutte le uscite e se l'istruzione domina tutti i suoi usi.
                In tal caso potranno essere messe nel preheader.
        */
        for(auto instr : invariants){
            outs() << *instr <<"\n";
            if(((dominatesExit(instr, DT, L)|| isInstrDead(instr, L))&& dominatesUse(instr,DT,L))){ 
                preHeader.push_back(instr);
            }
        }
        outs () << " PRIMA --------------------------------------------"<< "\n";
        printInstr(L);
        //le istruzioni sono spostate nel preheader.
        for(auto instr : preHeader){
            instr->moveBefore(PreHeader->getTerminator());//getTerminator mi restitusice l'istruzione terminatore(ultima) del preheader.
            movedInstructions.push_back(instr); //vengono salvate per la stampa le istruzioni spostate
        }
        //sono stampate le istruzioni dopo gli spostamenti.
        outs () << " DOPO --------------------------------------------"<< "\n";
        printInstr(L);

        outs() << "ISTRUZIONI SPOSTATE NEL PREHEADER --------------------------------------------" << "\n";
        for (auto instr : movedInstructions) {
            outs() << *instr << "\n";
        }
    
    }else{
        outs() <<"Loop in forma non semplificata" <<"\n";
    }
    return PreservedAnalyses::all();
}