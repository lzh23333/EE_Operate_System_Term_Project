import sys
import random
import numpy

consumer_num = int(sys.argv[1])
max_del_time = 8
max_serve_time = 40
last_enter_time = 0

print('consumer number  = ' + str(consumer_num))
txt_table = []


for i in range(consumer_num):
    #txt_table.append([i+1,random.randint(1,max_del_time)+last_enter_time,random.randint(1,max_serve_time)])
    txt_table.append([i+1,random.randint(1,5),random.randint(1,10)])
    last_enter_time = txt_table[-1][1]

file = open('consumer.txt','w')
for line in txt_table:
    for i in range(3):
        file.write(str(line[i]) + '\t')
    file.write('\r\n')
file.close()