# D3D11-Worldtoscreen-Finder 
for UT4 Engine games and for few other games. Some other games may need different worldtoscreen function to work.

![alt tag](https://github.com/DrNseven/D3D11-Worldtoscreen-Finder/blob/master/w2sloggergithub.jpg)

What is worldtoscreen?
W2S will give you screen coordinates, it can be used for esp and aimbot.

How to use:
1. Find models in your game, use the logger for that (press CTRL+ALT+L to toggle & use keys -O & P+). Edit universal.cpp and add your model rec.

Model recognition example:
if (Stride == xx && ..)
{
	..
}

2. Use the same model rec for worldtoscreen, example:

if (Stride == xx && ..)
{
	AddModel(pContext, 1);
}

3. Bruteforce pWorldViewCBCBnum, pProjCBnum & matProjnum, press CTRL+ALT+L and use the displayed keys. If you have found the right values the word "Enemy" will be displayed on models position.

4. Compile with visual studio (creates .dll)

5. Inject .dll into d3d11 game



Credits: Wu ZhiQiang, dracorx, evolution536

