#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


#define DIMHT 64
#define EXPLODED -1
#define LIMPAROLE 64
#define LIMPAROLEVALIDE 128
#define POSOK 1
#define POSNO 2
#define MINIMO 3
#define ESATTO 4
#define DIMCRONO 100
#define CMDMAX 20  				//lunghezza massima dei comandi
#define DIMBUFF 200


typedef struct nodo_{
	struct nodo_ *next;
	char *parola;	
}nodo_t;

typedef struct tabella_{
	nodo_t* head[DIMHT];
	int numparole[DIMHT];
	long long int contatore;	
}tabella_t;

typedef struct elenco_{
	char hash;
	char tipo;
	int informazione;
}elenco_t;


char *vincoli;													//0 indeterminato, 1 c'è, 2 non c'è
int giusti[DIMHT];												//inizializzato a -1, indica il # di lettere corretto
int minimo[DIMHT];												//inizializzato a 0, indica il # di lettere minimo trovato
elenco_t *crono;												//cronologia di vincoli
long long int dimCrono, lastCrono_inizio, lastCrono_fine;		//valori indici di crono
tabella_t *HashTable, *Valide;									//tabelle principali
int k;															//# lettere parole
int dim_max;													//dimensione massima tra k e comandi
char *soluzione;												//stringa soluzione di ogni partita
int tentativi;													//# tentativi di ogni partita
int gioco;														//indice di livello di gioco: 3=inserimento soluzione, 2=inserimento tentativi, 1=partita
int buffDim;													//dimensione buffer parole
nodo_t *buffer;													//buffer nodi con puntatore a buffer parole
char *bufferParole;												//buffer parole
int inserimento;												//indice di inserimento


void stampaTabella(tabella_t *);  						//stampa contenuto di tabella in input
void inserisciTabella(tabella_t *, char[]);				//inserisce parola in tabella Hash
void inserisciTabellaValide(tabella_t *, char[]);		//inserisce parola in tabella Valide
void esplodiTabella(tabella_t *, int, int);				//crea sottotabella
int ricercaTabella(char[]);								//ricerca parola nella sottotabella
void inizializzaVincoli();								//inizializzazione per partita
int controllo(char[], char[]);							//controllo tentativo
void aggiornaVincoli(char, char, int);					//accoda vincolo
int filtraVincoli(char[], long long int);				//controlla se parola rispetta i vincoli
void creaValide();										//crea tabella Valide			
long long int aggiornaValide(tabella_t *);				//aggiunge ulteriori vincoli a Valide
void deallocaValide(tabella_t *);						//dealloca tabella Valide a fine partita
void riempiValide(tabella_t *);							//inserisce le parole nella tabella Valide
int scanner(char[]);									//funzione ausiliaria scanf + veloce
nodo_t * allocaParola(char[]);							//funzione ausiliaria per malloc a gruppi di 100 parole
int maggiore(char[], char[]);							//funzione ausiliaria strcmp
int trovaHash(char);									//funzione ausiliaria che calcola l'hash di una lettera


