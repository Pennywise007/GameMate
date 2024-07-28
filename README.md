<p align="center">
  <img src="./media/logo.png" alt="GameMate logo">
</p>
<h1 align="center">GameMate</h1>

GameMate is a powerful Windows application designed to enhance your productivity and gaming experience. It allows you to customize key bindings for different applications, add a crosshair overlay for games, block accidental key presses, track your time efficiently, and automate complex sequences of actions.

## Features

Advanced Action Automation: Create complex automation sequences that go beyond simple clicks. Include mouse movements, key presses, and script executions to automate repetitive tasks in any application.

Per-Application features:

- Key Bindings: Assign unique key bindings for each application. record multiple keys or mouse actions for each bind. For example, you can set up a series of actions for Notepad using the key '8', and configure different actions for Excel using the same key, without any interference.

- Crosshair Overlay: Display a crosshair in the center of the screen when a specified application (like a game) is active. Ensure you always have a precise aim.

- Accidental Key Press Blocker: Specify a list of keys to be blocked to prevent accidental presses in each application. For instance, disable the Windows key while playing a game to avoid disruptions.

- Key remapping. Some apps doesn't allow you to override default key bindings. For example in some games you will crouch on `C` key, while in the others you will use `Ctrl`. You can easily unify it now, check example on remapping  `WASD` keys to `YGHJ`:


- Brightness control: allows to set custom brightness to each application. An external monitor must have DDC/CI enabled.
![GameMate](./media/ddcci.jpg)

Timer: Start a timer to monitor how long some action takes.


## Build

1. Run `prerequisites.bat` to load submodules and build dependencies.
2. Open `GameMate.sln` with Visual Studio and build.
