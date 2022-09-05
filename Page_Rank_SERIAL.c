#include <stdio.h>
#include <stdlib.h>
#include<string.h>
#include<time.h> 

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

int main(int argc, char *argv[]){

  // Open the data set
  //char filename[] = "./web-NotreDame.txt";
  //char filename[] = "./DEMO.txt";
  /*
  char filename[10];
  strcpy(filename,argv[1]);

  char firstnode[1];
  strcpy(firstnode,argv[2]);

  printf("value %s and %s\n",argv[1],argv[2]);
  */
  int firstnode;
  int n,e;
  
  int fromnode, tonode;
  float mean_coloumn_weighed ;
  float score_norm;

  char ch;
  char str[100];
  FILE *fp;

  clock_t start, end;
  double cpu_time_used;




  if(strcmp("0",argv[2]) == 0)firstnode = 0;
    else if (strcmp("1",argv[2]) == 0)firstnode = 1;
      else firstnode = -1;

  if(firstnode == -1){
    printf("INVALID ARGUMENT %s  %s \n",argv[1],argv[2]);
    exit(1);
  }

  printf("DEBUG: open the file %s",argv[1]);

  
  // n: number of nodes   e: number of edges

  if ((fp = fopen(argv[1], "r")) == NULL){
    fprintf(stderr, "[Error] cannot open file");
    exit(1);
  }

  // Read the data set and get the number of nodes (n) and edges (e)
  ch = getc(fp);


  while (ch == '#'){
    fgets(str, 100 - 1, fp);
    // Debug: print title of the data set
    //printf("%s", str);
    sscanf(str, "%*s %d %*s %d", &n, &e); // number of nodes
    ch = getc(fp);
  }

  ungetc(ch, fp);

  // DEBUG: Print the number of nodes and edges, skip everything else
  //printf("DEBUG:\nGraph data:\n\n  Nodes: %d, Edges: %d \n\n", n, e);

  // Creation of the matrix from the file and count of outdegree and indregree of all nodes
  

  int* in_degree = malloc(n * sizeof(int));
  int* out_degree = malloc(n * sizeof(int));

  float* page_ranks = malloc(n * sizeof(float));
  float* old_page_ranks = malloc(n * sizeof(float));
  

  //Creation of the sparse matrix
  Node ** sparse_matrix = malloc(n * sizeof(Node*));

  //printf("DEBUG: INITIALIZATION\n");
  for (int k = 0; k < n; k++){
    in_degree[k] = 0;
    out_degree[k] = 0;
    sparse_matrix[k] = NULL;
  }


  //printf("\n");
  //printf("DEBUG: READ FILE\n");

  int percento = 0;
  int m = 0;

  while (!feof(fp)){

    m++;

    fscanf(fp, "%d%d", &fromnode, &tonode);

    if(firstnode == 1){
      fromnode=fromnode -1 ;
      tonode=tonode-1;
    }


    if(e>=100 && ((m%(e/10)) == 0)){
        percento +=10;
        //printf("%d%% ",percento);
    }

    Node* NuovoArco = malloc(sizeof(Node));
    NuovoArco->start_node = fromnode;
    NuovoArco->end_node = tonode;
    NuovoArco->value = 1;


    // INSERIMENTO IN TESTA
    NuovoArco->next = sparse_matrix[NuovoArco->end_node];
    sparse_matrix[NuovoArco->end_node] = NuovoArco;

    // use fromnode and tonode as index
    out_degree[fromnode]++;
    in_degree[tonode]++;

  }
  printf("\n");
  /*printf("MATRIX IS \n");
  for (int i = 0 ; i < n ; i++){

    printf("ROW %d",i);
    Node *pointer = sparse_matrix[i];

    do{
        printf(" %d - %d /// ",pointer->end_node,pointer->start_node);
        pointer=pointer->next;
    }while(pointer !=NULL);

    printf("\n");

  }
  */


  //printf("DEBUG: INITIALIZE PAGE RANKS TO 1/N\n");

  percento = 0;
  m=0;
  for (int i = 0; i < n; i++){

    m++;
    page_ranks[i] = 1.0 / (float)n;
    old_page_ranks[i] = 1.0 / (float)n;
    //mean_coloumn_weighed[i] = (1 - WEIGHT) / (float)n;

    if(n>=100 && ((m%(n/10)) == 0)){
        percento +=10;
        //printf("%d%% ",percento);
    }

  }
    mean_coloumn_weighed = (1 - WEIGHT) / (float)n;


  //printf("\nDEBUG: UPDATE MATRIX\n");

  percento = 0;
  m=0;

  for (int i = 0; i < n; i++){

    m++;

    Node *pointer = sparse_matrix[i];

    if(n>=100 && ((m%(n/10)) == 0)){
        percento +=10;
        //printf("%d%% ",percento);
    }

    // Update the value of the pointer
    while (pointer != NULL){
      pointer->value = (WEIGHT / (float)out_degree[pointer->start_node]);
      pointer = pointer->next;
    }

  }

  printf("\n");
  /*printf("MATRIX IS \n");
  for (int i = 0 ; i < n ; i++){

    printf("ROW %d",i);
    Node *pointer = sparse_matrix[i];

    do{
        printf(" %d - %d = %f /// ",pointer->end_node,pointer->start_node,pointer->value);
        pointer=pointer->next;
    }while(pointer !=NULL);

    printf("\n");

  }
  */



  // Measure time from now
  start = clock();

  
  int count = 0;

  do{
    //printf("\nDEBUG: GIRO N: %d\n", count + 1);
    score_norm = 0.0;

    /*
    printf("DEBUG : INITIAL PAGE RANK\n");
    for(int i = 0; i < n ;i++){
        printf("Node %d is %f\n",i,page_ranks[i]);
    }
    */

    percento = 0;
    m=0;
    for (int i = 0; i < n; i++){

      m++;

      if(n>=100 && ((m%(n/10)) == 0)){
          percento +=10;
          //printf("%d%% ",percento);
      }

      float sum = 0.0;
      Node *currNode = sparse_matrix[i];

      while (currNode!=NULL){

        sum += (old_page_ranks[currNode->start_node] * currNode->value);

        currNode = currNode->next;


      }

      // somma con colonna costante mean_coloumn_weighed
      //printf("row %d finalsum = %f\n",i,sum);
      page_ranks[i] = sum + mean_coloumn_weighed;
      //printf("row %d page_ranks[i] = %f\n",i,page_ranks[i]);


      float diff = page_ranks[i] - old_page_ranks[i];

      // take the absolute value of the error, using old_page_rank avoiding to create a new variable
      if (diff < 0)
        diff = -diff;

      // sum to the score_norm
      score_norm += diff;

      // reinitialize the old_pagerank value to the current pagerank
      //old_page_ranks[i] = page_ranks[i];
      //printf("at row %d old page is %f \n",i,old_page_ranks[i]);
    }

    for (int i =0 ; i < n;i++){
        old_page_ranks[i]=page_ranks[i];
    }
    count++;

    //printf("score norm is %0.50f\n",score_norm);

  } while (score_norm > ERROR);

  /*
  printf("\n");
  printf("DEBUG: NUMBER OF ITERATION: %d\n", count);
  printf("\n");


  for (int i = 0; i < n; i++){
    printf("THE PAGE RANKE OF NODE %d IS : %0.50f \n", i , page_ranks[i]);
  }*/

  end = clock();
  cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;

  printf ("Tempo di esecuzione (secondi): %f\n", cpu_time_used);

  return 0;
}
