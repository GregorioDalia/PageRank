#include <stdio.h>
#include <stdlib.h>

//to handle the sparse matrix
typedef struct Node {

    int start_node;
    int end_node;
    float value;

    struct Node* next;
}Node;

#define WEIGHT 0.85 // Real in (0, 1), best at 0.85
#define ERROR 0.0001 // Real in (0, +inf), best at 0.0001

int main()
{
     // Open the data set
  char filename[]="./web-NotreDameDEMO.txt";
  FILE *fp;
  if((fp=fopen(filename,"r"))==NULL) {
    fprintf(stderr,"[Error] cannot open file");
    exit(1);
  }
   // Read the data set and get the number of nodes (n) and edges (e)
  int n, e;
  char ch;
  char str[100];
  ch = getc(fp);
  while(ch == '#') {
    fgets(str, 100-1, fp);
    //Debug: print title of the data set
    printf("%s",str);
    sscanf (str,"%*s %d %*s %d", &n, &e); //number of nodes
    ch = getc(fp);
  }
  ungetc(ch,fp);

  // DEBUG: Print the number of nodes and edges, skip everything else
  printf("\nGraph data:\n\n  Nodes: %d, Edges: %d \n\n", n, e);

int fromnode, tonode;
int file_Matrix[e][2];
int i = 0;
int in_degree[n];
int out_degree[n];

float page_ranks[n];
float mean_coloumn_weighed[n];

for(int k = 0;k<n;k++){
    in_degree[k]=0;
    out_degree[k]=0;
}

 while(!feof(fp)){

    fscanf(fp,"%d%d",&fromnode,&tonode);

    //DEBUG: print fromnode and tonode
    printf("From: %d To: %d\n",fromnode, tonode);

    file_Matrix[i][0] = fromnode;
    file_Matrix[i][1] = tonode;

    //use fromnode and tonode as index
    out_degree[fromnode]++;
    in_degree[tonode]++;

    i++;


  }

  //DEBUG
  for(i=0;i<n;i++){
    printf("outdegree del nodo %d e' %d\n",i,out_degree[i]);
    printf("indegree del nodo %d e' %d\n",i,in_degree[i]);

  }

for (int i = 0; i<n ; i++){
    page_ranks[i]= 1.0/(float)n;
    mean_coloumn_weighed[i] = (1-WEIGHT)/(float)n;

}

Node* sparse_matrix [n];
for (int i = 0 ; i < n ;i++){
    sparse_matrix[i]=NULL;
}


  for (int i = 0; i<e;i++){
    //qui dovremmo distribuire gli archi ai vari processi
    //distribuisci dicendogli prima di quel nodo quale è l'indegree quindi sai quanto fare il ciclo

    Node* NuovoArco = ( struct Node *)malloc(sizeof(Node));
    NuovoArco->start_node=file_Matrix[i][0];
    NuovoArco->end_node=file_Matrix[i][1];
    //printf("DEBUG OUTDEGREE %d \n",out_degree[NuovoArco->start_node]);

    NuovoArco->value = (WEIGHT / (float)out_degree[NuovoArco->start_node]);

    //NuovoArco->value = 1;

    //INSERIMENTO IN TESTA

    NuovoArco->next=sparse_matrix[NuovoArco->end_node];

    sparse_matrix[NuovoArco->end_node] = NuovoArco;

    printf("DEBUG\n");
    printf("%d",NuovoArco->end_node);
    printf("\n");


  }

/*
//DEBUG
for(int i = 0 ; i<n ;i++){

    Node* pointer=sparse_matrix[i];

    printf("Riga %d ",i);

    while(pointer != NULL){


        printf("%f",pointer->value);
        printf(" ");

        pointer=pointer->next;


    }
    printf("\n");

}

*/

float old_PageRank[n];
float score_norm ;
int count =0;

do{
    score_norm = 0;

    for(int i =0; i<n;i++){
        old_PageRank[i]= page_ranks[i];
    }

   for (int i = 0;i<n;i++){
        float sum = 0.0;

        for(int j = 0;j<in_degree[i];j++){
        // o in_degree o while curr node != null

        Node* currNode = sparse_matrix[i];

        //moltiplicazione e somma

        sum += page_ranks[currNode->end_node] * currNode->value;

        currNode =currNode->next;

        }

    page_ranks[i]= sum + mean_coloumn_weighed[i];
    //somma con colonna costante mean_coloumn_weighed

    old_PageRank[i] = page_ranks[i] - old_PageRank[i];

    if(old_PageRank[i]<0)old_PageRank[i] = -old_PageRank[i];

    score_norm += old_PageRank[i];



}

// valutare se questo far si puo mettere dentro
/*
for(int i=0;i<n;i++){

    old_PageRank[i] = page_ranks[i] - old_PageRank[i];

    if(old_PageRank[i]<0)old_PageRank[i] = -old_PageRank[i];

    score_norm += old_PageRank[i];


}*/
count++;

}while (score_norm > ERROR);
printf("quanti giri? %d\n",count);

for (int i = 0;i<n;i++){
    printf("THE PAGE RANKE IS... \n");
    printf("%f\n",page_ranks[i]);

}





    return 0;
}
