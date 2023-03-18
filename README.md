# screen-dimmer
Only works on Windows. Windows has a screen dimming mechanism built in, but it only works for my built-in display for some stupid reason. This dims all the displays.

# How To Build
Visual Studio solution. Open in VS and build. Recommendation: Release|x86

# How To Install
Drop final binary into an autostart folder somewhere on your computer.

# How To Use
Use Ctrl+Alt+Q to dim screen, use Ctrl+Alt+W to make screen brighter. The brightness is represented by an 8-bit value and it wraps, meaning that holding W
for too long will wrap back around and make your screen super dark again.

# What To Do If It Doesn't Work
When the program encounters an issue, it'll just kill itself. If you want to know more about why it died, open up Visual Studio and run the project in Debug mode.
Information will be outputted to the debug console, which you can use to debug the program.