int main(){																		//OK
	char *comando=NULL;	
	int i,risultato;
	
	gioco=0;
	inserimento=2;
	
	HashTable=NULL;
	while(HashTable==NULL)
		HashTable=malloc(sizeof(tabella_t));
	
	HashTable->contatore=0;
	for(i=0; i<DIMHT; i++){
		HashTable->head[i]=NULL;
		HashTable->numparole[i]=0;
	}
	
	if(scanf("%d", &k));
	
	buffDim=0;
	buffer=malloc(sizeof(nodo_t)*DIMBUFF);
	bufferParole=malloc(sizeof(char)*DIMBUFF*(k+1));
	
	soluzione=malloc((k+1)*sizeof(char));
	vincoli=malloc(sizeof(char)*DIMHT*k);
	
	if(k>CMDMAX)
		dim_max=k+1;
	else
		dim_max=CMDMAX+1;

	comando=malloc(dim_max*sizeof(char));
	if(comando==NULL)
		return 0;
		
	Valide=NULL;
	
	while(!feof(stdin)){
		
		scanner(comando);
		if(isalnum(comando[0]) || comando[0]=='-' || comando[0]=='_'){
			
			if(inserimento>0){
				if(!gioco || Valide==NULL)
					inserisciTabella(HashTable,comando);
				
				else{
					inserisciTabella(HashTable,comando);
					if(filtraVincoli(comando, 0))
						inserisciTabellaValide(Valide, comando);
				}
			}
			
			else if(gioco==3){
				strcpy(soluzione,comando);
				gioco--;
			}
			
			else if(gioco==2){
				tentativi=atoi(comando);
				gioco--;
			}
			
			else if(gioco==1){
				
				risultato=controllo(soluzione, comando);
				
				if(risultato==2){
					
					if(Valide!=NULL)
						deallocaValide(Valide);
						
					Valide=NULL;	
					free(crono);
					gioco=0;
				}
				
				else if(risultato==1){
					
					if(Valide==NULL)
						creaValide();
						
					else if(lastCrono_inizio!=lastCrono_fine)
						aggiornaValide(Valide);
					
					printf("%lld\n", Valide->contatore);
					tentativi--;
					lastCrono_inizio=lastCrono_fine;
				}
				
				if(tentativi==0 && gioco==1){
					printf("ko\n");
					
					if(Valide!=NULL)
						deallocaValide(Valide);	
						
					Valide=NULL;	
					free(crono);
					gioco=0;
				}
			}
		}
		
		else if(maggiore(comando,"+nuova_partita")==2){
			gioco=3;
			inserimento=0;
			inizializzaVincoli();
			Valide=NULL;
		}
		
		else if(maggiore(comando,"+inserisci_inizio")==2){
			if(inserimento==0)
				inserimento=1;
		}
		
		else if(maggiore(comando,"+inserisci_fine")==2){
			if(inserimento==1)
				inserimento=0;	
		}
		
		else if(maggiore(comando,"+stampa_filtrate")==2){
			if(!gioco || Valide==NULL)
				stampaTabella(HashTable);
			else
				stampaTabella(Valide);
		}
	}

	return 0;
}

void inserisciTabella(tabella_t *rif, char parola[]){							//OK
	
	tabella_t *curr=rif;	
	int hash=0, livello=0;	
	nodo_t *nuovo, *tmp, *prec;
		
	hash=trovaHash(parola[livello]);
	while(curr->numparole[hash]==EXPLODED)	{
		curr->contatore++;
		curr=((tabella_t*)curr->head[hash]);
		livello++;
		hash=trovaHash(parola[livello]);
	}
	
	curr->contatore++;
	
	nuovo=allocaParola(parola);	
	
	if(curr->head[hash]==NULL)
		curr->head[hash]=nuovo;
		
	else{
		tmp=curr->head[hash];
		prec=NULL;
		
		while(tmp!=NULL && maggiore(parola, tmp->parola) ){
			prec=tmp;
			tmp=tmp->next;			
		}
		
		nuovo->next=tmp;
		
		if(prec)
			prec->next=nuovo;
			
		else
			curr->head[hash]=nuovo;		
	}
	
	curr->numparole[hash]++;
	
	if(curr->numparole[hash]==LIMPAROLE)
		esplodiTabella(curr, hash, livello+1);
}

