#include <stdio.h>
#include <stdlib.h>
#include<string.h>
#include <mpi.h>
//#include <papi.h>

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
  //char filename[] = "./DEMO.txt";    /* file containing the list of the edges */
  //char filename[] = "./web-NotreDame.txt";    /* file containing the list of the edges */
  FILE *fp;                             /* file pointer */
  char ch;                              /* reads the characters in the file */
  char str[100];                        /* buffer for storing file lines */

  // Variables for page rank algorithm
  int n, e;                             /* n: number of nodes   e: number of edges */
  int rows_num;                         /* number of rows managed by each process */
  int remaining_rows;                   /* remainder of the division n/numtasks */
  int* info;                          /* buffer for sending information from master to workers */
  int fromnode, tonode;                 /* fromnode: start node of an edge    tonode: end node of an edge */
  int* out_degree;                      /* vector containing the out_degree of all nodes */
  float* local_sub_page_ranks;          /* sub page_ranks vector of a process */
  float* complete_page_ranks;           /* complete page_rankes vector, all processes have their copy */
  float* complete_page_ranks_buf;
  float teleport_probability;           /* probability of the random walker to teleport on a random node */ 
  Node** sparse_matrix_local;           /* sparse matrix containing only the rows of the transition matrix managed by the process*/
  Node *pointer;                        /* iterates over sparse_matrix_local */        
  int iterate;                          /* flag: checks if the algorithm is converging or not*/
  float score_norm = 0;                 /* difference between two consecutive page ranks */
  float local_score_norm = 0;           /* difference between two consecutive page ranks in the single process */
  float diff;                           /* difference between two elements of consecutive page ranks */

  int min_rows_num ;
  int max_rows_num;
  float* minarray;
  float* maxarray;

  // Variable for performance measures
  double wallClock_start, wallClock_stop;
  /*long_long papi_Time_start , papi_Time_stop;
  long_long countCacheMiss;
	int EventSet = PAPI_NULL;*/

  double MPItime_start,MPItime_end;
  int firstnode;

  
  
  //inizializzo la libreria PAPI
  //if (PAPI_library_init(PAPI_VER_CURRENT) != PAPI_VER_CURRENT) {
  //  printf("Errore init PAPi\n");
  //  exit(1);
  //}

  // creo un EvntSet per PAPI
  //if (PAPI_create_eventset(&EventSet) != PAPI_OK) {
  //  printf("Errore creazione eventset PAPi\n");
  //  exit(1);
  //}

		//EventSet??-- intero per un set di eventi (PAPI_create_eventset)
		//EventCode??-- un evento definito (cache miss di L2)
  //if (PAPI_add_event(EventSet,PAPI_L2_TCM) != PAPI_OK){
  //  	printf("Errore nell'aggiunta dell'evento\n");
  //  	exit(1);
  //}

  // MPI initialization
  MPI_Init(&argc,&argv);
  MPI_Comm_size(MPI_COMM_WORLD,&numtasks);
  MPI_Comm_rank(MPI_COMM_WORLD,&rank);
  


  info = malloc ( 2 * sizeof(int));  

  MPI_Request request_receivePr_master[numtasks-1];
  MPI_Request request_sendPr_master[numtasks];

  MPI_Request request_sendPr_worker;
  MPI_Request request_receivePr_worker;

  int* flag_rec_Pr = malloc(sizeof(int));
  
  int* flag_send_Pr;

  int recv_master_state = 0;
  int send_worker_state = 0;
  

  





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
        //fprintf(stderr, "[Error] cannot open file");
        exit(1);
      }

      // Read the data set and get the number of nodes (n) and edges (e)
      
      ch = getc(fp);

      while (ch == '#'){
        fgets(str, 100 - 1, fp);
        // Debug: print title of the data set
        //printf("%s", str);
        sscanf(str, "%*s %d %*s %d", &n, &e); // number of nodes and edges
        ch = getc(fp);
      }
      
      ungetc(ch, fp);

      // DEBUG: Print the number of nodes and edges, skip everything else
      //printf("DEBUG MASTER :\nGraph data:\n\n  Nodes: %d, Edges: %d \n\n", n, e);

      // distribuiamo ipotizzando che l'indegree dei nodi ?? +/- bilanciato
      
      //rows_num = n/numtasks;
      min_rows_num = n/numtasks;
      max_rows_num = min_rows_num + 1;
      
      minarray = malloc((min_rows_num+1) * sizeof(float));

      maxarray = malloc((max_rows_num + 1) * sizeof(float));

      remaining_rows = n%numtasks;
      
    
      info[1] = n;

      //printf("DEBUG %d  invio il %d n nodi ai worker \n",rank,info[1]);

      //Prendo il wall clock time
		  //papi_Time_start = PAPI_get_real_usec();
      //MPItime_start = MPI_Wtime();

      int special_Process = remaining_rows - 1; // ultimo processo con rows + 1 (e non rows )

      // Send the rows number and the number of nodes to all WORKER
      for(int i = 1; i < numtasks; i++){
        if(i < remaining_rows) {       
          info[0] = max_rows_num;
          //printf("DEBUG: MASTER SEND NROW %d AND N TO WORKER %d\n",info[0],i);


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

      //printf("DEBUG: MASTER HAS %d ROW\n",rows_num);

      sparse_matrix_local = malloc(rows_num * sizeof(Node *));
      for (int k = 0; k < rows_num; k++){
          sparse_matrix_local[k] = NULL;
      }



  }   
  // WORKER : receive the values of the edges
  else{
       
    
    MPI_Recv(info, 2, MPI_INT, MASTER, TAG, MPI_COMM_WORLD,
                    MPI_STATUS_IGNORE);
    rows_num = info[0];

    sparse_matrix_local = malloc(rows_num * sizeof(Node *)); 
    
    n = info[1];
    
    //printf("DEBUG: WORKER %d RECEIVED %d rows  and %d as n\n",rank,info[0],info[1]);

    
    for (int k = 0; k < rows_num; k++){
      sparse_matrix_local[k] = NULL;
    }

  }

  // Creation of the array for the out_degree of all nodes
  
  //printf("DEBUG i'm %d and i'm going to allocate %d column\n",rank,n);

  out_degree = malloc(n * sizeof(int)); // Lo fanno tutti, master e workers

  // Creation of the sub page_ranks vector for a process
  local_sub_page_ranks = malloc((rows_num + 1) * sizeof(float));
  
  // Creation of the complete page_ranks vector
  complete_page_ranks = malloc((n+1) * sizeof(float)); // versione mpi
  complete_page_ranks_buf = malloc((n+1) * sizeof(float));

  //Creation of the sparse matrix
  //Node *sparse_matrix[n]; DA CANCELLARE

  //printf("DEBUG:%d INITIALIZATION of outdegree \n",rank);
  for (int k = 0; k < n; k++){
    //in_degree[k] = 0; DA CANCELLARE
    out_degree[k] = 0;
    //sparse_matrix[k] = NULL; DA CANCELLARE
  }

  info[0] = 0; // from node (-1 se finisce file)
  info[1] = 0; // to node (-1 se finisce file)
  
  if (rank == MASTER){
    //printf("\n");
    //printf("DEBUG: Master continue to  READ FILE\n");

    while (!feof(fp)){
      
      fscanf(fp, "%d%d", &fromnode, &tonode);

      if(firstnode == 1){
        fromnode=fromnode -1 ;
        tonode=tonode-1;
      }

      
      info[0] = fromnode;
      info[1] = tonode;
      int dest = tonode % numtasks; // numtasks numero processi

      if (dest != MASTER){

        //printf("DEBUG: MASTER SEND edge %d %d to %d\n",info[0],info[1],dest);
        // We can do it asynchronous because master doesn't need to wait, it can
        // rewrite the info variable
        MPI_Send(info, 2, MPI_INT, dest, TAG, MPI_COMM_WORLD);

      }
      else{

        Node * NuovoArco =  (struct Node *)malloc(sizeof(Node));
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
     
     /*
    printf("\nDEBUG: SPARSE MATRIX IN PROCESS %d", rank);
    for(int i= 0; i < rows_num; i++){
      printf("\nRANK %d   ROW: %d\n",rank, i);
      pointer = sparse_matrix_local[i];
      while (pointer != NULL){  
        printf("Start node: %d\nEnd node: %d\nValue: %f\n", pointer->start_node, pointer->end_node, pointer ->value);
        pointer = pointer->next;
      }
    }*/

    // Send the edges
    info[0] = -1;
    info[1] = -1;
    for(int i = 1; i < numtasks; i++){
      MPI_Send(info, 2, MPI_INT, i, TAG, MPI_COMM_WORLD);
    }

    // Mettere barrier?
    // Send the out_degree array
    //  printf("DEBUG %d INVIO OUT DEGREE\n",rank);
    for(int i = 1; i < numtasks; i++){
        MPI_Send(out_degree, n, MPI_INT, i, TAG, MPI_COMM_WORLD);
    }
    
   } //fine if master
   else{  // WORKER receive edge code
      
    // Receive the new edge (almeno 1) 
    //receive(0, info)
    MPI_Recv(info, 2, MPI_INT, MASTER, TAG, MPI_COMM_WORLD,
            MPI_STATUS_IGNORE);
    //printf("DEBUG: %d WORKER HAS RECIVED %d - %d edge\n",rank,info[0],info[1]);

    // Check if the master doesn't reach the eof
    while(info[0] != -1 && info[1] != -1){
      
      fromnode = info[0];
      tonode = info[1];

      Node * NuovoArco =(struct Node *) malloc(sizeof(Node));
      NuovoArco->start_node = fromnode;
      NuovoArco->end_node = tonode;
      NuovoArco->value = 1;


      // INSERIMENTO IN TESTA
      NuovoArco->next = sparse_matrix_local[(tonode - rank) / numtasks];
      sparse_matrix_local[(tonode - rank) / numtasks] = NuovoArco;

      // Receive the new edge
      //receive(0, info)
      //printf("DEBUG %d: RICEVO  ARCO\n",rank);

      MPI_Recv(info, 2, MPI_INT, 0, TAG, MPI_COMM_WORLD,MPI_STATUS_IGNORE);
      
      //printf("DEBUG: %d WORKER HAS RECIVED %d - %d edge\n",rank,info[0],info[1]);


    }

      /*
      printf("\nDEBUG: SPARSE MATRIX IN PROCESS %d\n", rank);
      for(int i= 0; i < rows_num; i++){
        printf("\nRANK %d   ROW: %d\n",rank, i);
        pointer = sparse_matrix_local[i];
          while (pointer != NULL){  
            printf("Start node: %d\nEnd node: %d\nValue: %f\n", pointer->start_node, pointer->end_node, pointer ->value);
            pointer = pointer->next;
          }
      }
      */
      //receive(0, out_degree);
    MPI_Recv(out_degree, n, MPI_INT, MASTER, TAG, MPI_COMM_WORLD,
            MPI_STATUS_IGNORE);      
      //printf("DEBUG: %d WORKER HAS RECIVED the outdegree\n", rank);

  }

  /*
    DISTRIBUZIONE ARCHI ROUND ROBIN, USANDO MODULO;
    MOLTIPLICAZIONE RIGA PER COLONNA FATTA DA OGNI WORKER, CHE NON COSTRUISCE UN ARRAY MA UNA MATRICE (chiamata my_page_ranks),
    DA INVIARE AL MASTER: [POSIZIONE IN page_ranks COMPLETO, VALORE]
  */
 
 


  //printf("DEBUG: %d INITIALIZE PAGE RANKS TO 1/N and UPDATE MATRIX\n",rank);
  if(rank==MASTER)
    MPItime_start = MPI_Wtime();

  teleport_probability = (1 - WEIGHT) / (float)n;
  // Same code MASTER and WORKER

  for (int i = 0; i < n; i++){

  // old_page_rank ?? il page rank completo, ricevuto dal master, new_page_ranks ?? invece
  // il sotto-array di page rank calcolato dal worker e ha dimensione size.
  // Per fare la differenza, il worker accede in old_page_ranks a partire dal proprio rank(nel senso proprio id) e saltando di numtasks
    if(i<rows_num){
      local_sub_page_ranks[i] = 1.0 / (float)n;
      
      
      pointer = sparse_matrix_local[i];

      // Update the value of the pointer
      while (pointer != NULL){  
        pointer->value = (WEIGHT / (float)out_degree[pointer->start_node]);
        pointer = pointer->next;
      }
      
    }
      

    complete_page_ranks[i] = 1.0 / (float)n;
    
  }

    



  iterate = 1;
  //printf("DEBUG: %d Start to iterete\n",rank);
  while(iterate ){

    local_score_norm = 0;
    
    
    // CALCOLO DELLA MIA PARTE
    for (int i = 0,k=rank; i < rows_num;i++){

      float sum = 0.0;
      Node *currNode = sparse_matrix_local[i];
      //printf("DEBUG: %d currnode allocate\n",rank);

      while (currNode!=NULL){

        sum += (complete_page_ranks[currNode->start_node] * currNode->value);
        
        currNode = currNode->next;

      } 

      //printf("DEBUG: total sum %f\n", sum);

      //printf("DEBUG: %d local_sub_page_ranks update\n",rank);
      // somma con colonna costante teleport_probability
      local_sub_page_ranks[i] = sum + teleport_probability;
      
      // take the absolute value of the error

      //printf("PROCESS %d row %d from %f to %f\n",rank,k,complete_page_ranks[k],local_sub_page_ranks[i]);

      diff = local_sub_page_ranks[i] - complete_page_ranks[k];
      if (diff < 0)
        diff = -diff;

      //printf("DEBUG: diff %f\n", diff);
      // sum to the score_norm

      float temp = local_score_norm + diff;
      //printf("PROCESS %d from %f to %f\n",rank,local_score_norm,temp);
      local_score_norm += diff;

      //printf("DEBUG: local score norm %f\n", local_score_norm);

      //printf("DEBUG: %d end iteration\n",rank);
      // update the round robin index for moving in complete_page_ranks
      k += numtasks;

    }
    
    


    // MASTER: receive the local page rank from WORKER, update the page rank and valuete the error
    if (rank == MASTER){
  
      score_norm = local_score_norm;
      
      // Controlliamo se non siamo gia'in receive
      if(recv_master_state == 0){

          // Avviamo le receive asincrone
          for (int sender_rank = 1 ; sender_rank < numtasks;sender_rank++){

              if(sender_rank < remaining_rows){     

                  // Riceviamo asincronamente il pagerank e lo scorenorme    
                  MPI_Irecv(maxarray,max_rows_num + 1, MPI_FLOAT, sender_rank, TAG, MPI_COMM_WORLD, &request_receivePr_master[sender_rank-1]);
                  
              }else{
                            
                  MPI_Irecv(minarray,min_rows_num+1, MPI_FLOAT, sender_rank, TAG, MPI_COMM_WORLD, &request_receivePr_master[sender_rank-1]);

              }
          }

          // update comunication state
          recv_master_state = 1;
      } 

      
      // Aggiornamento PageRank completo da fare solo se sono arrivati tutti i valori attesi daai WORKER
      MPI_Testall(numtasks-1, request_receivePr_master, flag_rec_Pr, MPI_STATUSES_IGNORE);
      if( flag_rec_Pr ){ //verifico se mi sono arrivati tutti i pezzi

        // update comunication state
        // Possiamo rimetterci in ricezione
        recv_master_state = 0;

        // Aggiorniamo i valori che ci sono arrivati dai WORKER
        for(int sender_rank = 1 ; sender_rank < numtasks;sender_rank++){
            
            if(sender_rank < remaining_rows){     

              // Riceviamo asincronamente il pagerank e lo scorenorme    
              for( int k=0,i = sender_rank; k<max_rows_num  ; k++){
                  complete_page_ranks[i] = maxarray[k];
                  i=i+numtasks;
              }

              score_norm += maxarray[max_rows_num];

            }else{

              for( int k=0, i= sender_rank; k<min_rows_num ; k++){
                
                complete_page_ranks[i] = minarray[k];
                i=i+numtasks;

              }

              score_norm += minarray[min_rows_num];
              
            }

        }
        
        if(score_norm <= ERROR)  {
          iterate = 0;
        }

        complete_page_ranks[n] = iterate;

        
        // Send the new old_page_rank value to all worker
        for(int i=1; i<numtasks; i++){
          MPI_Isend(complete_page_ranks, n+1, MPI_FLOAT, i, TAG, MPI_COMM_WORLD,&request_sendPr_master[i-1]);
        }

        // Flag dell'invio della send
        //printf("DEBUG: %d TEST  SEND_MASTER\n",rank);
      }

      printf("DEBUG: %d TEST  iterate: %d\n",rank,iterate);
      /*
      if(score_norm <= ERROR)  {
          iterate = 0;
      }
      */
      // Aggiorniamo PageRank con i valori locali:  DA FARE SEMPRE
      for (int i = 0,k = 0 ; i<rows_num;i++){ 
          complete_page_ranks[k]=local_sub_page_ranks[i];
          k += numtasks;
      }
      
      

    }
    // WORKERS send the new page_rank values
    else{
      
      //  DEVONO AGGIORNARE SOLO I VALORI CHE NON MODIFICANO DURANTE LE ITERAZIONI
      //  PERCHE' QUEI VALORI SONO I PIU' AGGIORNATI
      local_sub_page_ranks[rows_num]=local_score_norm;
      

      if(send_worker_state == 0){
        
        MPI_Isend(local_sub_page_ranks, rows_num+1, MPI_FLOAT, 0, TAG, MPI_COMM_WORLD,&request_sendPr_worker); 
        
        MPI_Irecv(complete_page_ranks_buf, n+1, MPI_FLOAT, 0, TAG, MPI_COMM_WORLD,&request_receivePr_worker);

        // Update comunication state
        send_worker_state = 1;
      }
      
      

      //Controllo se ho ricevuto il nuovo complete pagerank
      MPI_Test(&request_receivePr_worker, flag_rec_Pr,MPI_STATUSES_IGNORE);
      if(flag_rec_Pr){

        iterate = complete_page_ranks_buf[n];

        //MOLTO BRUTTO
        // update complete page rank
        if(iterate)  
          for(int i=0;i<n;i++)
            complete_page_ranks[i]=complete_page_ranks_buf[i];

        //possiamo ricevere un nuovo pagerank
        send_worker_state = 0;

        //printf("DEBUG: %d TEST  SEND_WORKER\n",rank);
      }

      printf("DEBUG: %d TEST  iterate: %d\n",rank,iterate);

      // Aggiorniamo PageRank con i valori locali:  DA FARE SEMPRE
      for (int i = 0,k = rank ; i<rows_num;i++){ 
          complete_page_ranks[k]=local_sub_page_ranks[i];
          k += numtasks;
      }

    }
    

  }

  
  //clock= finish();
  MPItime_end = MPI_Wtime();


  //DEBUG
  MPI_Barrier(MPI_COMM_WORLD);

  MPI_Finalize();
  
  

  if(rank == MASTER){
    //papi_Time_stop = PAPI_get_real_usec();
    
    //printf("DEBUG: %d END PROCESS:\n",rank);
    // Print the results
    
    
    /*for (int i = 0; i < n; i++){
      printf("DEBUG: %d THE PAGE RANK OF NODE %d IS : %0.50f \n",rank, i , complete_page_ranks[i]);
    }
    */
    printf ("Tempo di esecuzione (secondi): %f\n", MPItime_end - MPItime_start);
    
    //printf ("Tempo di esecuzione PAPI (microsecondi): %d\n",papi_Time_stop - papi_Time_start);
  }



  return 0;

}
