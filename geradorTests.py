import random
import string

#how to use:
#terminal: python3 geradorTests.py > ./myTest/test.job


MAX_STRING_LENGTH = 5

def letra_aleatoria():
    return random.choice(string.ascii_lowercase)

def string_aleatoria(max_length):
    length = random.randint(1, max_length)
    return ''.join(random.choice(string.ascii_lowercase) for _ in range(length))

def main():
    for _ in range(10):
        #if random.randint(0, 10000) == 1234: print("BACKUP")
        print(f"WRITE [({string_aleatoria(MAX_STRING_LENGTH)},{string_aleatoria(MAX_STRING_LENGTH)})]")
    #for _ in range(10):
        #print("BACKUP")
    print("SHOW")
    
main()