void inserisciTabellaValide(tabella_t *rif, char parola[]){						//OK
	
	tabella_t *curr=rif;	
	int hash=0, livello=0;	
	nodo_t *nuovo, *tmp, *prec;
		
	hash=trovaHash(parola[livello]);
	while(curr->numparole[hash]==EXPLODED)	{
		curr->contatore++;
		curr=((tabella_t*)curr->head[hash]);
		livello++;
		hash=trovaHash(parola[livello]);
	}
	
	curr->contatore++;
		
	nuovo=NULL;	
	while(nuovo==NULL)
		nuovo=malloc(sizeof(nodo_t));
		
	nuovo->parola=NULL;	
	while(nuovo->parola==NULL)
		nuovo->parola=malloc(sizeof(char)*(k+1));	
	
	strcpy(nuovo->parola, parola);
	nuovo->next=NULL;
	
	if(curr->head[hash]==NULL)
		curr->head[hash]=nuovo;
		
	else{
		tmp=curr->head[hash];
		prec=NULL;
		
		while(tmp!=NULL && maggiore(parola, tmp->parola)){
			prec=tmp;
			tmp=tmp->next;			
		}
		
		nuovo->next=tmp;
		
		if(prec)
			prec->next=nuovo;
			
		else
			curr->head[hash]=nuovo;		
	}
	
	curr->numparole[hash]++;
	
	if(curr->numparole[hash]==LIMPAROLEVALIDE)
		esplodiTabella(curr, hash, livello+1);		
}

void stampaTabella(tabella_t* tabella){											//OK

	nodo_t *tmp;
	int i;
	
	tmp=NULL;
	for(i=0; i<DIMHT; i++){
		
		tmp=tabella->head[i];
		if(tabella->numparole[i]==EXPLODED){
			stampaTabella((tabella_t*)tmp);
		}
		
		else{
			while(tmp!=NULL){
				printf("%s\n", tmp->parola);
				tmp=tmp->next;			
			}
		}
	}	
}

void esplodiTabella(tabella_t *tabella, int hash, int livello){					//OK

	int hash2=0, i;
	tabella_t *esploso;
	nodo_t *tmp, *prec;
	
	esploso=NULL;
	
	while(esploso==NULL)	
		esploso=malloc(sizeof(tabella_t));
	
	esploso->contatore=tabella->numparole[hash];
	tabella->numparole[hash]=EXPLODED;
			
	for(i=0; i<DIMHT; i++){
		esploso->head[i]=NULL;
		esploso->numparole[i]=0;
	}
	
	tmp=tabella->head[hash];
	prec=NULL;
	
	while(tmp!=NULL){
		hash2=trovaHash(tmp->parola[livello]);
		
		if(esploso->head[hash2]==NULL){
			esploso->head[hash2]=tmp;
			if(prec!=NULL)
				prec->next=NULL;
		}
		
		esploso->numparole[hash2]++;
		
		prec=tmp;
		tmp=tmp->next;			
	}
	
	tabella->head[hash]=(nodo_t*)esploso;	
}

int ricercaTabella(char ricercata[]){											//OK
	
	tabella_t *curr=HashTable;	
	nodo_t *tmp;
	int hash=0, livello=0;
	
	hash=trovaHash(ricercata[livello]);
	while(curr->numparole[hash]==EXPLODED){
		curr=(tabella_t*)curr->head[hash];
		livello++;
		hash=trovaHash(ricercata[livello]);
	}
	
	tmp=curr->head[hash];
	while(tmp!=NULL){
		if(maggiore(ricercata, tmp->parola)==2)
			return 1;
		tmp=tmp->next;
	}
	
	return 0;	
}

void inizializzaVincoli(){														//OK
	
	int i;
	for(i=0; i<DIMHT*k; i++){
		vincoli[i]=0;
	}
	
	for(i=0; i<DIMHT; i++)
		giusti[i]=-1;
		
	for(i=0; i<DIMHT; i++)
		minimo[i]=0;
		
	crono=NULL;
	while(crono==NULL)
		crono=malloc(sizeof(elenco_t)*DIMCRONO);
		
	dimCrono=DIMCRONO;
	
	lastCrono_inizio=0;
	lastCrono_fine=0;	
}

