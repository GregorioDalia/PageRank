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
      Node * sparse_matrix[num_nodi_per_processo_min]
      for (int k = 0; k < num_nodi_per_processo_min; k++){
        sparse_matrix[k] = NULL;
      }
    }
    else{ 
      Node * sparse_matrix[num_nodi_per_processo_min + 1]
      for (int k = 0; k < num_nodi_per_processo_min + 1; k++){
        sparse_matrix[k] = NULL;
      }
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
    Node *sparse_matrix_locale[info[0]]
    int n = info[1]
    // considerare uso di malloc

    for (int k = 0; k < info[0]; k++){
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

    int percento = 0;
    int m = 0;

    while (!feof(fp)){
      
      m++;

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
        NuovoArco->next = sparse_matrix[NuovoArco->end_node];
        sparse_matrix[NuovoArco->end_node] = NuovoArco;
      }
      */

      if(e>=100 && ((m%(e/10)) == 0)){
          percento +=10;
          printf("%d%% ",percento);
      }

      Node *NuovoArco = (struct Node *)malloc(sizeof(Node));
      NuovoArco->start_node = fromnode;
      NuovoArco->end_node = tonode;
      NuovoArco->value = 1;


      // INSERIMENTO IN TESTA
      NuovoArco->next = sparse_matrix[NuovoArco->end_node];
      sparse_matrix[NuovoArco->end_node] = NuovoArco;

      // use fromnode and tonode as index
      out_degree[fromnode]++;
      //in_degree[tonode]++; DA CANCELLARE

    }
  //} fine if master
  printf("\n");

  /*
    DISTRIBUZIONE ARCHI ROUND ROBIN, USANDO MODULO;
    MOLTIPLICAZIONE RIGA PER COLONNA FATTA DA OGNI WORKER, CHE NON COSTRUISCE UN ARRAY MA UNA MATRICE (chiamata my_page_ranks),
    DA INVIARE AL MASTER: [POSIZIONE IN page_ranks COMPLETO, VALORE]
  */
  printf("DEBUG: INITIALIZE PAGE RANKS TO 1/N\n");
  
  percento = 0;
  m=0;
  for (int i = 0; i < n; i++){
    
    m++;
    page_ranks[i] = 1.0 / (float)n;
    old_page_ranks[i] = 1.0 / (float)n;
    mean_coloumn_weighed[i] = (1 - WEIGHT) / (float)n;

    if(n>=100 && ((m%(n/10)) == 0)){
        percento +=10;
        printf("%d%% ",percento);
    }

  }

  printf("\nDEBUG: UPDATE MATRIX\n");
  
  percento = 0;
  m=0;
  
  for (int i = 0; i < n; i++){
    
    m++;
    
    Node *pointer = sparse_matrix[i];

    if(n>=100 && ((m%(n/10)) == 0)){
        percento +=10;
        printf("%d%% ",percento);
    }

    // Update the value of the pointer
    while (pointer != NULL){  
      pointer->value = (WEIGHT / (float)out_degree[pointer->start_node]);
      pointer = pointer->next;
    }

  }    
  
  printf("\n");


  float score_norm;
  int count = 0;

  do{
    printf("\nDEBUG: GIRO N: %d\n", count + 1);
    score_norm = 0;

    percento = 0;
    m=0;
    for (int i = 0; i < n; i++){

      m++;
      
      if(n>=100 && ((m%(n/10)) == 0)){
          percento +=10;
          printf("%d%% ",percento);
      }

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


  for (int i = 0; i < n; i++){
    printf("THE PAGE RANKE OF NODE %d IS : %0.15f \n", i , page_ranks[i]);
  }

  return 0;
}
