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
    #return letra_aleatoria()

def main():
    for _ in range(1000):
        #if random.randint(-1000, 1000) == 123: print("BACKUP")
        #if random.randint(-1000, 1000) == 123: print("SHOW")
        if random.randint(0, 0) == 0: print(f"WRITE [({string_aleatoria(MAX_STRING_LENGTH)},{string_aleatoria(MAX_STRING_LENGTH)})]")
        #if random.randint(0, 0) == 0: print(f"READ [{string_aleatoria(MAX_STRING_LENGTH)},{string_aleatoria(MAX_STRING_LENGTH)},{string_aleatoria(MAX_STRING_LENGTH)}]")
        #if random.randint(0, 0) == 0: print(f"DELETE [{string_aleatoria(MAX_STRING_LENGTH)},{string_aleatoria(MAX_STRING_LENGTH)}]")
        #print(f"WRITE [({string_aleatoria(MAX_STRING_LENGTH)},{string_aleatoria(MAX_STRING_LENGTH)})]")
        
    for _ in range(20):
        print("BACKUP")
    #print("BACKUP")

main()