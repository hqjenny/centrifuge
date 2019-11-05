mpirun --allow-run-as-root  -np 1 --hostfile hostfile_1 ./test_matrix 128 128 0
mpirun --allow-run-as-root  -np 4 --hostfile hostfile_4 ./test_matrix 128 256 0
mpirun --allow-run-as-root  -np 1  ./sp_matrix 512 512 0
mpirun --allow-run-as-root  -np 1  ./sp_matrix 512 512 0
mpirun --allow-run-as-root  -np 1  ./sp_matrix 1024 1024 0
