PAGE RANK ALGORITM

Core assumpition: 

  -In order to save space  we have a sparse matrix saved as array of pointer to a list.
  -We use this huge model of web 
   https://snap.stanford.edu/data/web-NotreDame.html
   and so we have implemented a specific way to read this file
  
  
#Page_Rank_SERIAL.c =  PageRank serial implementation according to google's algoritm 
https://www.youtube.com/watch?v=E9aoTVmQvok&list=PLLssT5z_DsK9JDLcT8T62VtzwyW9LNepV&index=12 
for more info.


#Page_Rank_MPI.c = a parallel implementation using MPI library; we choose a master/worker architetture to distribuite the load