int controllo(char trova[], char p[]){											//OK

	int i, j, posizione, count[DIMHT], hash;
	char res[k+1], r[k+1];
	
	strcpy(r, trova);
		
	if(maggiore(r, p)==2){
		printf("ok\n");
		return 2;
	}
	
	if(!ricercaTabella(p)){
		printf("not_exists\n");
		return 0;
	}
	
	for(i=0; i<DIMHT; i++){
		count[i]=0;
	}
	
	for(i=0; i<k; i++){
		
		hash=trovaHash(p[i]);
		posizione=k*hash+i;
		
		if(r[i]==p[i]){			
			res[i]='+';		
			
			if(vincoli[posizione]!=POSOK){
				vincoli[posizione]=POSOK;
				aggiornaVincoli(hash, POSOK, i);
			}			
		}
			
		else{			
			res[i]='/';
			for(j=0; j<k; j++){
				
				if(r[j]!=p[j] && p[i]==r[j]){
					res[i]='|';
					r[j]='*';
					break;
				}
							
			}
			
			if(res[i]=='/' && giusti[hash]==-1){
				
				for(j=i+1;j<k;j++){
					if(r[j]==p[j] && p[i]==r[j])
						count[hash]++;
				}
				
				giusti[hash]=count[hash];
				aggiornaVincoli(hash, ESATTO, count[hash]);				
			}
			
			if(vincoli[posizione]!=POSNO){
				vincoli[posizione]=POSNO;
				aggiornaVincoli(hash, POSNO, i);
			}				
		}
		
		count[hash]++;		
	}
	
	res[k]='\0';
	printf("%s\n", res);
	
	for(i=0; i<DIMHT; i++){
		
		if(giusti[i]==-1 && count[i]>minimo[i]){
			minimo[i]=count[i];
			aggiornaVincoli(i, MINIMO, count[i]);
		}
	}
	
	return 1;	
}

void aggiornaVincoli(char charact, char type, int info){						//OK

	elenco_t *tmp;

	if(lastCrono_fine == dimCrono){
		tmp=NULL;
	
		while(tmp==NULL)
			tmp=realloc(crono, dimCrono*sizeof(elenco_t)+DIMCRONO*sizeof(elenco_t));
			
		crono=tmp;
		dimCrono=dimCrono+DIMCRONO;
	}
	
	crono[lastCrono_fine].hash=charact;
	crono[lastCrono_fine].tipo=type;
	crono[lastCrono_fine].informazione=info;
	
	lastCrono_fine++;
}

int filtraVincoli(char parola[], long long int inizio){							//OK
	
	long long int i;
	int j, count;
	
	for(i=lastCrono_fine-1; i>=inizio; i--){

		if(crono[i].tipo==POSOK){
			if(trovaHash(parola[crono[i].informazione])!=crono[i].hash)
				return 0;
		}	
			
		else if(crono[i].tipo==POSNO){		
			if(trovaHash(parola[crono[i].informazione])==crono[i].hash)
				return 0;
		}	
		
		else{
			count=0;
			for(j=0; j<k; j++){
				
				if(trovaHash(parola[j])==crono[i].hash)
					count++;
			}
			
			if(crono[i].tipo==ESATTO && count!=crono[i].informazione)
				return 0;
				
			else if(crono[i].tipo==MINIMO && count<crono[i].informazione)
				return 0;
		}
	}
	
	return 1;	
}

void creaValide(){																//OK
	int i;
	
	Valide=malloc(sizeof(tabella_t));
	Valide->contatore=0;
	
	for(i=0; i<DIMHT; i++){
		Valide->head[i]=NULL;
		Valide->numparole[i]=0;
	}
	
	riempiValide(HashTable);
}

void riempiValide(tabella_t *tabella){											//OK
	int i;
	nodo_t *tmp;
	
	for(i=0; i<DIMHT; i++){
		
		if(tabella->numparole[i]==EXPLODED)
			riempiValide((tabella_t*)tabella->head[i]);
			
		else{
			tmp=tabella->head[i];
			while(tmp!=NULL){
				
				if(filtraVincoli(tmp->parola, lastCrono_inizio))
					inserisciTabellaValide(Valide, tmp->parola);						
				
				tmp=tmp->next;
			}
		}
	}
}

