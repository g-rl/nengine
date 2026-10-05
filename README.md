# neura engine patches

the DLL source code that is used with the [neura GSC mod](https://github.com/g-rl/neura). 

this should support most of IW8, S4, IW9. if you are looking for JUP/MWIII, check out [cengine](https://github.com/g-rl/cengine) instead

- **you must own a license to the game(s) in order to use neura.**
- neura's engine and GSC will *always* be **free** and **Open Source** for anyone
- neura is only usable offline and is not to be used as "cheats" or "hacks"
- nengine contains **0 AI code** inside of it and has been written and tested entirely by hand

## how to compile
1. run `generate.bat`
2. go to `build/` folder, open the .SLN file for Visual Studio 2026
3. build Debug x64
4. make sure it is named XInput9_1_0.dll, drag into game, and you're done

## features
- native .GPC scripting (Cronus format)
- custom bot names via `neura/bots.txt` in main folder
- ^: rainbow color
- custom weapon mechanics (sprint swaps, instashoots, always canswaps, freeze animation, canzoom, altswaps)

## confirmed working versions
- IW8 1.20.4 & 1-20.4-replay
- IW8 1.38
- IW8 1.41
- S4 1.14
- S4 1.26
- IW9 1.25
- IW9 latest steam

### credits
- [mjkzy](https://github.com/mjkzy) - initial research + compatibility for every single game version [when possible]
- blue - cronus & player mechanics research
- [hinatyu](https://x.com/hinatyu) - extremely useful resources for reverse engineering newer games
