const express = require('express');
const cors = require('cors');
const { spawn } = require('child_process');
const fs = require('fs');
const path = require('path');

const app = express();
app.use(cors()); // Allow requests from React dev server (adjust in production)
app.use(express.json()); // Parse JSON request bodies

// --- Configuration ---
// Adjust these paths if you put the C files in a subdirectory
const C_EXECUTABLE_PATH = path.join(__dirname, '../ttt-rl/ttt'); // Path to your compiled C program
const NN_MODEL_PATH = path.join(__dirname, '../ttt-rl/my_model.dat'); // Path to your saved NN model
const TRAINING_LOG_PATH = path.join(__dirname, '../ttt-rl/training_log.json'); // Path to your training log
const C_EXEC_TIMEOUT = 3000; // Timeout for C process in milliseconds (e.g., 3 seconds)
// --- ------------- ---


// Endpoint to get AI move
app.post('/api/move', (req, res) => {
    const { board } = req.body; // Expect board state from React, e.g., "X.O.O.X.."

    if (!board || typeof board !== 'string' || board.length !== 9 || !/^[XO.]{9}$/.test(board)) {
        console.error("Invalid board received:", board);
        return res.status(400).json({ error: 'Invalid board state format. Expected 9 chars of X, O, or .' });
    }

    // --- Basic Game Over Check (Optional but Recommended) ---
    // You could implement check_game_over logic here in JS to avoid calling C if game is done
    // function checkGameOverJS(boardString) { ... return 'X', 'O', 'T', or null; }
    // const winner = checkGameOverJS(board);
    // if (winner) {
    //     console.log(`Game already over on board: ${board}. Winner: ${winner}`);
    //     // Decide what to return. Maybe the final board or an error/status.
    //     // Returning the current board might be simplest for the frontend.
    //     return res.json({ board: board, status: `Game Over: ${winner}` });
    // }
    // --- ---------------------------------------------- ---


    console.log(`Requesting AI move for board: ${board}`);

    const args = [
        '--play-ai', board,
        '--load', NN_MODEL_PATH // Make sure the C code uses this argument
    ];

    let outputData = '';
    let errorData = '';
    let processExited = false;

    const tttProcess = spawn(C_EXECUTABLE_PATH, args);

    // Set a timeout to kill the process if it takes too long
    const timeoutId = setTimeout(() => {
        if (!processExited) {
            console.error(`C process timed out after ${C_EXEC_TIMEOUT}ms for board: ${board}`);
            tttProcess.kill('SIGTERM'); // Send termination signal
            if (!res.headersSent) {
                 res.status(504).json({ error: 'AI move calculation timed out.' });
            }
        }
    }, C_EXEC_TIMEOUT);

    tttProcess.stdout.on('data', (data) => {
        outputData += data.toString();
        console.log(`C Process stdout: ${data}`);
    });

    tttProcess.stderr.on('data', (data) => {
        errorData += data.toString();
        console.error(`C Process stderr: ${data}`);
    });

    tttProcess.on('error', (err) => {
        processExited = true;
        clearTimeout(timeoutId);
        console.error('Failed to start C process:', err);
        if (!res.headersSent) {
            res.status(500).json({ error: 'Failed to start AI engine.' });
        }
    });

    tttProcess.on('close', (code) => {
        processExited = true;
        clearTimeout(timeoutId); // Clear the timeout
        console.log(`C Process exited with code ${code}`);

        if (res.headersSent) {
            return; // Response already sent (e.g., timeout)
        }

        if (code !== 0) {
            console.error(`C process exited with error code ${code}. Stderr: ${errorData}`);
            return res.status(500).json({ error: 'AI engine encountered an error.', details: errorData });
        }

        // Process the outputData - expect a single number (the move index)
        const moveIndexStr = outputData.trim();
        const moveIndex = parseInt(moveIndexStr, 10);

        if (isNaN(moveIndex) || moveIndex < 0 || moveIndex > 8) {
            console.error(`Invalid move index received from C process: '${moveIndexStr}'`);
            return res.status(500).json({ error: 'AI engine returned invalid move.', details: outputData });
        }

        // --- Apply the move to the board ---
        let nextBoard = board.split('');
        if (nextBoard[moveIndex] === '.') {
             // Determine AI symbol ('O' if X moved last or board empty, 'X' otherwise - safer)
             const xCount = board.split('X').length - 1;
             const oCount = board.split('O').length - 1;
             const aiSymbol = (xCount > oCount) ? 'O' : 'X'; // AI plays 'O' if it's O's turn
             // A simpler assumption based on the C code is AI is always 'O'
             // const aiSymbol = 'O';
            nextBoard[moveIndex] = aiSymbol;
            nextBoard = nextBoard.join('');
            console.log(`AI chose move ${moveIndex}. New board: ${nextBoard}`);
            res.json({ board: nextBoard, aiMove: moveIndex }); // Send the *updated* board back
        } else {
            // AI chose an invalid (occupied) square!
            console.error(`AI chose occupied square ${moveIndex} on board ${board}. Output was: ${outputData}`);
            // Handle this error - maybe try another move? Or return error?
            // For now, return an error. This indicates a bug in the C logic or NN.
            res.status(500).json({ error: 'AI chose an invalid square.', details: `Move: ${moveIndex}` });
        }
        // --- ----------------------------- ---
    });
});


// Endpoint for training data
app.get('/api/training-data', (req, res) => {
    fs.readFile(TRAINING_LOG_PATH, 'utf8', (err, data) => {
        if (err) {
            console.error("Error reading training log:", err);
            // Check if file not found specifically
            if (err.code === 'ENOENT') {
                 return res.status(404).json({ error: 'Training data log file not found.' });
            }
            return res.status(500).json({ error: 'Could not read training data file.' });
        }
        try {
            const jsonData = JSON.parse(data); // Assuming JSON format from C code
            res.json(jsonData);
        } catch (parseErr) {
            console.error("Error parsing training log:", parseErr);
            res.status(500).json({ error: 'Could not parse training data (invalid JSON?).' });
        }
    });
});

const PORT = process.env.PORT || 3001; // Use port 3001 for the backend
app.listen(PORT, () => {
    console.log(`Server running on http://localhost:${PORT}`);
    // Check if C executable exists and is executable
    fs.access(C_EXECUTABLE_PATH, fs.constants.X_OK, (err) => {
        if (err) {
            console.error(`Error: C executable not found or not executable at ${C_EXECUTABLE_PATH}`, err);
        } else {
             console.log(`C executable found at ${C_EXECUTABLE_PATH}`);
        }
    });
     fs.access(NN_MODEL_PATH, fs.constants.R_OK, (err) => {
        if (err) {
            console.warn(`Warning: NN model file not found or not readable at ${NN_MODEL_PATH}`, err);
        } else {
             console.log(`NN model file found at ${NN_MODEL_PATH}`);
        }
    });
     fs.access(TRAINING_LOG_PATH, fs.constants.R_OK, (err) => {
        if (err) {
            console.warn(`Warning: Training log file not found or not readable at ${TRAINING_LOG_PATH}`, err);
        } else {
             console.log(`Training log file found at ${TRAINING_LOG_PATH}`);
        }
    });
});
