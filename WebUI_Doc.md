# Tic-Tac-Toe RL - Web UI Usage Documentation

This section explains how to run the Node.js backend server and the React frontend client to interact with the trained Tic-Tac-Toe C program (`ttt`) through a web browser interface.

## Prerequisites

1.  **C Program Compiled:** Ensure you have successfully compiled `ttt.c` into an executable named `ttt` using the provided compilation command.
2.  **Trained Model:** You need a trained neural network model file (e.g., `ttt_nn.dat` or `my_model.dat`). Generate this by running the C program with the `--train` option (e.g., `./ttt --train 100000 --save my_model.dat`).
3.  **Node.js and npm:** Make sure Node.js (which includes npm) is installed on your system. You can download it from [nodejs.org](https://nodejs.org/).
4.  **Project Structure:** Assume you have a project structure similar to this:
    ```
    ttt-rl-web/
    ├── server/          # Backend code
    │   ├── node_modules/
    │   ├── server.js
    │   ├── package.json
    │   ├── package-lock.json
    │   ├── ttt           # <-- COPY compiled C executable here for easier path
    │   └── my_model.dat  # <-- COPY trained model file here for easier path
    │   └── training_log.json # <-- Optional: Copy log file here or let C create it
    │
    └── client/          # Frontend React code
        ├── node_modules/
        ├── public/
        ├── src/
        ├── package.json
        └── ...
    ```


5.  **Dependencies Installed:**
    *   Navigate to the `server` directory and run `npm install`.
    *   Navigate to the `client` directory and run `npm install`.

## Running the Application

You need to run the backend server first, and then the frontend client in a separate terminal.

**Step 1: Start the Backend Server**

1.  **Open a Terminal:** Launch a new terminal window or tab.
2.  **Navigate:** Change directory to the `server` folder:
    ```bash
    cd path/to/your/project/server
    ```
3.  **Verify Configuration (Important):**
    *   Open the `server.js` file in a text editor.
    *   Check the configuration paths near the top:
        ```javascript
        const C_EXECUTABLE_PATH = path.join(__dirname, 'ttt');
        const NN_MODEL_PATH = path.join(__dirname, 'my_model.dat'); // <-- Make sure this matches your model filename!
        const TRAINING_LOG_PATH = path.join(__dirname, 'training_log.json');
        ```
    *   Ensure these paths correctly point to your compiled `ttt` executable and your trained `.dat` model file *relative to the `server.js` file*. The example assumes they are directly inside the `server` directory. Adjust if needed.
4.  **Run the Server:**
    ```bash
    node server.js
    ```
5.  **Expected Output:** You should see messages indicating the server is running and checking for the necessary files:
    ```
    Server running on http://localhost:3001
    C executable found at /path/to/your/project/server/ttt
    NN model file found at /path/to/your/project/server/my_model.dat
    Training log file found at /path/to/your/project/server/training_log.json
    ```
    *(Note: Warnings might appear if the model or log file isn't found initially, but the server should still run).*
6.  **Keep it Running:** Leave this terminal window open. The backend server needs to stay running to handle requests from the frontend.

**Step 2: Start the React Frontend Client**

1.  **Open a *Second* Terminal:** Launch another new terminal window or tab. **Do not** close the first one running the server.
2.  **Navigate:** Change directory to the `client` folder:
    ```bash
    cd path/to/your/project/client
    ```
3.  **Verify API URL (If Necessary):**
    *   The React code (e.g., in `src/pages/GamePage.js` and `src/pages/TrainingPage.js`) uses a variable like `const API_URL = 'http://localhost:3001';`.
    *   This URL *must* match the address where your backend server is running. The default `http://localhost:3001` should be correct if you followed Step 1.
4.  **Run the Client:**
    ```bash
    npm start
    ```
5.  **Expected Output:** React will compile the application. This might take a few moments. Once ready, it will likely:
    *   Print messages like `Compiled successfully!`
    *   Automatically open a new tab in your default web browser pointing to `http://localhost:3000` (or another port if 3000 is busy).
    *   Show the Tic-Tac-Toe web interface.
6.  **Keep it Running:** Leave this second terminal window open as well. The React development server needs to stay running to serve the frontend application.

## Using the Web Interface

1.  **Access:** Open your web browser and navigate to `http://localhost:3000` (or the address provided by `npm start`).
2.  **Play Game:**
    *   The default page should be the game board.
    *   Click on an empty square to make your move (you are 'X').
    *   The status message will indicate "AI is thinking...".
    *   The backend server (in the first terminal) will execute the C program (`ttt --play-ai ...`). You can observe the logs there.
    *   The AI's move ('O') will appear on the board.
    *   Continue playing until the game ends (Win, Loss, or Tie).
    *   Use the "Reset Game" button to start over.
3.  **View Training Stats:**
    *   Click the "Training Stats" link in the navigation header.
    *   A chart will be displayed showing the AI's performance (Win/Loss/Tie percentages) over the training intervals, based on the data in `training_log.json`. If the file doesn't exist or is empty, an appropriate message will be shown.

## How it Works (Interaction Flow)

1.  **User Action (React):** You click a square on the board in the browser.
2.  **API Request (React -> Node):** The React frontend sends the current board state (e.g., `"X......."`) via an HTTP POST request to the backend's `/api/move` endpoint (`http://localhost:3001/api/move`).
3.  **Execute C Program (Node):** The Node.js server receives the request. It uses Node's `child_process.spawn` to execute the compiled C program (`./ttt`) with the appropriate arguments (`--play-ai <board_string> --load <model_file>`).
4.  **C Calculation:** The C program loads the neural network, calculates the best move for the given board state, and prints *only* the move index (e.g., `4`) to its standard output (`stdout`). Other messages go to `stderr`.
5.  **Capture Output (Node):** The Node.js server captures the `stdout` from the C process.
6.  **Process & Respond (Node -> React):** The server parses the move index, updates the board string (e.g., `"X......."` becomes `"X...O...."`), and sends a JSON response containing the *new* board state and the AI's move index back to the React frontend.
7.  **Update UI (React):** The React frontend receives the JSON response, updates its internal state (the board), and re-renders the UI to show the AI's move.

*(A similar, simpler flow occurs for the `/api/training-data` endpoint, where the Node server just reads the JSON file and sends its content to React).*

## Stopping the Application

1.  Go to the terminal running the React client (`npm start`) and press `Ctrl + C`. Confirm if prompted.
2.  Go to the terminal running the Node.js server (`node server.js`) and press `Ctrl + C`.