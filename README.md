# D3D11-Worldtoscreen-Finder 
for UT4 Engine games and for few other games. Some other games may need different worldtoscreen function to work.

Tested and working in the following games so far:

Game			  WorldViewCBnum		ProjCBnum		matProjnum

UT4 Alpha		2					        1				    16		

Fortnite		2					        1				    16

Outlast 		0					        1				    0 and 16

Warframe		0					        0				    0 or 4

GTA 5       5 (and other too)	1				    44

![alt tag](https://github.com/DrNseven/D3D11-Worldtoscreen-Finder/blob/master/w2sloggergithub.jpg)

What is worldtoscreen? W2S will give you screen coordinates, it can be used for esp and aimbot.

How to use:
1. Find models in your game, use the logger for that (press CTRL+ALT+L to toggle & use keys -O & P+). 

2. Press I to log highlighted models to log.txt.

3. Open log.txt, edit universal.cpp and add your model rec. Example: if (Stride == 24 && Descr.Format == 71 && pscdesc.ByteWidth == 4096 && indesc.ByteWidth > ?)

4. Use the same model rec for worldtoscreen. (and/or filter out heads for better performance)

5. Bruteforce pWorldViewCBnum, pProjCBnum & matProjnum, press CTRL+ALT+L and use the displayed keys. If you have found the right values the word "Enemy" will be displayed on models position. 

6. Edit main.h and Replace pWorldViewCBnum, pProjCBnum & matProjnum with the working values.

7. Compile with visual studio (creates .dll)

8. Inject .dll into d3d11 game

The ingame menu key is INSERT, use arrows to navigate. (uncomment options to make toggling work, remove the "//"
Example: //if (sOptions[2].Function == 1) 


Credits: Wu ZhiQiang, dracorx, evolution536

