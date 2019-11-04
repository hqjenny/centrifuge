 #mpic++ sp_matrix.cpp -o sp_matrix -std=gnu++17 -lcblas64_ -I../../ -I/usr/include/cblas/ -I./ -lpthread
# /scratch/qijing.huang/firesim/sw/firesim-software/mpich-3.2.1/build/bin/mpic++ sp_matrix.cpp -g  -o sp_matrix -std=gnu++17 -I../../ -I./ -lpthread -fpermissive
mpic++ sp_matrix.cpp -g  -o sp_matrix -std=gnu++17 -I../../ -I./ -lpthread -fpermissive
mpic++ test_matrix.cpp -g  -o test_matrix -std=gnu++17 -I../../ -I./ -lpthread -fpermissive
