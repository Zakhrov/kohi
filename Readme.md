# Kohi game engine 


This is a from-scratch implementation **NOT A FORK** of Travis Vroman's [Kohi game engine](https://github.com/travisvroman/kohi) with my own features and enhancements. 

------------------------------------------------
## Planned features and enhancements

### Explicit Multi-GPU alternate frame rendering
The main addition to the upstream Kohi engine is explicit multi GPU and alternate frame rendering support. Yes it may be an inefficient stuttery mess but it is a for science pet project of mine and not meant to go into production.
That last part I might revisit if I can get it to work effectively

----------------------------------
### Full software renderer
Something I have always been curious about and wanted to implement. May or may not make it in though

-------------------------------


## Main differences from the upstream Kohi engine

-------------------------------------------------
### Platform target
The platform target is Linux currently with no immediate plans to add Windows, Mac or Console support. Windows support might come down the line later once I learn how to effectively use cross-compiler toolchains

-----------------------------------------
### Build system
I have used CMake instead of Make or the custom build scripts that Travis used in his tutorial series mostly because I'm more comfortable with it.

------------------------------

### Language and naming conventions
I have used a mix of C and C++ instead of the pure C implementation that upstream Kohi uses. The main reason is because I have heavily used C++ vectors in the Vulkan backend to implement AFR
I *could* have used the darray implementation but I felt more comfortable using the C++ vector. Also keeping the Vulkan backend in C++ allows me to use GLM for the math library instead of the custom math library that Kohi ships with. I'll implement it by following the tutorials of course but I'm not sure if I'll use it. 
