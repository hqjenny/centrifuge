
# ddnet
#(d, c, pooling, duplicate, rep)
#network = [
#(56, 116, 1, 0, 1),
#(28, 116, 0, 0, 3),
#(28, 232, 1, 1, 1),
#(14, 232, 0, 0, 7),
#(14, 464, 1, 1, 1),
#(7, 464, 0, 0, 3),
#]

network = [
(64, 128, 1, 0, 1),
(32, 128, 0, 0, 3),
(32, 256, 1, 1, 1),
(16, 256, 0, 0, 7),
(16, 512, 1, 1, 1),
(8, 512, 0, 0, 3),
]


d_sizes = []
c_sizes = []
is_ps = []
is_dup = []

i = 0
for d, c, is_pool, is_dup, rep in network:
  for j in range(rep):
    #d_sizes.append(d)
    #c_sizes.append(c)
    #is_ps.append(is_pool)
    #codegen
    print("static char fmap_{0}[REP*{1}*{1}*{2}*FM_W/8];".format(i, d, c))
    print("static char out_{0}[REP*{1}*{1}*{2}*FM_W/8];".format(i, d, c))
    print("static char k0_{0}[{1}*{1}*W_W/8];".format(i, c))
    print("static char k1_{0}[{1}*{1}*W_W/8];".format(i, c))
    print("static char k2_{0}[{1}*{1}*W_W/8];".format(i, c))
    i+=1


i = 0
for d, c, is_pool, is_dup, rep in network:
  for j in range(rep):
    #d_sizes.append(d)
    #c_sizes.append(c)
    #is_ps.append(is_pool)
    #codegen
    print("top_wrapper( fmap_{0}, out_{0}, k0_{0}, k1_{0}, k2_{0}, {1}, {2}, 0, {3}, REP);".format(i, d, c, is_pool))
    i+=1


