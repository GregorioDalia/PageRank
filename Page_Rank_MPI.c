#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

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
#define MASTER 0    // Rank of the master process
#define TAG 0       /* MPI message tag parameter */


int main(int argc, char *argv[]){

  // Variables for MPI communication              
  int numtasks;               /* number of MPI tasks */
  int rank;                   /* my MPI task number */
  int rc;                     /* return code */
  MPI_Status status;          /* MPI receive routine parameter */

  // Variables for input file reading
  char filename[] = "./DEMO.txt";    /* file containing the list of the edges */
  //char filename[] = "./DEMO.txt";       /* file containing the list of the edges */
  FILE *fp;                             /* file pointer */
  char ch;                              /* reads the characters in the file */
  char str[100];                        /* buffer for storing file lines */

  // Variables for page rank algorithm
  int n, e;                             /* n: number of nodes   e: number of edges */
  int rows_num;                         /* number of rows managed by each process */
  int remaining_rows;                   /* remainder of the division n/numtasks */
  int info[2];                          /* buffer for sending information from master to workers */
  int fromnode, tonode;                 /* fromnode: start node of an edge    tonode: end node of an edge */
  int* out_degree;                      /* vector containing the out_degree of all nodes */
  float* local_sub_page_ranks;          /* sub page_ranks vector of a process */
  float* complete_page_ranks;           /* complete page_rankes vector, all processes have their copy */
  float teleport_probability;           /* probability of the random walker to teleport on a random node */ 
  Node** sparse_matrix_local;           /* sparse matrix containing only the rows of the transition matrix managed by the process*/
  int iterate;                          /* flag: checks if the algorithm is converging or not*/
  float score_norm;                     /* difference between two consecutive page ranks */
  float local_score_norm;                     /* difference between two consecutive page ranks */

  
  // MPI initialization
  MPI_Init(&argc,&argv);
  MPI_Comm_size(MPI_COMM_WORLD,&numtasks);
  MPI_Comm_rank(MPI_COMM_WORLD,&rank);
  


  // Variable for performance measures
  //PAPI_get_real_usec();


  // MASTER CODE : read the input file
  if(rank == MASTER){ 
      
      printf("DEBUG: MASTER open the file %s",filename);

      if ((fp = fopen(filename, "r")) == NULL){
        fprintf(stderr, "[Error] cannot open file");
        exit(1);
      }

      // Read the data set and get the number of nodes (n) and edges (e)
      
      ch = getc(fp);

      while (ch == '#'){
        fgets(str, 100 - 1, fp);
        // Debug: print title of the data set
        printf("%s", str);
        sscanf(str, "%*s %d %*s %d", &n, &e); // number of nodes and edges
        ch = getc(fp);
      }
      
      ungetc(ch, fp);

      // DEBUG: Print the number of nodes and edges, skip everything else
      printf("DEBUG MASTER :\nGraph data:\n\n  Nodes: %d, Edges: %d \n\n", n, e);

      // distribuiamo ipotizzando che l'indegree dei nodi è +/- bilanciato
      
      rows_num = n/numtasks;
      remaining_rows = n%numtasks;
      if(remaining_rows != 0){
        rows_num ++;
      }

      printf("DEBUG: MASTER HAS %d ROW\n",rows_num);

      sparse_matrix_local = malloc(rows_num * sizeof(Node* ));
      for (int k = 0; k < rows_num; k++){
          sparse_matrix_local[k] = NULL;
      }
    
      info[1] = n;

      //printf("DEBUG %d  invio il n nodi ai worker \n",rank);

      // Send the rows number and the number of nodes to all WORKER
      for(int i = 1; i < numtasks; i++){
        if(i < remaining_rows) {       
          info[0] = rows_num + 1;
          printf("DEBUG: MASTER SEND NROW %d AND N TO WORKER %d",info[0],i);

          MPI_Send(info, 2, MPI_INT, i, TAG, MPI_COMM_WORLD);

        }
        else{
          info[0] = rows_num;
          printf("DEBUG: MASTER SEND NROW %d AND N TO WORKER %d",info[0],i);

          MPI_Send(info, 2, MPI_INT, i, TAG, MPI_COMM_WORLD);

        }
      }
  }   
  // WORKER : receive the values of the edges
  else{
       
    
    MPI_Recv(&info, 2, MPI_INT, MASTER, TAG, MPI_COMM_WORLD,
                    &status);
    rows_num = info[0];

    sparse_matrix_local = malloc(rows_num * sizeof(Node* )); 
    
    int n = info[1];
    
    printf("DEBUG: WORKER %d RECEIVED %d rows  and %d as n\n",info[0],info[1]);

    
    for (int k = 0; k < rows_num; k++){
      sparse_matrix_local[k] = NULL;
    }

  }


  // Creation of the array for the out_degree of all nodes
  
  printf("DEBUG i'm %d and i'm going to allocate %d colon",rank,n);

  out_degree = malloc(n * sizeof(int)); // Lo fanno tutti, master e workers

  // Creation of the sub page_ranks vector for a process
  local_sub_page_ranks = malloc(rows_num * sizeof(float));
  
  // Creation of the complete page_ranks vector
  complete_page_ranks = malloc(n * sizeof(float)); // versione mpi
  

  //Creation of the sparse matrix
  //Node *sparse_matrix[n]; DA CANCELLARE

  printf("DEBUG:%d INITIALIZATION of outdegree \n",rank);
  for (int k = 0; k < n; k++){
    //in_degree[k] = 0; DA CANCELLARE
    out_degree[k] = 0;
    //sparse_matrix[k] = NULL; DA CANCELLARE
  }

  info[0] = 0; // from node (-1 se finisce file)
  info[1] = 0; // to node (-1 se finisce file)
  
  if (rank == MASTER){
    printf("\n");
    printf("DEBUG: Master continue to  READ FILE\n");

    while (!feof(fp)){
      
      fscanf(fp, "%d%d", &fromnode, &tonode);
      
      info[0] = fromnode;
      info[1] = tonode;
      int dest = fromnode % numtasks; // numtasks numero processi

      if (dest != 0){

        printf("DEBUG: MASTER SEND edge %d %d to %d\n",info[0],info[1],dest);

        MPI_Send(info, 2, MPI_INT, dest, TAG, MPI_COMM_WORLD);

      }
      else{

        Node *NuovoArco = (struct Node *)malloc(sizeof(Node));
        NuovoArco->start_node = fromnode;
        NuovoArco->end_node = tonode;
        NuovoArco->value = 1;

        // INSERIMENTO IN TESTA
        NuovoArco->next = sparse_matrix_local[(NuovoArco->end_node) % rows_num];
        sparse_matrix_local[NuovoArco->end_node % rows_num] = NuovoArco;
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

    // Mettere barrier?
    // Send the out_degree array
    printf("DEBUG %d INVIO OUT DEGREE",rank);
    for(int i = 1; i < numtasks; i++){
        MPI_Send(out_degree, n, MPI_INT, i, TAG, MPI_COMM_WORLD);
    }
    
   } //fine if master
   else{  // WORKER receive edge code
      
      // Receive the new edge (almeno 1) 
      //receive(0, info)
      MPI_Recv(info, 2, MPI_INT, MASTER, TAG, MPI_COMM_WORLD,
             MPI_STATUS_IGNORE);

      printf("DEBUG: $d WORKER HAS RECIVED %d - %d edge\n",info[0],info[1]);

      // Check if the master doesn't reach the eof
      while(info[0] != -1 && info[1] != -1){
        

        Node *NuovoArco = (struct Node *)malloc(sizeof(Node));
        NuovoArco->start_node = fromnode;
        NuovoArco->end_node = tonode;
        NuovoArco->value = 1;


        // INSERIMENTO IN TESTA
        NuovoArco->next = sparse_matrix_local[NuovoArco->end_node % rows_num];
        sparse_matrix_local[NuovoArco->end_node % rows_num] = NuovoArco;

        // Receive the new edge
        //receive(0, info)
        printf("DEBUG %d: RICEVO  ARCO\n",rank);

        MPI_Recv(info, 2, MPI_INT, 0, TAG, MPI_COMM_WORLD,
             MPI_STATUS_IGNORE);
        
        printf("DEBUG: $d WORKER HAS RECIVED %d - %d edge\n",info[0],info[1]);


      }

      //receive(0, out_degree);
      MPI_Recv(out_degree, n, MPI_INT, 0, TAG, MPI_COMM_WORLD,
             MPI_STATUS_IGNORE);
      
      printf("DEBUG: $d WORKER HAS RECIVED the outdegree\n");

  }

  /*
    DISTRIBUZIONE ARCHI ROUND ROBIN, USANDO MODULO;
    MOLTIPLICAZIONE RIGA PER COLONNA FATTA DA OGNI WORKER, CHE NON COSTRUISCE UN ARRAY MA UNA MATRICE (chiamata my_page_ranks),
    DA INVIARE AL MASTER: [POSIZIONE IN page_ranks COMPLETO, VALORE]
  */




  printf("DEBUG: %d INITIALIZE PAGE RANKS TO 1/N and UPDATE MATRIX\n",rank);
  
  // Same code MASTER and WORKER
  for (int i = 0; i < n; i++){

  // old_page_rank è il page rank completo, ricevuto dal master, new_page_ranks è invece
  // il sotto-array di page rank calcolato dal worker e ha dimensione size.
  // Per fare la differenza, il worker accede in old_page_ranks a partire dal proprio rank(nel senso proprio id) e saltando di numtasks
    if(i<rows_num){
      local_sub_page_ranks[i] = 1.0 / (float)n;
      teleport_probability = (1 - WEIGHT) / (float)n;
      
      Node *pointer = sparse_matrix_local[i];

      // Update the value of the pointer
      while (pointer != NULL){  
        pointer->value = (WEIGHT / (float)out_degree[pointer->start_node]);
        pointer = pointer->next;
      }
      
    }
      

    complete_page_ranks[i] = 1.0 / (float)n;
    
  }
  
     

  // Matrix moltiplication : same code MASTER WORKER
  // receive the score_values from workers      send the score_norm  and result value to MASTER : WORKER
  // Error valutation : MASTER                  WORKER: wait the message:
      // TRUE CONDITION :                           receive page rank update 
      //      send page rank : MASTER               iterate
      //      iterate
      // FALSE CONDITION :
      //      stop iteration and workers
      //      end
  
  int count = 0;

  //sono il master
  //if(rank == 0)
  iterate = 1;
  printf("DEBUG: %d Start to iterete\n",rank);
  while(iterate){

    // qui calcolo la mia parte di lavoro
    
    for (int i = 0,k=rank; i < rows_num;i++){

      float sum = 0.0;
      Node *currNode = sparse_matrix_local[i];

      do{

        sum += (local_sub_page_ranks[currNode->start_node] * currNode->value);

        currNode = currNode->next;

      } while (currNode!=NULL);

      // somma con colonna costante teleport_probability
      local_sub_page_ranks[i] = sum + teleport_probability;

      // take the absolute value of the error, using old_page_rank avoiding to create a new variable 
      complete_page_ranks[k] = local_sub_page_ranks[i] - complete_page_ranks[k];
      if (complete_page_ranks[k] < 0)
        complete_page_ranks[k] = -complete_page_ranks[k];

      // sum to the score_norm
      local_score_norm += complete_page_ranks[k];

      
      // update the round robin index for moving in complete_page_ranks
      k += numtasks;

    }

    printf("")
    // Map-Reduce for the calculation
    //reduce ( score_norm,0); 
      printf("DEBUG %d: REDUCE \n",rank);

    MPI_Reduce(&local_score_norm, &score_norm, 1, MPI_FLOAT, MPI_SUM, 0,
           MPI_COMM_WORLD);
    
    // MASTER update the page rank and valuete the error
    if (rank == MASTER){
      
      float newElem;
      // Receive the new page rank values from the workers  
      for (int i=0,x=0,sender_rank=0;i < n; i++){
        
        if(sender_rank != 0){

          MPI_Recv(&newElem, 1, MPI_FLOAT, sender_rank, TAG, MPI_COMM_WORLD,MPI_STATUS_IGNORE);

          complete_page_ranks[i]=newElem;
        }
        else{
          // Update from local value of the MASTER  
          complete_page_ranks[i]=local_sub_page_ranks[x];
          x++;
        }
        
        sender_rank++;
        if (sender_rank==numtasks)sender_rank=0;

      }

      // Evaluate the Error condition for end the iteration
      if(score_norm <= ERROR)  iterate = 0; 
      
      // Send the new old_page_rank value to all worker
      for(int i=1; i<numtasks; i++){
        
        MPI_Send(&iterate, 1, MPI_INT, i, TAG, MPI_COMM_WORLD);

        //send (iterate);
        if(iterate)   
                MPI_Send(&complete_page_ranks, n, MPI_FLOAT, i, TAG, MPI_COMM_WORLD);

                //send(i, complete_page_ranks);
      }
      


    }
    // WORKERS send the new page_rank values
    else{

      // Send the update values of page_rank
      for(int i=0; i < rows_num; i++){
            
            MPI_Send(&local_sub_page_ranks[i],1 , MPI_FLOAT, 0, TAG, MPI_COMM_WORLD);

               //send(0, local_page_ranks[i]);
      }
      
      // Receive iterate: check wether continue or not
      //receive(0, iterate);
      MPI_Recv(&iterate, 1, MPI_INT, 0, TAG, MPI_COMM_WORLD,MPI_STATUS_IGNORE);

      
      
      // Receive the old_page_rank update
      if(iterate){
        //receive(0, complete_page_ranks);
          MPI_Recv(&complete_page_ranks, 1, MPI_FLOAT, 0, TAG, MPI_COMM_WORLD,MPI_STATUS_IGNORE);

      }
      
      
    }

  }

  
  //clock= finish();
  MPI_Finalize();


  
  // Print the results
  for (int i = 0; i < n; i++){
    printf("THE PAGE RANKE OF NODE %d IS : %0.15f \n", i , complete_page_ranks[i]);
  }



  return 0;

}
