import React, { useState, useEffect } from 'react';
import axios from 'axios';
import Board from '../components/Board';
import './GamePage.css'; // Create this file

const API_URL = 'http://localhost:3001'; // Your backend URL

// Helper function to check for winner
function calculateWinner(squares) {
  const lines = [
    [0, 1, 2], [3, 4, 5], [6, 7, 8], // rows
    [0, 3, 6], [1, 4, 7], [2, 5, 8], // columns
    [0, 4, 8], [2, 4, 6]             // diagonals
  ];
  for (let i = 0; i < lines.length; i++) {
    const [a, b, c] = lines[i];
    if (squares[a] && squares[a] === squares[b] && squares[a] === squares[c]) {
      return squares[a]; // 'X' or 'O'
    }
  }
  // Check for tie (no null squares left)
  if (squares.every(square => square !== null)) {
      return 'T'; // Tie
  }
  return null; // Game not over
}


function GamePage() {
    const initialBoard = Array(9).fill(null);
    const [board, setBoard] = useState(initialBoard);
    const [isPlayerTurn, setIsPlayerTurn] = useState(true); // Player is 'X'
    const [status, setStatus] = useState('Your turn (X)');
    const [loadingAI, setLoadingAI] = useState(false);
    const [gameOver, setGameOver] = useState(false);
    const [error, setError] = useState(null);

    // Reset game handler
    const handleReset = () => {
        setBoard(initialBoard);
        setIsPlayerTurn(true);
        setStatus('Your turn (X)');
        setGameOver(false);
        setLoadingAI(false);
        setError(null);
    };

    // Effect to handle AI turn when it's not the player's turn
    useEffect(() => {
        if (!isPlayerTurn && !gameOver && !loadingAI) {
            setLoadingAI(true);
            setError(null); // Clear previous errors
            setStatus('AI is thinking...');

            // Convert board array to string format expected by backend/C code
            // Use '.' for null squares
            const boardString = board.map(cell => cell === null ? '.' : cell).join('');

            axios.post(`${API_URL}/api/move`, { board: boardString })
                .then(response => {
                    const aiBoardString = response.data.board;
                    if (aiBoardString && aiBoardString.length === 9) {
                        // Convert response board string back to array
                        const updatedBoard = aiBoardString.split('').map(cell => cell === '.' ? null : cell);
                        setBoard(updatedBoard);

                        // Check for winner after AI move
                        const winner = calculateWinner(updatedBoard);
                        if (winner) {
                            setGameOver(true);
                            setStatus(winner === 'T' ? "It's a Tie!" : `Winner: ${winner}!`);
                        } else {
                            setIsPlayerTurn(true); // Switch back to player's turn
                            setStatus('Your turn (X)');
                        }
                    } else {
                         throw new Error("Invalid board data received from server.");
                    }
                })
                .catch(err => {
                    console.error("Error getting AI move:", err);
                    let errorMsg = 'Failed to get AI move.';
                    if (err.response && err.response.data && err.response.data.error) {
                        errorMsg = `AI Error: ${err.response.data.error}`;
                    } else if (err.message) {
                        errorMsg = err.message;
                    }
                    setError(errorMsg);
                    setStatus('Error occurred. Try resetting.');
                    // Don't switch turn back if error occurred
                })
                .finally(() => {
                    setLoadingAI(false);
                });
        }
    // eslint-disable-next-line react-hooks/exhaustive-deps
    }, [isPlayerTurn, gameOver, board]); // Dependency array


    // Handle player click
    const handleClick = (index) => {
        // Ignore click if not player's turn, square is filled, or game over
        if (!isPlayerTurn || board[index] || gameOver || loadingAI) {
            return;
        }

        const newBoard = [...board];
        newBoard[index] = 'X'; // Player is always 'X'
        setBoard(newBoard);

        // Check for winner after player move
        const winner = calculateWinner(newBoard);
        if (winner) {
            setGameOver(true);
            setStatus(winner === 'T' ? "It's a Tie!" : `Winner: ${winner}!`);
        } else {
            setIsPlayerTurn(false); // Switch to AI's turn
            // The useEffect hook will now trigger the AI move
        }
    };


    return (
        <div className="game-page">
            <h2>Play against the AI</h2>
            <div className="status-area">
                <p className={`status ${gameOver ? 'game-over' : ''} ${error ? 'error' : ''}`}>
                    {status}
                </p>
                {error && <p className="error-message">Details: {error}</p>}
            </div>

            <div className="game-board">
                 <Board squares={board} onClick={handleClick} />
            </div>

            <div className="controls">
                {loadingAI && <div className="spinner"></div>}
                <button onClick={handleReset} disabled={loadingAI}>
                    Reset Game
                </button>
            </div>
        </div>
    );
}

export default GamePage;