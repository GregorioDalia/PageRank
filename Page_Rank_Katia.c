#include <stdio.h>
#include <stdlib.h>
#include<string.h>
#include <mpi.h>

typedef struct Node
{
  int start_node;
  int end_node;
  float value;
  struct Node *next;
} Node;

#define WEIGHT 0.85     // Real in (0, 1), best at 0.85
#define ERROR 0.0001   // Real in (0, +inf), best at 0.0001
//#define LOCAL_ERROR 0.000001 
#define MASTER 0        // Rank of the master process
#define TAG 0           /* MPI message tag parameter */


int main(int argc, char *argv[]){

  // Variables for MPI communication              
  int numtasks;               /* number of MPI tasks */
  int rank;                   /* my MPI task number */
  int rc;                     /* return code */
  MPI_Status status;          /* MPI receive routine parameter */

  // Variables for input file reading
  //char filename[] = "./DEMO.txt";    /* file containing the list of the edges */
  //char filename[] = "./web-NotreDame.txt";    /* file containing the list of the edges */
  FILE *fp;                             /* file pointer */
  char ch;                              /* reads the characters in the file */
  char str[100];                        /* buffer for storing file lines */

  // Variables for page rank algorithm
  int n, e;                             /* n: number of nodes   e: number of edges */
  int rows_num;                         /* number of rows managed by each process */
  int remaining_rows;                   /* remainder of the division n/numtasks */
  int* info;                            /* buffer for sending information from master to workers */
  int fromnode, tonode;                 /* fromnode: start node of an edge    tonode: end node of an edge */
  int* out_degree;                      /* vector containing the out_degree of all nodes */
  float* old_page_ranks;          /* sub page_ranks vector of a process */
  float* page_ranks;           /* complete page_rankes vector, all processes have their copy */
  float* very_old_page_ranks;
  float teleport_probability;           /* probability of the random walker to teleport on a random node */ 
  Node** sparse_matrix_local;           /* sparse matrix containing only the rows of the transition matrix managed by the process*/
  Node *pointer;                        /* iterates over sparse_matrix_local */     
  Node *currNode;   
  int iterate;                          /* flag: checks if the algorithm is converging or not*/
  float sum;                            /* result of the multiplication of a row of the transition matrix and the page rank vector */
  float score_norm = 0;                 /* difference between two consecutive page ranks */
  float local_score_norm = 0;           /* difference between two consecutive page ranks in the single process */
  float diff;                           /* difference between two elements of consecutive page ranks */
  int min_rows_num;                     /* minimum number of rows managed by a process */
  int max_rows_num;                     /* maximum number of rows managed by a process */
  float* minarray;                      /* array of size min_rows_num; stores the rows managed by a process */
  float* maxarray;                      /* array of size max_rows_num; stores the rows managed by a process */

  // Variable for performance measures
  double wallClock_start, wallClock_stop;


  double MPItime_start,MPItime_end;
  int firstnode;
      int count = 0;



  // MPI initialization
  MPI_Init(&argc,&argv);
  MPI_Comm_size(MPI_COMM_WORLD,&numtasks);
  MPI_Comm_rank(MPI_COMM_WORLD,&rank);

  info = malloc ( 2 * sizeof(int));  

  // MASTER CODE : read the input file
  if(rank == MASTER){ 
    if(strcmp("0",argv[2]) == 0)firstnode = 0;
    else if (strcmp("1",argv[2]) == 0)firstnode = 1;
    else firstnode = -1;

    if(firstnode == -1){
      printf("INVALID ARGUMENT %s  %s \n",argv[1],argv[2]);
      exit(1);
    }

    printf("DEBUG: MASTER open the file %s\n",argv[1]);

    if ((fp = fopen(argv[1], "r")) == NULL){
      fprintf(stderr, "[Error] cannot open file");
      exit(1);
    }

    // Read the data set and get the number of nodes (n) and edges (e)
    ch = getc(fp);

    while (ch == '#'){
      fgets(str, 100 - 1, fp);
      sscanf(str, "%*s %d %*s %d", &n, &e); // number of nodes and edges
      ch = getc(fp);
    }
      
    ungetc(ch, fp);

    
    min_rows_num = n/numtasks;
    max_rows_num = min_rows_num + 1;
    
    minarray = malloc((min_rows_num + 1) * sizeof(float));

    maxarray = malloc((max_rows_num + 1) * sizeof(float));

    remaining_rows = n%numtasks;
    
  
    info[1] = n;

    //Prendo il wall clock time
    //papi_Time_start = PAPI_get_real_usec();
    //MPItime_start = MPI_Wtime();

    // Send the rows number and the number of nodes to all WORKER
    for(int i = 1; i < numtasks; i++){
      if(i < remaining_rows) {       
        info[0] = max_rows_num;

        MPI_Send(info, 2, MPI_INT, i, TAG, MPI_COMM_WORLD);

      }
      else{
        info[0] = min_rows_num;

        MPI_Send(info, 2, MPI_INT, i, TAG, MPI_COMM_WORLD);

      }
    }

    if(remaining_rows != 0){
      rows_num = max_rows_num;
    }else{
      rows_num = min_rows_num;
    }

    sparse_matrix_local = malloc(rows_num * sizeof(Node *));
    for (int k = 0; k < rows_num; k++){
        sparse_matrix_local[k] = NULL;
    }

    very_old_page_ranks = malloc(n * sizeof(float));
  }   
  // WORKER : receive the values of the edges
  else{
    MPI_Recv(info, 2, MPI_INT, MASTER, TAG, MPI_COMM_WORLD,MPI_STATUS_IGNORE);
    rows_num = info[0];

    sparse_matrix_local = malloc(rows_num * sizeof(Node *)); 
    
    n = info[1];

    for (int k = 0; k < rows_num; k++){
      sparse_matrix_local[k] = NULL;
    }

  }

  // Creation of the array for the out_degree of all nodes
  out_degree = malloc(n * sizeof(int));

  // Creation of the sub page_ranks vector for a process
  old_page_ranks = malloc((n + 1) * sizeof(float));
  
  // Creation of the complete page_ranks vector
  page_ranks = malloc((n+1) * sizeof(float));
  
  for (int k = 0; k < n; k++){
    out_degree[k] = 0;
  }

  info[0] = 0; // from node
  info[1] = 0; // to node
  
  if (rank == MASTER){

    while (!feof(fp)){
      fscanf(fp, "%d%d", &fromnode, &tonode);
      if(firstnode == 1){
      fromnode=fromnode -1 ;
      tonode=tonode-1;
    }

      info[0] = fromnode;
      info[1] = tonode;
      int dest = tonode % numtasks; 

      if (dest != MASTER){
        MPI_Send(info, 2, MPI_INT, dest, TAG, MPI_COMM_WORLD);
      }
      else{
        Node * NuovoArco =  malloc(sizeof(Node));
        NuovoArco->start_node = fromnode;
        NuovoArco->end_node = tonode;
        NuovoArco->value = 1;

        // INSERIMENTO IN TESTA
        NuovoArco->next = sparse_matrix_local[(tonode - rank) / numtasks];
        sparse_matrix_local[(tonode - rank) / numtasks] = NuovoArco;
        
        
      }

      // use fromnode and tonode as index
      out_degree[fromnode]++;
    }
     
    // Send the edges
    info[0] = -1;
    info[1] = -1;
    for(int i = 1; i < numtasks; i++){
      MPI_Send(info, 2, MPI_INT, i, TAG, MPI_COMM_WORLD);
    }

    // Send the out_degree array
    for(int i = 1; i < numtasks; i++){
        MPI_Send(out_degree, n, MPI_INT, i, TAG, MPI_COMM_WORLD);
    }
    
   }
   else{  // WORKER receive edge code
      
    MPI_Recv(info, 2, MPI_INT, MASTER, TAG, MPI_COMM_WORLD,
            MPI_STATUS_IGNORE);

    // Check if the master doesn't reach the eof
    while(info[0] != -1 && info[1] != -1){
      
      fromnode = info[0];
      tonode = info[1];

      Node * NuovoArco =  malloc(sizeof(Node));
      NuovoArco->start_node = fromnode;
      NuovoArco->end_node = tonode;
      NuovoArco->value = 1;

      // INSERIMENTO IN TESTA
      NuovoArco->next = sparse_matrix_local[(tonode - rank) / numtasks];
      sparse_matrix_local[(tonode - rank) / numtasks] = NuovoArco;

      // Receive the new edge
      MPI_Recv(info, 2, MPI_INT, 0, TAG, MPI_COMM_WORLD,
            MPI_STATUS_IGNORE);

    }

    MPI_Recv(out_degree, n, MPI_INT, MASTER, TAG, MPI_COMM_WORLD,
            MPI_STATUS_IGNORE);
  }

  if(rank==MASTER){
    MPItime_start = MPI_Wtime();
  }
 
  teleport_probability = (1 - WEIGHT) / (float)n;

  for (int i = 0; i < n; i++){

    if(i<rows_num){
      
      
      pointer = sparse_matrix_local[i];

      // Update the value of the pointer
      while (pointer != NULL){  
        pointer->value = (WEIGHT / (float)out_degree[pointer->start_node]);
        pointer = pointer->next;
      }
    }

    if(rank==MASTER){
      very_old_page_ranks[i] = 1.0 / (float)n;
    }
    
    old_page_ranks[i] = 1.0 / (float)n;
    page_ranks[i] = 1.0 / (float)n;
  }

    
  iterate = 1;

/***************************************************************************************************/
    while(iterate ){
        if(rank==MASTER)count++;

        do{
            local_score_norm = 0;

            float* temp = page_ranks;
            page_ranks = old_page_ranks;
            old_page_ranks = temp;
            /*
            if(rank == 1){
              printf("\nI'M THE PROCESS %d AND THIS IS MY PAGE RANK AFTER THE SWITCH:\n", rank);
              for (int i = 0; i < n; i++){
                  printf("IN PROCESS %d THE PAGE RANK OF NODE %d IS : %0.50f \n", rank, i , page_ranks[i]);
              }

              printf("\nI'M THE PROCESS %d AND THIS IS MY OLD PAGE RANK AFTER THE SWITCH:\n", rank);
              for (int i = 0; i < n; i++){
                  printf("IN PROCESS %d THE PAGE RANK OF NODE %d IS : %0.50f \n", rank, i , old_page_ranks[i]);
              }
            }
            */
            for(int i = 0, k = rank, j = 0; i < n; i++){
              if(i == k){
                sum = 0.0;
                currNode = sparse_matrix_local[j];

                while (currNode!=NULL){
                    sum += (old_page_ranks[currNode->start_node] * currNode->value);
                    currNode = currNode->next;
                } 

                page_ranks[i] = sum + teleport_probability;
                
                // take the absolute value of the error
                diff = page_ranks[i] - old_page_ranks[i];
                if (diff < 0){
                    diff = -diff;
                }
                    
                // sum to the score_norm
                float temp = local_score_norm + diff;
                local_score_norm += diff;

                // update the round robin index for moving in complete_page_ranks
                k += numtasks;
                j++;
              }
              else{
                page_ranks[i] = old_page_ranks[i];
              }
            }


            /*
            for (int i = 0,k=rank; i < rows_num;i++){
                sum = 0.0;
                currNode = sparse_matrix_local[i];

                while (currNode!=NULL){
                    sum += (old_page_ranks[currNode->start_node] * currNode->value);
                    currNode = currNode->next;
                } 

                page_ranks[k] = sum + teleport_probability;
                
                // take the absolute value of the error
                diff = page_ranks[k] - old_page_ranks[k];
                if (diff < 0){
                    diff = -diff;
                }
                    
                // sum to the score_norm
                float temp = local_score_norm + diff;
                local_score_norm += diff;

                // update the round robin index for moving in complete_page_ranks
                k += numtasks;
              }
            }
            
            if(rank == 1){
              for (int i = 0; i < n; i++){
                  printf("\n\nIN PROCESS %d THE PAGE RANK (AFTER CALCULUS) OF NODE %d IS : %0.50f \n\n", rank, i , page_ranks[i]);
              }

              printf("IN PROCESS %d LOCAL SCORE NORM IS : %0.50f \n", rank, local_score_norm);
            }*/
            

        } while(local_score_norm > ERROR);

        
        
        // MASTER update the page rank and valuete the error
        if (rank == MASTER){
            score_norm = 0.0;

            for (int sender_rank = 1 ; sender_rank < numtasks;sender_rank++){

                MPI_Recv(old_page_ranks, n + 1, MPI_FLOAT, sender_rank, TAG, MPI_COMM_WORLD,MPI_STATUS_IGNORE);
                    /*printf("\nI'M THE MASTER AND THIS IS THE PAGE RANK I RECEIVED FROM %d\n", sender_rank);
                    for (int i = 0; i < n; i++){
                        printf("IN PROCESS %d THE PAGE RANK (RECEIVED) OF NODE %d IS : %0.50f \n", rank, i , old_page_ranks[i]);
                    }*/
                    
                    for( int k=0,i= sender_rank; k<max_rows_num  ; k++){
                        page_ranks[i] = old_page_ranks[i];
                        i=i+numtasks;
                    }
                    //score_norm += old_page_ranks[n];

            } 

            float global_diff;

            //printf("\nI'M THE MASTER AND THIS IS MY NEW PAGE RANK\n");
            for (int i = 0; i < n; i++){
                //printf("IN PROCESS %d THE PAGE RANK OF NODE %d IS : %0.50f \n", rank, i , page_ranks[i]);
                global_diff = page_ranks[i] - very_old_page_ranks[i];
                if(global_diff < 0){
                  global_diff = -global_diff;
                }
                score_norm += global_diff;
            }

            //printf("\nI'M THE MASTER AND THIS IS MY VERY OLD PAGE RANK\n");
            for (int i = 0; i < n; i++){
                //printf("IN PROCESS %d THE PAGE RANK OF NODE %d IS : %0.50f \n", rank, i , very_old_page_ranks[i]);
            }

            /*printf("\nI'M THE MASTER AND THIS IS MY OLD PAGE RANK\n");
            for (int i = 0; i < n; i++){
                printf("IN PROCESS %d THE PAGE RANK OF NODE %d IS : %0.50f \n", rank, i , old_page_ranks[i]);
            }
            
            printf("\nTOTAL SCORE NORM IS: %0.50f\n", score_norm);*/
            if(score_norm <= ERROR)  {
                iterate = 0;
            }
            
            page_ranks[n] = iterate;

            // Send the new old_page_rank value to all worker
            for(int i=1; i<numtasks; i++){
                MPI_Send(page_ranks, n+1, MPI_FLOAT, i, TAG, MPI_COMM_WORLD);
            }

            very_old_page_ranks = page_ranks;
        }
        // WORKERS send the new page_rank values
        else{

            page_ranks[n]=local_score_norm;
            
            MPI_Send(page_ranks, n+1, MPI_FLOAT, 0, TAG, MPI_COMM_WORLD);  

            MPI_Recv(page_ranks, n+1, MPI_FLOAT, 0, TAG, MPI_COMM_WORLD,MPI_STATUS_IGNORE);
            /*
            printf("\nI'M THE PROCESS %d AND THIS IS THE PAGE RANK I RECEIVED:\n", rank);
            for (int i = 0; i < n; i++){
                printf("IN PROCESS %d THE PAGE RANK OF NODE %d IS : %0.50f \n", rank, i , page_ranks[i]);
            }*/
            
            iterate = page_ranks[n];
        }

  }

  MPItime_end = MPI_Wtime();


  MPI_Finalize();

  if(rank == MASTER){
    printf("%d iterazioni \n ",count);
    printf ("Tempo di esecuzione (secondi): %f\n", MPItime_end - MPItime_start);
    for (int i = 0; i < n; i++){
        printf("THE PAGE RANK OF NODE %d IS : %0.50f \n", i , page_ranks[i]);
    }
  }

  return 0;
}
