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
    //distribuisci dicendogli prima di quel nodo quale � l'indegree quindi sai quanto fare il ciclo

    Node* NuovoArco = ( struct Node *)malloc(sizeof(Node));
    NuovoArco->start_node=file_Matrix[i][0];
    NuovoArco->end_node=file_Matrix[i][1];
    //printf("DEBUG OUTDEGREE %d \n",out_degree[NuovoArco->start_node]);

    NuovoArco->value = (WEIGHT / (float)out_degree[NuovoArco->start_node]);

    //NuovoArco->value = 1;

    //INSERIMENTO IN TESTA

    NuovoArco->next=sparse_matrix[NuovoArco->end_node];

    sparse_matrix[NuovoArco->end_node] = NuovoArco;

    //printf("DEBUG\n");
    //printf("%d",NuovoArco->end_node);
    //printf("\n");


  }


//DEBUG
printf("MATRICE FINALE:\n");

for(int i = 0 ; i<n ;i++){

    Node* pointer=sparse_matrix[i];
    printf("Riga %d ",i);

    while(pointer != NULL){


        printf("%d - %d / %f",pointer->end_node,pointer->start_node,pointer->value);
        printf(" ");

        pointer=pointer->next;


    }
    printf("\n");

}



float old_PageRank[n];
float score_norm ;
int count =0;

do{

    printf("GIRO N: %d\n",count+1);

    score_norm = 0;

    for(int i =0; i<n;i++){
        old_PageRank[i]= page_ranks[i];
    }

   for (int i = 0;i<n;i++){
        float sum = 0.0;
        Node* currNode = sparse_matrix[i];



        int j = 0;
        do{
        printf("DEBUG: moltiplico con questo indice %d i numeri %f per %f\n",currNode->end_node,page_ranks[currNode->end_node],currNode->value);
        printf("sum ora = %f\n",sum);


        sum += (page_ranks[currNode->start_node] * currNode->value);

        printf("sum dopo = %f\n",sum);

        currNode =currNode->next;

        printf("j = %d\n",j);
        printf("max = %d\n",in_degree[i]-1);
        j++;

        }while(j<in_degree[i]);

        /*
        for(int j = 0;j<in_degree[i];j++){
        // o in_degree o while curr node != null


        //moltiplicazione e somma
        printf("DEBUG: moltiplico con questo indice %d i numeri %f per %f\n",currNode->end_node,page_ranks[currNode->end_node],currNode->value);

        sum += page_ranks[currNode->end_node] * currNode->value;

        currNode =currNode->next;

        }*/

        page_ranks[i]= sum + mean_coloumn_weighed[i];
        printf("Valore %d aggiornato finale del peso = %f\n",i,page_ranks[i]);

    //somma con colonna costante mean_coloumn_weighed


        old_PageRank[i] = page_ranks[i] - old_PageRank[i];
        printf("differenza = %f\n",old_PageRank[i]);


        if(old_PageRank[i]<0)old_PageRank[i] = -old_PageRank[i];

        score_norm += old_PageRank[i];
        printf("norma alla riga %d = %f\n",i,score_norm);




}

// valutare se questo far si puo mettere dentro
/*
for(int i=0;i<n;i++){

    old_PageRank[i] = page_ranks[i] - old_PageRank[i];

    if(old_PageRank[i]<0)old_PageRank[i] = -old_PageRank[i];

    score_norm += old_PageRank[i];


}
*/


count++;
printf("check error = %f\n",score_norm);

}while (score_norm > ERROR);


printf("quanti giri? %d\n",count);

for (int i = 0;i<n;i++){
    printf("THE PAGE RANKE IS... \n");
    printf("%f\n",page_ranks[i]);

}





    return 0;
}
