#include <stdio.h>
#include <stdlib.h>

// to handle the sparse matrix
typedef struct Node
{

  int start_node;
  int end_node;
  float value;

  struct Node *next;
} Node;

#define WEIGHT 0.85  // Real in (0, 1), best at 0.85
#define ERROR 0.0001 // Real in (0, +inf), best at 0.0001

int main(){
  int info[2];
  // Open the data set
  //char filename[] = "./web-NotreDame.txt";
  //if(r== 0){ // r rank del processo, 0 è il processo master
  char filename[] = "./DEMO.txt";
  printf("DEBUG: open the file %s",filename);

  FILE *fp;
  int n,e;
  // n: number of nodes   e: number of edges

  if ((fp = fopen(filename, "r")) == NULL){
    fprintf(stderr, "[Error] cannot open file");
    exit(1);
  }

  // Read the data set and get the number of nodes (n) and edges (e)
  char ch;
  char str[100];
  ch = getc(fp);

  while (ch == '#'){
    fgets(str, 100 - 1, fp);
    // Debug: print title of the data set
    printf("%s", str);
    sscanf(str, "%*s %d %*s %d", &n, &e); // number of nodes
    ch = getc(fp);
  }
  
  ungetc(ch, fp);

  // DEBUG: Print the number of nodes and edges, skip everything else
  printf("DEBUG:\nGraph data:\n\n  Nodes: %d, Edges: %d \n\n", n, e);
  // Controllo numero archi: maggiore del numero di processi?

  // distribuiamo ipotizzando che l'indegree dei nodi è +/- bilanciato
  /*
    num_nodi_per_processo_min = n/p
    resto = n%p
    if(resto == 0){
      size = num_nodi_per_processo_min;
    }
    else{ 
      size = num_nodi_per_processo_min + 1;
    }

    Node * sparse_matrix[size]
    for (int k = 0; k < size; k++){
        sparse_matrix[k] = NULL;
    }
  
    info[1] = n
    for(int i = 1; i < p; i++){
      if(i < resto) {
        info[0] = num_nodi_per_processo_min + 1;
        send(i,info);
      }
      else{
        info[0] = num_nodi_per_processo_min;
        send(i,info);
      }
    }

    
  */



  
  //}   //Fine if master
  /* else{
    int info[2]
    
    receive(0, info)
    size = info[0]
    Node *sparse_matrix_locale[size] // considerare uso di malloc
    int n = info[1]
    

    for (int k = 0; k < size; k++){
      sparse_matrix[k] = NULL;
    }
  }

  */

  // Creation of the matrix from the file and count of outdegree and indregree of all nodes
  int fromnode, tonode;

  //int* in_degree = malloc(n * sizeof(int)); DA CANCELLARE
  int* out_degree = malloc(n * sizeof(int)); // Lo fanno tutti, master e workers

  
  float* page_ranks = malloc(n * sizeof(float));
  //float* old_page_ranks = malloc(n * sizeof(float)); // verione seriale
  float* old_page_ranks = malloc(info[0] * sizeof(float)); // versione mpi
  
  //float* mean_coloumn_weighed = malloc(n * sizeof(float)); // verione seriale
  float* mean_coloumn_weighed = malloc(info[0] * sizeof(float)); // versione mpi

  //Creation of the sparse matrix
  //Node *sparse_matrix[n]; DA CANCELLARE

  printf("DEBUG: INITIALIZATION\n");
  for (int k = 0; k < n; k++){
    //in_degree[k] = 0; DA CANCELLARE
    out_degree[k] = 0;
    //sparse_matrix[k] = NULL; DA CANCELLARE
  }

  info[0] = 0; // from node (-1 se finisce file)
  info[1] = 0; // to node (-1 se finisce file)
  
  //if (r == 0){
    printf("\n");
    printf("DEBUG: READ FILE\n");


    while (!feof(fp)){
      

      fscanf(fp, "%d%d", &fromnode, &tonode);

      /*
      info[0] = fromnode;
      info[1] = tonode;
      dest = fromnode % p // p numero processi

      if (dest != 0){
        send (dest, info, ...);
      }
      else{
        Node *NuovoArco = (struct Node *)malloc(sizeof(Node));
        NuovoArco->start_node = fromnode;
        NuovoArco->end_node = tonode;
        NuovoArco->value = 1;

        // INSERIMENTO IN TESTA
        NuovoArco->next = sparse_matrix[NuovoArco->end_node % size];
        sparse_matrix[NuovoArco->end_node % size] = NuovoArco;
      }
      */

      // use fromnode and tonode as index
      out_degree[fromnode]++;
      //in_degree[tonode]++; DA CANCELLARE

    }
    /*
    // Send the edges
    info[0] = -1;
    info[1] = -1;
    for(i = 1; i < p; i++){
      send(i, info);
      MPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest,
        int tag, MPI_Comm comm)
    }

    // Mettere barrier?
    // Send the out_degree array
    for(i = 1; i < p; i++){
      send(i, out_degree);
      MPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest,
        int tag, MPI_Comm comm)
    }
    */
  //} fine if master
  /* else{  // WORKER receive edge code
      
      // Receive the new edge
      receive(0, info)
      MPI_Recv(void *info, int count, MPI_Datatype datatype,
          int source, int tag, MPI_Comm comm, MPI_Status *status)

      // Check if the master doesn't reach the eof
      while(info[0] != -1 && info[1] != -1){
        
        Node *NuovoArco = (struct Node *)malloc(sizeof(Node));
        NuovoArco->start_node = fromnode;
        NuovoArco->end_node = tonode;
        NuovoArco->value = 1;


        // INSERIMENTO IN TESTA
        NuovoArco->next = sparse_matrix[NuovoArco->end_node % size];
        sparse_matrix[NuovoArco->end_node % size] = NuovoArco;

        // Receive the new edge
        receive(0, info)
        MPI_Recv(void *info, int count, MPI_Datatype datatype,
          int source, int tag, MPI_Comm comm, MPI_Status *status)

      }

      receive(0, out_degree);
  }*/
  printf("\n");

  /*
    DISTRIBUZIONE ARCHI ROUND ROBIN, USANDO MODULO;
    MOLTIPLICAZIONE RIGA PER COLONNA FATTA DA OGNI WORKER, CHE NON COSTRUISCE UN ARRAY MA UNA MATRICE (chiamata my_page_ranks),
    DA INVIARE AL MASTER: [POSIZIONE IN page_ranks COMPLETO, VALORE]
  */




  printf("DEBUG: INITIALIZE PAGE RANKS TO 1/N and UPDATE MATRIX\n");

  // Same code MASTER and WORKER
  for (int i = 0; i < n; i++){

  // old_page_rank è il page rank completo, ricevuto dal master, new_page_ranks è invece
  // il sotto-array di page rank calcolato dal worker e ha dimensione size.
  // Per fare la differenza, il worker accede in old_page_ranks a partire dal proprio rank(nel senso proprio id) e saltando di p
    if(i<size){
      new_page_ranks[i] = 1.0 / (float)n;
      mean_coloumn_weighed[i] = (1 - WEIGHT) / (float)n;
      
      Node *pointer = sparse_matrix[i];

      // Update the value of the pointer
      while (pointer != NULL){  
        pointer->value = (WEIGHT / (float)out_degree[pointer->start_node]);
        pointer = pointer->next;
      }
      
    }
      

    old_page_ranks[i] = 1.0 / (float)n;
    
  }
  
     
  
  printf("\n");


  float score_norm;
  int count = 0;

  do{
    printf("\nDEBUG: GIRO N: %d\n", count + 1);
    score_norm = 0;
  
    for (int i = 0; i < n; i++){


      float sum = 0.0;
      Node *currNode = sparse_matrix[i];

      int j = 0;
      do{

        sum += (page_ranks[currNode->start_node] * currNode->value);

        currNode = currNode->next;

        j++;

      } while (j < in_degree[i]);

      // somma con colonna costante mean_coloumn_weighed
      page_ranks[i] = sum + mean_coloumn_weighed[i];

      // take the absolute value of the error, using old_page_rank avoiding to create a new variable 
      old_page_ranks[i] = page_ranks[i] - old_page_ranks[i];
      if (old_page_ranks[i] < 0)
        old_page_ranks[i] = -old_page_ranks[i];

      // sum to the score_norm
      score_norm += old_page_ranks[i];

      // reinitialize the old_pagerank value to the current pagerank
      old_page_ranks[i] = page_ranks[i];
    }

    count++;

  } while (score_norm > ERROR);
  
  printf("\n");
  printf("DEBUG: NUMBER OF ITERATION: %d\n", count);
  printf("\n");

  /*
    int x,k,i,newElem;
    //x indice risultati locali
    //k ID processo da cui ricevere
    //newElem buffer di ricezione di dimensione 1
    
    // Il processo master non ha bisogno di un local_page_ranks ma inserire i valori, man mano che li calcola,
    // direttamente in page_ranks, saltando ogni volta di p
    
    for (i=0,x=0,k=0;i<n;i++){
        if(k != 0){
          receive(k,newElem , int ...)
          page_rank[i]=newElem;
        }
        k++;
        if (k==p)k=0;

    }


    */

  for (int i = 0; i < n; i++){
    printf("THE PAGE RANKE OF NODE %d IS : %0.15f \n", i , page_ranks[i]);
  }

  

  return 0;
}
