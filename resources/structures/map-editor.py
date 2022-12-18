import os
from getch import getch

filename = input("File: ")
file = open(filename, 'w')

width = int(input("width: "))
height = int(input("height: "))

map = [[ 0 for x in range(0,height)] for y in range(0,width)]

tiles = [
    ("  ", "Void"),
    ("░░", "Oxygen"),
    ("░░", "Vacumn"),
    ("██", "Stone"),
    ("▓▓", "Silt"),
    ("Cu", "Copper"),
    ("Ti", "Titanium"),
    ("##", "Insulated Wall"),
    ("[]", "Reinforced Window"),
    ("==", "Door"),
    ("[=", "Door panel"),
    ("=]", "Door panel")
]

ENDC = '\033[0m'
BOLD = '\033[1m'
UNDERLINE = '\033[4m'

selectx = int(width/2)
selecty = int(height/2)

def render():
    b = 0
    if(width > len("-| " + filename + " |-")/2):
        b = int(width - (len("-| " + filename + " |-")/2))
    os.system("clear")
    print('-'*b + "-| " + filename + " |-" + '-'*b)
    
    for y in range(0, height):
        print("|",end='')
        for x in range(0, width):
            if(x == selectx and y == selecty):
                print(UNDERLINE + tiles[map[x][y]][0], end=ENDC)
            else:
                print(tiles[map[x][y]][0], end='')
        print("|",end='')
        print("")
    print("-"*(width*2 - 1))
    print(tiles[map[selectx][selecty]][0] + "  -  " + tiles[map[selectx][selecty]][1])

done = False
while(not done):
    render()
    c = getch()
    if(c == 'w'):
        selecty -= 1
    if(c == 's'):
        selecty += 1
    if(c == 'a'):
        selectx -= 1
    if(c == 'd'):
        selectx += 1
    
    if(c == 'z'):
        map[selectx][selecty] -= 1
        if(map[selectx][selecty] < 0):
            map[selectx][selecty] = len(tiles)-1
    if(c == 'x'):
        map[selectx][selecty] += 1
        if(map[selectx][selecty] > len(tiles)-1):
            map[selectx][selecty] = 0
    
    if(c == 'q'):
        done = True
        txt = ""
        for y in range(0, height):
            for x in range(0, width):
                txt += chr(map[x][y] + 48)
            if(y!= height-1):
                txt += '\n'
        file.write(txt)
        file.close()

            