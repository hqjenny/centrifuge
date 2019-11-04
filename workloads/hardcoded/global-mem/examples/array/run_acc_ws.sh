mpirun --allow-run-as-root --hostfile hostfile_1 -np 1  ./sp_matrix 256 256 $1 
mpirun --allow-run-as-root --hostfile hostfile_4 -np 1  ./sp_matrix 256 512 $1 
