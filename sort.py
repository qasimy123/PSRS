# Sort a list of 8 million items and time it

import time

l = [i for i in range(8000000)]
import random
random.shuffle(l)
# print(l)
start = time.time()
l.sort()
end = time.time()
print(end - start)