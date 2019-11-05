def main():
    configs = [1, 4, 16]
    for config in configs:
        f = open("hostfile_"+str(config), "a")
        for i in range(config):
            f.write("172.16.0.%d\n"%(i+2))

main()
