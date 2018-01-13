# D3D11-Worldtoscreen-Finder 
for some games. Other games may need different worldtoscreen function to work.

![alt tag](https://github.com/DrNseven/D3D11-Worldtoscreen-Finder/blob/master/w2s.jpg)

![alt tag](https://github.com/DrNseven/D3D11-Worldtoscreen-Finder/blob/master/w2sloggergithub.jpg)

What is worldtoscreen? W2S will give you screen coordinates, it can be used for esp and aimbot.

How to use:
1. Find models in your game, use the logger for that (press CTRL+ALT+L to toggle & use keys -O & P+). 

2. Press I to log highlighted models to log.txt.

3. Open log.txt, edit universal.cpp and add your model rec. Example: if (Stride == 24 && pscdesc.ByteWidth == 4096 && indesc.ByteWidth > ?)

4. Use same model rec for worldtoscreen. (and/or filter out heads for better performance)

5. Bruteforce pWorldViewCBnum, pProjCBnum & matProjnum, press CTRL+ALT+L and use the displayed keys. Text will be displayed on models position. If positions are wrong try other worldtoscreen function (main.h: w2s 1 - 3)

6. Edit main.h and Replace pWorldViewCBnum, pProjCBnum & matProjnum with the right values.

7. Compile with visual studio (creates .dll) and inject .dll into your d3d11 game

The ingame menu key is INSERT, use arrows to navigate. 



Credits: Wu ZhiQiang, dracorx, evolution536, Yazzn

