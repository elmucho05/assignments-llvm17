Thigs to do for loop fusion


1. L1 e L2 devono essere adiacenti. 
	non ci possono essere statement/BB che esegunoo tra la fine di L1 e l'inizio di L2.
	**caso guarded**
	se i loop sono guarded, cioè si eseguono ad una determinata condizione, il successore NON LOOP del guard branch deve essere l'entry block di L1.
	**caso uguarded**
	collego l'exit block di L1 con il preheader di L2

	/*Vengono uniti i loop connettendo tra loro le seguenti parti:
Unguarded:hL1->exitL2 bodyL1->bodyL2->LatchL1 hL2->LatchL2
Guarded: guardL1->exitL2 headerL1->headerL2->LatchL1->exitL2*/


2. Devono poter iterare lo stesso numero di volte
	Richiede quindi di fare quello che è il *trip count* dei loop. Quello che ci dà questo tipo di informazione è lo *Scalar Evolution*
 	**Scalar Evolution**
	cambio il valore di una variabile scalare ad ogni iterazione del loop.
	invece di t = t+k.
	Questa si chiama ***induction variable***
	```
	%tk = add nsw i32 %t, %k
	```
	mi deve diventare
	```
	%tk.final = mul i32 %n, %k
	--> t = n*k
	```
3. I loop devono essere control flow equivalent
	-> quando L1 esgue,anche L2 deve eseguire e viceversa
	
	Servono informazioni di dominanza e postdominanza
    - Se L1 domina L2 e L2 postdomina L0, allora i due loop sono *control flow equivalenti*

4. NON ci deveno essere dipendenze negative di distanza tra i due loop
	```
	for (i=0; i<N; i++){
		A[i] = … ;
	}
	for (i=0; i<N; i++){
		B[i] = A[i+3] + …;
	}
	```
	*se io dovessi fondere i due loop, allora sarebbe un problema perché i+3 non l'avrei ancora calcolato*.



## Trasformazione di codice
Una volta verificate tutte le condizioni per la loop fusion, passo alla trasformazione di codice.
Devo fare due cose:
1. Modificare gli usi della ***induction variable*** nel bodu del loop2 con quelli della induction variabile del loop1. (Ricodandoci che in SSA sono due var diverse)
2. Modificare il CFG perché il body di L2 sia collegato al body di L1.