long long int aggiornaValide(tabella_t *tabella){								//OK
		
	int i;
	long long int nuoviel;
	long long int eliminati=0;
	nodo_t *tmp, *prec;
	
	for(i=0; i<DIMHT; i++){
		
		if(tabella->numparole[i]==EXPLODED){
			
			nuoviel=aggiornaValide((tabella_t*)tabella->head[i]);
			eliminati=eliminati+nuoviel;
			tabella->contatore=tabella->contatore-nuoviel;
			
			if(((tabella_t*)tabella->head[i])->contatore==0){
				free((tabella_t*)tabella->head[i]);
				tabella->head[i]=NULL;
				tabella->numparole[i]=0;
			}			
		}
		
		else{
			tmp=tabella->head[i];
			
			while(tmp!=NULL && !filtraVincoli(tmp->parola, lastCrono_inizio)){
				tabella->head[i]=tmp->next;
				free(tmp->parola);
				free(tmp);
				tabella->contatore--;
				eliminati++;
				tmp=tabella->head[i];
			}
			
			prec=tabella->head[i];
			
			if(prec)
				tmp=prec->next;
			else
				tmp=NULL;
					
			while(tmp!=NULL){
				
				if(!filtraVincoli(tmp->parola, lastCrono_inizio)){
					prec->next=tmp->next;
					free(tmp->parola);
					free(tmp);	
					tabella->contatore--;
					eliminati++;				
				}	
					
				else
					prec=prec->next;
					
				tmp=prec->next;
			}
		}
	}
	
	return eliminati;
}

void deallocaValide(tabella_t *tabella){										//OK
	
	int i;
	nodo_t *tmp, *tmp2;
	
	for(i=0; i<DIMHT; i++){
		if(tabella->numparole[i]==EXPLODED){
			deallocaValide((tabella_t *)tabella->head[i]);
		}
		
		else{
			tmp=tabella->head[i];
			
			while(tmp!=NULL){
				tmp2=tmp->next;
				free(tmp->parola);
				free(tmp);
				tmp=tmp2;
			}
		}
	}
	
	free(tabella);	
}

int scanner(char parola[]){														//OK
	char i;
	int j;	
	
	i=getchar_unlocked();
	
	while(!feof(stdin) && !isgraph(i))
		i=getchar_unlocked();
		
	j=1;
	parola[0]=i;
	i=getchar_unlocked();
	
	while(!feof(stdin) && isgraph(i)){
		parola[j]=i;
		j++;
		i=getchar_unlocked();
	}
	
	parola[j]='\0';
	
	while(!feof(stdin) && i!='\n')
		i=getchar_unlocked();

	return 1;
}

nodo_t * allocaParola(char parola[]){											//OK
		
	nodo_t *tmp;
	if(buffDim==DIMBUFF){
		buffDim=0;
		buffer=NULL;
		bufferParole=NULL;
		
		while(buffer==NULL)
			buffer=malloc(sizeof(nodo_t)*DIMBUFF);
			
		while(bufferParole==NULL)
			bufferParole=malloc(sizeof(char)*DIMBUFF*(k+1));
	}
	
	buffer[buffDim].parola=bufferParole+buffDim*(k+1);
	buffer[buffDim].next=NULL;
	
	strcpy(buffer[buffDim].parola, parola);
	tmp=buffer+buffDim;
	buffDim++;
	return tmp;
	
}

int maggiore(char p1[], char p2[]){												//OK

	int i;
	for(i=0; p1[i]!='\0'; i++){
		if(p1[i]>p2[i])
			return 1;
		else if(p1[i]<p2[i])
			return 0;
	}
	
	return 2;
}

int trovaHash(char lettera){													//OK
	int num, hash=0;
	num=lettera;
	
	if(num==45)	//-
		hash=0;
	else if(num>=48 && num<=57) //0-9
		hash=num-47;
	else if(num>=65 && num<=90) //A-Z
		hash=num-54;
	else if(num==95) //_
		hash=37;
	else if(num>=97 && num<=122) //a-z
		hash=num-59;
	return hash;
}
