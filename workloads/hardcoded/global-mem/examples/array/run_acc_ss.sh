mpirun --allow-run-as-root --hostfile hostfile_1 -np 1  ./sp_matrix 1024 1024 $1 
mpirun --allow-run-as-root --hostfile hostfile_4 -np 4  ./sp_matrix 512 1024 $1 
