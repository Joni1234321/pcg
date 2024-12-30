# Installation
1. Install libfreetype with MINGW64 and add its path to environment.  
On my Windows machine it's `C:\msys64\mingw64\bin`
2. Run CMAKE it copies the DLL's into the build directory 

## Problem
freetyped.dll not found. idk, found this post that says we need to not use relative path. idk why but i set the aboslute path once and then turned it back to relative and it worked


```aiignore
    char* font_path_absolute = "C:\\Active\\CPP\\pcg\\resources\\font.ttf"; // working
    char* font_path = "../resources/font.ttf"; // fail
```