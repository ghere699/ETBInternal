# A Simple cheat for Escape The Backrooms

This project is an internal cheat for the game *Escape The Backrooms*, created for educational purposes to explore game memory, function hooking, and GUI implementation with Dear ImGui in dx12.


### ⚠️ Disclaimer

This software is intended for educational purposes only. Using it in online multiplayer games may result in a ban. The creator is not responsible for any consequences of its use. **Use at your own risk.**

---

### Features

*   **Player Cheats:**s
    *   Infinite Stamina
    *   Infinite Sanity
*   **Item Spawner:**
    *   Spawn any item.
*   **Host-Only:**
    *   Teleport All Players to yourself
    *   Kill All Players
    *   Speedhack with adjustable speed
    *   Fly Mode with adjustable speed
*   **GUI:**
    *   Clean and simple menu made with ImGui.

---

### How to Compile

1.  Clone this repository: `git clone https://github.com/ghere699/ETBInternal.git`
2.  Open `ETBCheat.sln` with Visual Studio.
3.  Set the build configuration to **Release** and **x64**.
4.  Build the solution.
5.  The compiled DLL will be in the `x64/Release/` folder.

---

### How to Use

1.  Compile the DLL as described above.
2.  Use a DLL injector of your choice to inject `ETBCheat.dll` into the game process (`Backrooms-Win64-Shipping.exe`).
3.  Press **`INSERT`** to open and close the menu.

---

### License

This project is released under the [MIT License](LICENSE).