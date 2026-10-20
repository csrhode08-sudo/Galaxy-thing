Super Mario Galaxy - PC (test build)
====================================

How to run
----------
1. Unzip everything (SuperMarioGalaxy.exe and all the .dll files) into the folder that has your
   Korean disc image, "Super Mario Wii - Galaxy Adventure (Korea).wbfs".
   (Any RMGK01 .wbfs/.iso/.rvz works; you can also drag the disc image onto the .exe.)
2. Double-click SuperMarioGalaxy.exe.

A console window opens next to the game and shows a log; if the game closes or crashes, please
copy the last lines of that window (or take a screenshot) - that's what I need to fix it.

Getting into the game
---------------------
- Title screen: press A and B together (Space + left mouse button, or A + right trigger on a pad).
- File select: point with the mouse and click the file bubble, then "OK", pick an icon, and
  "select this data". The story intro plays; press A to page through it.

Default controls (change them any time with F1)
-----------------------------------------------
                    Keyboard / mouse            Xbox-style controller
  Move (Nunchuk)    W A S D                     Left stick
  A (jump)          Space                       A
  B                 Left mouse button           Right trigger
  Spin (shake)      Right mouse button or E     X
  Z (crouch)        Left Shift                  Left trigger
  C (camera)        Q                           Left bumper
  D-pad             Arrow keys                  D-pad
  + / -             Enter / Tab                 Start / Back
  1 / 2             1 / 2                       Y / B
  Pointer           Mouse                       Right stick (R3 re-centres)

F1 opens the settings menu: rebind keys, mouse and controller buttons, and (Graphics tab) choose the
resolution, anti-aliasing, texture filtering, VSync and fullscreen. Everything is saved automatically.
F11 toggles fullscreen.

HD texture packs
----------------
In F1 > Graphics, turn on "Save the game's textures as you play" and restart. Every image the game draws is
saved (once) to %APPDATA%\GalaxyPC\SuperMarioGalaxy\texture_dumps\ as you play. To replace one, edit it at any
size, keep the file name (save as .png or .dds) and put it in %APPDATA%\GalaxyPC\SuperMarioGalaxy\textures\
(subfolders are fine). Replacements load when the game starts. The names follow Dolphin's texture-pack
convention. The Graphics page has buttons that open both folders.

Saves and settings live in %APPDATA%\GalaxyPC\SuperMarioGalaxy\.

The game runs at the Wii's 60 frames per second whatever your monitor's refresh rate.

Known problems in this build
----------------------------
- A few objects can still draw as black shapes.
- Some sounds may be missing.
- Only the opening of the game (title, file select, the storybook, the Star Festival and Bowser's attack) has
  been tested so far; later galaxies may still crash. If one does, send the console window.
- Text is Korean (it comes from the Korean disc).
