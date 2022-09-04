PAGE RANK ALGORITM

Core assumpition: 

  -In order to save space  we have a sparse matrix saved as array of pointer to a list.
  -We use this huge model of web 
   https://snap.stanford.edu/data/web-NotreDame.html
   and so we have implemented a specific way to read this file

#DEMO.txt is a really small model that we used to validate our algoritm since we already knew the results  
  
#Page_Rank_SERIAL.c =  PageRank serial implementation according to google's algoritm 
https://www.youtube.com/watch?v=E9aoTVmQvok&list=PLLssT5z_DsK9JDLcT8T62VtzwyW9LNepV&index=12 
for more info.

#Page_Rank_MPI.c = a parallel implementation using MPI library; we choose a master/worker architetture to distribuite the load
                    // THIS IS A NOT PERFORMANCE SOLUTION TOO MANY MESSAGE
                
#Page_Rank_MPI_2.c =a parallel implementation using MPI library; we choose a master/worker architetture to distribuite the load
                    OMPTIMIZED VERSION

how to run : main expect 2 arguments, the name of the file "./WEB.txt" in our case and the ID of the 1 node ( 0 or 1 )
