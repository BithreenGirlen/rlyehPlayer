# rlyehPlayer

某寢室用

## How to play

First, prepare the files, commented below, with proper directory.
<pre>
assets
└ advscene
   ├ resources
   │  └ advscene
   │     ├ ...
   │     ├ sound
   │     │  ├ ...
   │     │  └ voice
   │     │    ├ character
   │     │    │  ├ ch_10001
   │     │    │  │  ├ ...
   │     │    │  │  └ r18 // Scene voice folder
   │     │    │  │    ├ vo_R18_10001_001.m4a // Scene voice file
   │     │    │  │    └ ...
   │     │    │  └ ...
   │     │    └ ...
   │     └ texture
   │        ├ ...
   │        ├ character
   │        │  ├ ...
   │        │  └ r18_scenes // Scene Spine folder
   │        │    ├ r18_10001_0
   │        │    │  ├ rev10001_01.atlas
   │        │    │  ├ rev10001_01.png
   │        │    │  ├ rev10001_01.skel
   │        │    │  ├ rev10001_01_2.png
   │        │    │  └ ...
   │        │    ├ r18_10002_0
   │        │    │  └ ...
   │        │    └ ...
   │        └ ...
   ├ ...
   └ scenarioexcel
      ├ character // Scene script folder
      │  ├ 10001
      │  │  ├ ...
      │  │  └ 10001A5_R18.book.json // A selectable kind of script file.
      │  ├ 10002
      │  │  └ ...
      │  └ ...
      └ ...
</pre>

Then, select `advscene/scenarioexcel/character/XXXXX/*_R18.book*` file from application menu item `File->Open`.  

## Mouse functions

Mouse functions are valid only when a script is loaded.

| Input | Function |
| --- | --- |
| Wheel scroll | Scale up/down. Combining `Ctrl` to zoom in/out. |
| Left-pressed + wheel scroll | Speed up/down the animation. |
| Left-drag | Move view-point. |
| Middle-click | Reset scale, speed, and view-point to default. |
| Right-click | Show context menu to jump to the scene where the animation is switched. |
| Right-pressed + wheel scroll | Fast-forward/rewind the text. |
| Right-pressed + middle-click | Hide/show the frame of window. |
| Right-pressed + left-drag | Move borderless window. |

- When menu item `Option->SyncImage` is turned off, left-click shifts the animation.

## Keyboard functions

| Input | Function |
| --- | --- |
| <kbd>C</kbd> | Toggle text colour between black and white. |
| <kbd>H</kbd> | Show/hide the help text. |
| <kbd>T</kbd> | Hide/show the scene text. |
| <kbd>Esc</kbd> | Close the application. |
| <kbd>∧</kbd> | Open the previous script. |
| <kbd>∨</kbd> | Open the next script. |
| <kbd>＞</kbd> | Fast-forward the text. |
| <kbd>＜</kbd> | Rewind the text. |

## Preferences

The extensions of resource files can be configured through `setting.txt` in the same directory of the executable.

## External libraries

- [spine-cpp-4.2](https://github.com/EsotericSoftware/spine-runtimes/tree/4.2)
- [JSON for Modern C++ v3.12.0](https://github.com/nlohmann/json/releases/tag/v3.12.0)

## Build

Visual Studio is required.
1. Open `rlyerPlayer` directory with Visual Studio.
    - One of the way to achieve this is as follows:  
      (1) Open `rlyerPlayer` directory with file explorer.  
      (2) Type `cmd` in the address bar of file explorer to start up command prompt.  
      (3) Type `start devenv .` in command prompt.
3. Wait for downloading external libraries to be done.
4. Build All.
