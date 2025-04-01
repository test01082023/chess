# Terminal Chess Game

## Project Information
- **Title:** Terminal Chess Game
- **Version:** 1.0
- **Author:** You
- **Description:** A complete terminal-based chess game written in Python, including all essential components — pseudocode, Python code, flowchart, and an example game state. The project aims to provide an accessible yet robust implementation of a chess game within a terminal.

---

## Features
1. **Pseudocode**: Logical flow of the game from setup to end.
2. **Python Implementation**: Modular Python code for executing the game logic.
3. **Flowchart**: Visual representation of the game's logic using Mermaid.js.
4. **JSON Example**: Sample serialized board state to illustrate possible game states.

---

## Pseudocode Overview
The pseudocode outlines:
- **Game Setup:** Initializing the chess board and game state variables.
- **Game Loop:** Managing player actions, validating moves, and updating game states.
- **Game End:** Announcing results and restarting or exiting.

---

## Python Code
The Python code provides:
- Object-oriented design with modular methods.
- Features for move validation, board updates, and game state checks.
- Interactive terminal gameplay with options to restart or quit.

---

## Flowchart
The game's logic flow is depicted in Mermaid.js syntax:
```mermaid
graph TD
  A[Start Game] --> B[Initialize Game State]
  B --> C[Main Game Loop]
  C --> D[Display Board]
  D --> E{Player Action}
  E -- Move --> F[Parse & Validate Move]
  F -- Invalid --> C
  F -- Valid --> G[Update Board & History]
  G --> H[Checkmate or Stalemate?]
  H -- Yes --> I[Announce Result]
  H -- No --> J[Switch Player] --> C
  E -- Restart --> B
  E -- Quit --> K[Exit Game]
  I --> L{Play Again?}
  L -- Yes --> B
  L -- No --> K
