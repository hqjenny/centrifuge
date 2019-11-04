#define ARRAY_SIZE (1 << EXP)
#define LOCAL_SIZE (1 << LIMIT)

void compare(int* pos0, int* pos1, int dir){
#pragma HLS INLINE
  int p0 = *pos0, p1= *pos1;
  if((p0 > p1) != dir){
    *pos0 = p1;
    *pos1 = p0;
  }
}

#ifdef DEBUG
void print(int* array){
  for(int i = 0;i<ARRAY_SIZE;i++){
    cout<<"array["<<i<<"]="<<array[i]<<'\t';
  }
  cout<<endl;
}
#endif // DEBUG

void sortLocal(int array[ARRAY_SIZE], int dir, int id){
#pragma HLS INLINE
  int localArray[LOCAL_SIZE];
#pragma HLS ARRAY_PARTITION variable=localArray complete dim=1
  //memcpy(localArray, array + id * LOCAL_SIZE, LOCAL_SIZE * sizeof(int));
  int i;
  int base = id * LOCAL_SIZE;
  for (i = 0; i < LOCAL_SIZE; i +=2 ){
    localArray[i+1] = array[base + i+1];
    localArray[i] = array[base + i];
  }

  for(int bits = 1; bits <= LIMIT; bits ++){
    for(int stride = bits - 1; stride >= 0; stride--){
      for(int i=0; i< LOCAL_SIZE/2; i++){
//#pragma HLS UNROLL factor = 4
#pragma HLS PIPELINE
        int l_dir = ((i >> (bits - 1)) & 0x01) ;
        l_dir = dir ^ (l_dir || ((id & 0x01) && bits == LIMIT));
        int step = 1 << stride;
        int POS = i *2 - (i & (step - 1));
        compare(localArray + POS, localArray + POS + step, l_dir);
      }
    }
  }
  //memcpy(array + id * LOCAL_SIZE, localArray, LOCAL_SIZE * sizeof(int));
  for (i = 0; i < LOCAL_SIZE; i +=2 ){
      array[base + i+1] = localArray[i+1];
      array[base + i] = localArray[i];
  }

}

void mergeLocal(int* array, int bits, int id, int dir){
#pragma HLS INLINE
  int localArray[LOCAL_SIZE];
#pragma HLS ARRAY_PARTITION variable=localArray complete dim=1
  memcpy(localArray, array + id * LOCAL_SIZE, LOCAL_SIZE * sizeof(int));

  for(int stride = LIMIT - 1; stride >=0;stride--){
    for(int i=0; i< LOCAL_SIZE/2; i++){
#pragma HLS PIPELINE
      int index = i + id * LOCAL_SIZE/2;
      int l_dir = ((index >> (bits - 1)) & 0x01) ^ dir;
      int step = 1 << stride;
      int POS = i * 2 - (i & (step - 1));
      compare(localArray + POS, localArray + POS + step, l_dir);
    }
  }
  memcpy(array + id * LOCAL_SIZE, localArray, LOCAL_SIZE * sizeof(int));
}

void sort(int* array, int dir){
#pragma HLS INTERFACE ap_bus depth=10 port=array
 
#pragma HLS INLINE
  int iter = (ARRAY_SIZE >> LIMIT);
  for(int id = 0; id < iter;id++){
    sortLocal(array, dir, id);
  }
#ifndef LOCAL
  for(int bits = LIMIT + 1; bits <= EXP; bits ++){
    for(int stride = bits - 1; stride >= 0; stride--){
      for(int i=0; i< ARRAY_SIZE/2; i++){
#pragma HLS PIPELINE
        int l_dir = ((i >> (bits - 1)) & 0x01);
        l_dir = l_dir ^ dir;
        int step = 1 << stride;
        int POS = i *2 - (i & (step - 1));
        /*
        T p0 = array[POS], p1 = array[POS + step];
        compare(p0, p1, dir);
        array[POS] = p0;
        array[POS + step] = p1;
        */
        compare(array + POS, array + POS + step, l_dir);
      }
    }
  }
#else
  for(int bits = LIMIT + 1; bits <= EXP; bits ++){
    for(int stride = bits - 1; stride >= LIMIT; stride--){
      for(int i=0; i< ARRAY_SIZE/2; i++){
#pragma HLS PIPELINE
        int l_dir = ((i >> (bits - 1)) & 0x01);
        l_dir = l_dir ^ dir;
        int step = 1 << stride;
        int POS = i *2 - (i & (step - 1));
        /*
        T p0 = array[POS], p1 = array[POS + step];
        compare(p0, p1, dir);
        array[POS] = p0;
        array[POS + step] = p1;
        */
        compare(array + POS, array + POS + step, l_dir);
      }
    }
    for(int id = 0; id < iter;id++){
      mergeLocal(bits, id, dir);
    }
  }
#endif
}



