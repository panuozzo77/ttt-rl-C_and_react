#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <float.h>
#include <string.h>
#include <math.h>

// Neural network parameters .
#define NN_INPUT_SIZE 18
#define NN_HIDDEN_SIZE 100
#define NN_OUTPUT_SIZE 9
#define LEARNING_RATE 0.1

// Game board representation.
typedef struct {
    char board[9];          // Can be "." (empty) or "X", "O".
    int current_player;     // 0 for player (X), 1 for computer (O).
} GameState;

/* Neural network structure. For simplicity we have just
 * one hidden layer and fixed sizes (see defines above).
 * However for this problem going deeper than one hidden layer
 * is useless. */
typedef struct {
    // Weights and biases.
    float weights_ih[NN_INPUT_SIZE * NN_HIDDEN_SIZE];
    float weights_ho[NN_HIDDEN_SIZE * NN_OUTPUT_SIZE];
    float biases_h[NN_HIDDEN_SIZE];
    float biases_o[NN_OUTPUT_SIZE];

    // Activations are part of the structure itself for simplicity.
    float inputs[NN_INPUT_SIZE];
    float hidden[NN_HIDDEN_SIZE];
    float raw_logits[NN_OUTPUT_SIZE]; // Outputs before softmax().
    float outputs[NN_OUTPUT_SIZE];    // Outputs after softmax().
} NeuralNetwork;

/* ReLU activation function */
float relu(float x) {
    return x > 0 ? x : 0;
}

/* Derivative of ReLU activation function */
float relu_derivative(float x) {
    return x > 0 ? 1.0f : 0.0f;
}

// Function to save the neural network weights and biases to a file
int save_nn(NeuralNetwork *nn, const char *filename) {
    FILE *fp = fopen(filename, "wb"); // Open in binary write mode
    if (!fp) {
        perror("Error opening file for saving NN");
        return 0; // Indicate failure
    }

    // Write weights_ih
    if (fwrite(nn->weights_ih, sizeof(float), NN_INPUT_SIZE * NN_HIDDEN_SIZE, fp) != NN_INPUT_SIZE * NN_HIDDEN_SIZE) goto write_error;
    // Write weights_ho
    if (fwrite(nn->weights_ho, sizeof(float), NN_HIDDEN_SIZE * NN_OUTPUT_SIZE, fp) != NN_HIDDEN_SIZE * NN_OUTPUT_SIZE) goto write_error;
    // Write biases_h
    if (fwrite(nn->biases_h, sizeof(float), NN_HIDDEN_SIZE, fp) != NN_HIDDEN_SIZE) goto write_error;
    // Write biases_o
    if (fwrite(nn->biases_o, sizeof(float), NN_OUTPUT_SIZE, fp) != NN_OUTPUT_SIZE) goto write_error;

    fclose(fp);
    printf("Neural network saved to %s\n", filename);
    return 1; // Indicate success

write_error:
    perror("Error writing NN data to file");
    fclose(fp);
    return 0; // Indicate failure
}

// Function to load the neural network weights and biases from a file
int load_nn(NeuralNetwork *nn, const char *filename) {
    FILE *fp = fopen(filename, "rb"); // Open in binary read mode
    if (!fp) {
        perror("Error opening file for loading NN");
        printf("Could not load %s. Starting with a new network.\n", filename);
        return 0; // Indicate failure (or file not found)
    }

    // Read weights_ih
    if (fread(nn->weights_ih, sizeof(float), NN_INPUT_SIZE * NN_HIDDEN_SIZE, fp) != NN_INPUT_SIZE * NN_HIDDEN_SIZE) goto read_error;
    // Read weights_ho
    if (fread(nn->weights_ho, sizeof(float), NN_HIDDEN_SIZE * NN_OUTPUT_SIZE, fp) != NN_HIDDEN_SIZE * NN_OUTPUT_SIZE) goto read_error;
    // Read biases_h
    if (fread(nn->biases_h, sizeof(float), NN_HIDDEN_SIZE, fp) != NN_HIDDEN_SIZE) goto read_error;
    // Read biases_o
    if (fread(nn->biases_o, sizeof(float), NN_OUTPUT_SIZE, fp) != NN_OUTPUT_SIZE) goto read_error;


    fclose(fp);
    //printf("Neural network loaded from %s\n", filename);
    //fprintf(stderr, "Neural network loaded from %s\n", filename); // New line: Print to stderr
    return 1; // Indicate success

read_error:
    //perror("Error reading NN data from file");
    fprintf(stderr, "Error reading NN data from file: %s\n", filename);
    fclose(fp);
    // Optional: Initialize a new network if loading failed mid-way?
    // init_neural_network(nn); // Or handle error differently
    return 0; // Indicate failure
}

/* Initialize a neural network with random weights, we should
 * use something like He weights since we use RELU, but we don't
 * care as this is a trivial example. */
#define RANDOM_WEIGHT() (((float)rand() / RAND_MAX) - 0.5f)
void init_neural_network(NeuralNetwork *nn) {
    // Initialize weights with random values between -0.5 and 0.5
    for (int i = 0; i < NN_INPUT_SIZE * NN_HIDDEN_SIZE; i++)
        nn->weights_ih[i] = RANDOM_WEIGHT();

    for (int i = 0; i < NN_HIDDEN_SIZE * NN_OUTPUT_SIZE; i++)
        nn->weights_ho[i] = RANDOM_WEIGHT();

    for (int i = 0; i < NN_HIDDEN_SIZE; i++)
        nn->biases_h[i] = RANDOM_WEIGHT();

    for (int i = 0; i < NN_OUTPUT_SIZE; i++)
        nn->biases_o[i] = RANDOM_WEIGHT();
}

/* Apply softmax activation function to an array input, and
 * set the result into output. */
void softmax(float *input, float *output, int size) {
    /* Find maximum value then subtact it to avoid
     * numerical stability issues with exp(). */
    float max_val = input[0];
    for (int i = 1; i < size; i++) {
        if (input[i] > max_val) {
            max_val = input[i];
        }
    }

    // Calculate exp(x_i - max) for each element and sum.
    float sum = 0.0f;
    for (int i = 0; i < size; i++) {
        output[i] = expf(input[i] - max_val);
        sum += output[i];
    }

    // Normalize to get probabilities.
    if (sum > 0) {
        for (int i = 0; i < size; i++) {
            output[i] /= sum;
        }
    } else {
        /* Fallback in case of numerical issues, just provide
         * a uniform distribution. */
        for (int i = 0; i < size; i++) {
            output[i] = 1.0f / size;
        }
    }
}

/* Neural network foward pass (inference). We store the activations
 * so we can also do backpropagation later. */
void forward_pass(NeuralNetwork *nn, float *inputs) {
    // Copy inputs.
    memcpy(nn->inputs, inputs, NN_INPUT_SIZE * sizeof(float));

    // Input to hidden layer.
    for (int i = 0; i < NN_HIDDEN_SIZE; i++) {
        float sum = nn->biases_h[i];
        for (int j = 0; j < NN_INPUT_SIZE; j++) {
            sum += inputs[j] * nn->weights_ih[j * NN_HIDDEN_SIZE + i];
        }
        nn->hidden[i] = relu(sum);
    }

    // Hidden to output (raw logits).
    for (int i = 0; i < NN_OUTPUT_SIZE; i++) {
        nn->raw_logits[i] = nn->biases_o[i];
        for (int j = 0; j < NN_HIDDEN_SIZE; j++) {
            nn->raw_logits[i] += nn->hidden[j] * nn->weights_ho[j * NN_OUTPUT_SIZE + i];
        }
    }

    // Apply softmax to get the final probabilities.
    softmax(nn->raw_logits, nn->outputs, NN_OUTPUT_SIZE);
}

/* Initialize game state with an empty board. */
void init_game(GameState *state) {
    memset(state->board,'.',9);
    state->current_player = 0;  // Player (X) goes first
}

/* Show board on screen in ASCII "art"... */
void display_board(GameState *state) {
    for (int row = 0; row < 3; row++) {
        // Display the board symbols.
        printf("%c%c%c ", state->board[row*3], state->board[row*3+1],
                          state->board[row*3+2]);

        // Display the position numbers for this row, for the poor human.
        printf("%d%d%d\n", row*3, row*3+1, row*3+2);
    }
    printf("\n");
}

/* Convert board state to neural network inputs. Note that we use
 * a peculiar encoding I descrived here:
 * https://www.youtube.com/watch?v=EXbgUXt8fFU
 *
 * Instead of one-hot encoding, we can represent N different categories
 * as different bit patterns. In this specific case it's trivial:
 *
 * 00 = empty
 * 10 = X
 * 01 = O
 *
 * Two inputs per symbol instead of 3 in this case, but in the general case
 * this reduces the input dimensionality A LOT.
 *
 * LEARNING OPPORTUNITY: You may want to learn (if not already aware) of
 * different ways to represent non scalar inputs in neural networks:
 * One hot encoding, learned embeddings, and even if it's just my random
 * exeriment this "permutation coding" that I'm using here.
 */
void board_to_inputs(GameState *state, float *inputs) {
    for (int i = 0; i < 9; i++) {
        if (state->board[i] == '.') {
            inputs[i*2] = 0;
            inputs[i*2+1] = 0;
        } else if (state->board[i] == 'X') {
            inputs[i*2] = 1;
            inputs[i*2+1] = 0;
        } else {  // 'O'
            inputs[i*2] = 0;
            inputs[i*2+1] = 1;
        }
    }
}

/* Check if the game is over (win or tie).
 * Very brutal but fast enough. */
int check_game_over(GameState *state, char *winner) {
    // Check rows.
    for (int i = 0; i < 3; i++) {
        if (state->board[i*3] != '.' &&
            state->board[i*3] == state->board[i*3+1] &&
            state->board[i*3+1] == state->board[i*3+2]) {
            *winner = state->board[i*3];
            return 1;
        }
    }

    // Check columns.
    for (int i = 0; i < 3; i++) {
        if (state->board[i] != '.' &&
            state->board[i] == state->board[i+3] &&
            state->board[i+3] == state->board[i+6]) {
            *winner = state->board[i];
            return 1;
        }
    }

    // Check diagonals.
    if (state->board[0] != '.' &&
        state->board[0] == state->board[4] &&
        state->board[4] == state->board[8]) {
        *winner = state->board[0];
        return 1;
    }
    if (state->board[2] != '.' &&
        state->board[2] == state->board[4] &&
        state->board[4] == state->board[6]) {
        *winner = state->board[2];
        return 1;
    }

    // Check for tie (no free tiles left).
    int empty_tiles = 0;
    for (int i = 0; i < 9; i++) {
        if (state->board[i] == '.') empty_tiles++;
    }
    if (empty_tiles == 0) {
        *winner = 'T';  // Tie
        return 1;
    }

    return 0; // Game continues.
}

/* Get the best move for the computer using the neural network.
 * Note that there is no complex sampling at all, we just get
 * the output with the highest value THAT has an empty tile. */
int get_computer_move(GameState *state, NeuralNetwork *nn, int display_probs) {
    float inputs[NN_INPUT_SIZE];

    board_to_inputs(state, inputs);
    forward_pass(nn, inputs);

    // Find the highest probability value and best legal move.
    float highest_prob = -1.0f;
    int highest_prob_idx = -1;
    int best_move = -1;
    float best_legal_prob = -1.0f;

    for (int i = 0; i < 9; i++) {
        // Track highest probability overall.
        if (nn->outputs[i] > highest_prob) {
            highest_prob = nn->outputs[i];
            highest_prob_idx = i;
        }

        // Track best legal move.
        if (state->board[i] == '.' &&
            (best_move == -1 || nn->outputs[i] > best_legal_prob))
        {
            best_move = i;
            best_legal_prob = nn->outputs[i];
        }
    }

    // That's just for debugging. It's interesting to show to user
    // in the first iterations of the game, since you can see how initially
    // the net picks illegal moves as best, and so forth.
    if (display_probs) {
        printf("Neural network move probabilities:\n");
        for (int row = 0; row < 3; row++) {
            for (int col = 0; col < 3; col++) {
                int pos = row * 3 + col;

                // Print probability as percentage.
                printf("%5.1f%%", nn->outputs[pos] * 100.0f);

                // Add markers.
                if (pos == highest_prob_idx) {
                    printf("*"); // Highest probability overall.
                }
                if (pos == best_move) {
                    printf("#"); // Selected move (highest valid probability).
                }
                printf(" ");
            }
            printf("\n");
        }

        // Sum of probabilities should be 1.0, hopefully.
        // Just debugging.
        float total_prob = 0.0f;
        for (int i = 0; i < 9; i++)
            total_prob += nn->outputs[i];
        printf("Sum of all probabilities: %.2f\n\n", total_prob);
    }
    return best_move;
}

/* Backpropagation function.
 * The only difference here from vanilla backprop is that we have
 * a 'reward_scaling' argument that makes the output error more/less
 * dramatic, so that we can adjust the weights proportionally to the
 * reward we want to provide. */
void backprop(NeuralNetwork *nn, float *target_probs, float learning_rate, float reward_scaling) {
    float output_deltas[NN_OUTPUT_SIZE];
    float hidden_deltas[NN_HIDDEN_SIZE];

    /* === STEP 1: Compute deltas === */

    /* Calculate output layer deltas:
     * Note what's going on here: we are technically using softmax
     * as output function and cross entropy as loss, but we never use
     * cross entropy in practice since we check the progresses in terms
     * of winning the game.
     *
     * Still calculating the deltas in the output as:
     *
     *      output[i] - target[i]
     *
     * Is exactly what happens if you derivate the deltas with
     * softmax and cross entropy.
     *
     * LEARNING OPPORTUNITY: This is a well established and fundamental
     * result in neural networks, you may want to read more about it. */
    for (int i = 0; i < NN_OUTPUT_SIZE; i++) {
        output_deltas[i] =
            (nn->outputs[i] - target_probs[i]) * fabsf(reward_scaling);
    }

    // Backpropagate error to hidden layer.
    for (int i = 0; i < NN_HIDDEN_SIZE; i++) {
        float error = 0;
        for (int j = 0; j < NN_OUTPUT_SIZE; j++) {
            error += output_deltas[j] * nn->weights_ho[i * NN_OUTPUT_SIZE + j];
        }
        hidden_deltas[i] = error * relu_derivative(nn->hidden[i]);
    }

    /* === STEP 2: Weights updating === */

    // Output layer weights and biases.
    for (int i = 0; i < NN_HIDDEN_SIZE; i++) {
        for (int j = 0; j < NN_OUTPUT_SIZE; j++) {
            nn->weights_ho[i * NN_OUTPUT_SIZE + j] -=
                learning_rate * output_deltas[j] * nn->hidden[i];
        }
    }
    for (int j = 0; j < NN_OUTPUT_SIZE; j++) {
        nn->biases_o[j] -= learning_rate * output_deltas[j];
    }

    // Hidden layer weights and biases.
    for (int i = 0; i < NN_INPUT_SIZE; i++) {
        for (int j = 0; j < NN_HIDDEN_SIZE; j++) {
            nn->weights_ih[i * NN_HIDDEN_SIZE + j] -=
                learning_rate * hidden_deltas[j] * nn->inputs[i];
        }
    }
    for (int j = 0; j < NN_HIDDEN_SIZE; j++) {
        nn->biases_h[j] -= learning_rate * hidden_deltas[j];
    }
}

/* Train the neural network based on game outcome.
 *
 * The move_history is just an integer array with the index of all the
 * moves. This function is designed so that you can specify if the
 * game was started by the move by the NN or human, but actually the
 * code always let the human move first. */
void learn_from_game(NeuralNetwork *nn, int *move_history, int num_moves, int nn_moves_even, char winner) {
    // Determine reward based on game outcome
    float reward;
    char nn_symbol = nn_moves_even ? 'O' : 'X';

    if (winner == 'T') {
        reward = 0.3f;  // Small reward for draw
    } else if (winner == nn_symbol) {
        reward = 1.0f;  // Large reward for win
    } else {
        reward = -2.0f; // Negative reward for loss
    }

    GameState state;
    float target_probs[NN_OUTPUT_SIZE];

    // Process each move the neural network made.
    for (int move_idx = 0; move_idx < num_moves; move_idx++) {
        // Skip if this wasn't a move by the neural network.
        if ((nn_moves_even && move_idx % 2 != 1) ||
            (!nn_moves_even && move_idx % 2 != 0))
        {
            continue;
        }

        // Recreate board state BEFORE this move was made.
        init_game(&state);
        for (int i = 0; i < move_idx; i++) {
            char symbol = (i % 2 == 0) ? 'X' : 'O';
            state.board[move_history[i]] = symbol;
        }

        // Convert board to inputs and do forward pass.
        float inputs[NN_INPUT_SIZE];
        board_to_inputs(&state, inputs);
        forward_pass(nn, inputs);

        /* The move that was actually made by the NN, that is
         * the one we want to reward (positively or negatively). */
        int move = move_history[move_idx];

        /* Here we can't really implement temporal difference in the strict
         * reinforcement learning sense, since we don't have an easy way to
         * evaluate if the current situation is better or worse than the
         * previous state in the game.
         *
         * However "time related" we do something that is very effective in
         * this case: we scale the reward according to the move time, so that
         * later moves are more impacted (the game is less open to different
         * solutions as we go forward).
         *
         * We give a fixed 0.5 importance to all the moves plus
         * a 0.5 that depends on the move position.
         *
         * NOTE: this makes A LOT of difference. Experiment with different
         * values.
         *
         * LEARNING OPPORTUNITY: Temporal Difference in Reinforcement Learning
         * is a very important result, that was worth the Turing Award in
         * 2024 to Sutton and Barto. You may want to read about it. */
        float move_importance = 0.5f + 0.5f * (float)move_idx/(float)num_moves;
        float scaled_reward = reward * move_importance;

        /* Create target probability distribution:
         * let's start with the logits all set to 0. */
        for (int i = 0; i < NN_OUTPUT_SIZE; i++)
            target_probs[i] = 0;

        /* Set the target for the chosen move based on reward: */
        if (scaled_reward >= 0) {
            /* For positive reward, set probability of the chosen move to
             * 1, with all the rest set to 0. */
            target_probs[move] = 1;
        } else {
            /* For negative reward, distribute probability to OTHER
             * valid moves, which is conceptually the same as discouraging
             * the move that we want to discourage. */
            int valid_moves_left = 9-move_idx-1;
            float other_prob = 1.0f / valid_moves_left;
            for (int i = 0; i < 9; i++) {
                if (state.board[i] == '.' && i != move) {
                    target_probs[i] = other_prob;
                }
            }
        }

        /* Call the generic backpropagation function, using
         * our target logits as target. */
        backprop(nn, target_probs, LEARNING_RATE, scaled_reward);
    }
}

/* Play one game of Tic Tac Toe against the neural network. */
void play_game(NeuralNetwork *nn) {
    GameState state;
    char winner;
    int move_history[9]; // Maximum 9 moves in a game.
    int num_moves = 0;

    init_game(&state);

    printf("Welcome to Tic Tac Toe! You are X, the computer is O.\n");
    printf("Enter positions as numbers from 0 to 8 (see picture).\n");

    while (!check_game_over(&state, &winner)) {
        display_board(&state);

        if (state.current_player == 0) {
            // Human turn.
            int move;
            char movec;
            printf("Your move (0-8): ");
            scanf(" %c", &movec);
            move = movec-'0'; // Turn character into number.

            // Check if move is valid.
            if (move < 0 || move > 8 || state.board[move] != '.') {
                printf("Invalid move! Try again.\n");
                continue;
            }

            state.board[move] = 'X';
            move_history[num_moves++] = move;
        } else {
            // Computer's turn
            printf("Computer's move:\n");
            int move = get_computer_move(&state, nn, 1);
            state.board[move] = 'O';
            printf("Computer placed O at position %d\n", move);
            move_history[num_moves++] = move;
        }

        state.current_player = !state.current_player;
    }

    display_board(&state);

    if (winner == 'X') {
        printf("You win!\n");
    } else if (winner == 'O') {
        printf("Computer wins!\n");
    } else {
        printf("It's a tie!\n");
    }

    // Learn from this game
    learn_from_game(nn, move_history, num_moves, 1, winner);
}

/* Get a random valid move, this is used for training
 * against a random opponent. Note: this function will loop forever
 * if the board is full, but here we want simple code. */
int get_random_move(GameState *state) {
    while(1) {
        int move = rand() % 9;
        if (state->board[move] != '.') continue;
        return move;
    }
}

/* Play a game against random moves and learn from it.
 *
 * This is a very simple Montecarlo Method applied to reinforcement
 * learning:
 *
 * 1. We play a complete random game (episode).
 * 2. We determine the reward based on the outcome of the game.
 * 3. We update the neural network in order to maximize future rewards.
 *
 * LEARNING OPPORTUNITY: while the code uses some Montecarlo-alike
 * technique, important results were recently obtained using
 * Montecarlo Tree Search (MCTS), where a tree structure repesents
 * potential future game states that are explored according to
 * some selection: you may want to learn about it. */
char play_random_game(NeuralNetwork *nn, int *move_history, int *num_moves) {
    GameState state;
    char winner = 0;
    *num_moves = 0;

    init_game(&state);

    while (!check_game_over(&state, &winner)) {
        int move;

        if (state.current_player == 0) {  // Random player's turn (X)
            move = get_random_move(&state);
        } else {  // Neural network's turn (O)
            move = get_computer_move(&state, nn, 0);
        }

        /* Make the move and store it: we need the moves sequence
         * during the learning stage. */
        char symbol = (state.current_player == 0) ? 'X' : 'O';
        state.board[move] = symbol;
        move_history[(*num_moves)++] = move;

        // Switch player.
        state.current_player = !state.current_player;
    }

    // Learn from this game - neural network is 'O' (even-numbered moves).
    learn_from_game(nn, move_history, *num_moves, 1, winner);
    return winner;
}

/* Train the neural network against random moves. */
void train_against_random(NeuralNetwork *nn, int num_games) {
    // Open a log file for training progress
    FILE *log_fp = fopen("training_log.json", "w"); // Overwrite/create log file
    if (!log_fp) {
        perror("Could not open training log file");
        // Consider exiting early or handling this situation differently
        return;
    }
    fprintf(log_fp, "[\n"); // Start JSON array
    int first_log_entry = 1; // To handle commas in JSON array

    int move_history[9];
    int num_moves;
    int wins = 0, losses = 0, ties = 0;

    printf("Training neural network against %d random games...\n", num_games);

    int played_games = 0;
    for (int i = 0; i < num_games; i++) {
        char winner = play_random_game(nn, move_history, &num_moves);
        played_games++;

        // Accumulate statistics
        if (winner == 'O') {
            wins++; // Neural network won.
        } else if (winner == 'X') {
            losses++; // Random player won.
        } else {
            ties++; // Tie.
        }

        // Show progress every 10,000 games and log to JSON file
        if ((i + 1) % 10000 == 0) {
            if (played_games == 0) {
                printf("No games played yet.\n");
                continue;
            }

            float win_pct = (float)wins * 100 / played_games;
            float loss_pct = (float)losses * 100 / played_games;
            float tie_pct = (float)ties * 100 / played_games;

            printf("Games: %d, Wins: %d (%.1f%%), "
                   "Losses: %d (%.1f%%), Ties: %d (%.1f%%)\n",
                   i + 1, wins, win_pct,
                   losses, loss_pct,
                   ties, tie_pct);

            // Log progress to JSON file
            if (log_fp) {
                if (!first_log_entry) {
                    fprintf(log_fp, ",\n"); // Add comma before next entry
                }
                fprintf(log_fp, "  {\n");
                fprintf(log_fp, "    \"games\": %d,\n", i + 1);
                fprintf(log_fp, "    \"wins\": %d,\n", wins);
                fprintf(log_fp, "    \"losses\": %d,\n", losses);
                fprintf(log_fp, "    \"ties\": %d,\n", ties);
                fprintf(log_fp, "    \"win_pct\": %.2f,\n", win_pct);
                fprintf(log_fp, "    \"loss_pct\": %.2f,\n", loss_pct);
                fprintf(log_fp, "    \"tie_pct\": %.2f\n", tie_pct);
                // Optional: Log specific weights/biases if needed
                // fprintf(log_fp, "    \"sample_weight_ih_0\": %f,\n", nn->weights_ih[0]);
                // fprintf(log_fp, "    \"sample_bias_o_0\": %f\n", nn->biases_o[0]);
                fprintf(log_fp, "  }");
                fflush(log_fp); // Ensure data is written to file
                first_log_entry = 0;
            }

            // Reset counters for next interval
            played_games = 0;
            wins = 0;
            losses = 0;
            ties = 0;
        }
    }

    printf("\nTraining complete!\n");

    // Close the JSON array and file
    if (log_fp) {
        fprintf(log_fp, "\n]\n"); // End JSON array
        fclose(log_fp);
        log_fp = NULL;
    }

    // Save the final trained neural network to a file
    //const char *nn_filename = "ttt_nn.dat"; // Ensure filename is defined
    //save_nn(nn, nn_filename); 
}



int main(int argc, char **argv) {
    int random_games = 150000;
    const char *nn_filename = "ttt_nn.dat";
    char *load_filename = NULL;
    char *save_filename = NULL;
    char *board_string = NULL;
    int train_mode = 1; // Default to training + interactive play
    int play_ai_once = 0; // Flag for backend mode

    // --- Argument Parsing ---
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--train") == 0 && i + 1 < argc) {
            random_games = atoi(argv[++i]);
            train_mode = 1;
            play_ai_once = 0; // Ensure not in AI-once mode if --train is specified
        } else if (strcmp(argv[i], "--no-train") == 0) {
             train_mode = 0;
             random_games = 0;
        } else if (strcmp(argv[i], "--load") == 0 && i + 1 < argc) {
            load_filename = argv[++i];
        } else if (strcmp(argv[i], "--save") == 0 && i + 1 < argc) {
            save_filename = argv[++i];
        } else if (strcmp(argv[i], "--play-ai") == 0 && i + 1 < argc) {
            // Mode for backend: get one AI move for a given board
            board_string = argv[++i];
            play_ai_once = 1;
            train_mode = 0; // Don't train or play interactively in this mode
        } else {
            // Keep original simple argument for training games if needed
            // Or add a --help message
             if (i == 1 && random_games == 150000) { // Assume first arg without flag is num_games
                 random_games = atoi(argv[i]);
             } else {
                 fprintf(stderr, "Usage: %s [--train <num_games>] [--no-train] [--load <filename>] [--save <filename>] [--play-ai <board_string>]\n", argv[0]);
                 fprintf(stderr, "  <board_string> is 9 chars, e.g., 'X.O.O.X..'\n");
                 return 1;
             }
        }
    }

    // Use default filenames if not provided
    if (!load_filename) load_filename = (char*)nn_filename;
    if (!save_filename) save_filename = (char*)nn_filename;

    srand(time(NULL));

    // Initialize neural network.
    NeuralNetwork nn;
    init_neural_network(&nn);

    // Try loading specified or default file
    load_nn(&nn, load_filename);

    // --- Mode Selection ---
    if (play_ai_once) {
        // --- Backend AI Move Mode ---
        if (!board_string || strlen(board_string) != 9) {
            fprintf(stderr, "Error: Invalid board string provided for --play-ai.\n");
            return 1;
        }

        GameState current_state;
        init_game(&current_state); // Initialize game state
        // Assume AI is 'O', so it's player 1's turn conceptually for the move calculation
        current_state.current_player = 1;
        int x_count = 0;
        int o_count = 0;
        for(int j=0; j<9; j++) {
            current_state.board[j] = board_string[j];
            if (board_string[j] == 'X') x_count++;
            else if (board_string[j] == 'O') o_count++;
        }

        // Basic turn check (optional but good)
         if (x_count < o_count || x_count > o_count + 1) {
             fprintf(stderr, "Warning: Invalid turn state in board string '%s'\n", board_string);
             // Decide how to handle: error out, or proceed anyway?
             // Proceeding might be okay if the NN can handle weird states.
         }
         // Set current player based on counts (more robust)
         current_state.current_player = (x_count == o_count) ? 0 : 1; // If counts equal, X moves. If X > O, O moves.

         // Check if game already over
         char winner_check;
         if (check_game_over(&current_state, &winner_check)) {
             // Handle game over state - maybe return -1 or specific code?
             // For now, let get_computer_move run; it might pick something,
             // or the calling backend should ideally check first.
             // A robust backend would check game over *before* calling this.
         }

        // Get the computer's move (AI is 'O')
        // Pass 0 to get_computer_move to disable probability printing
        int ai_move = get_computer_move(&current_state, &nn, 0);

        // --- Output ONLY the move index ---
        printf("%d\n", ai_move);
        // --- --- --- --- --- --- --- --- ---

        return 0; // Exit immediately after printing the move

    } else {
        // --- Training and/or Interactive Mode ---
        if (train_mode && random_games > 0) {
            printf("Starting training for %d games...\n", random_games);
            train_against_random(&nn, random_games); // This now also saves the NN
            printf("Saving neural network to %s...\n", save_filename);
            if (!save_nn(&nn, save_filename)) {
                fprintf(stderr, "Error: Failed to save neural network to %s\n", save_filename);
            }
        } else if (!train_mode) {
             printf("Skipping training (--no-train specified or implied).\n");
             // Ensure NN is loaded if skipping training but playing interactively
             if (!load_nn(&nn, load_filename)) {
                 printf("Warning: Failed to load NN and training is skipped. Playing with untrained network.\n");
             }
        }

        // Play game with human and learn more (original loop)
        while(1) {
            char play_again;
            play_game(&nn); // play_game now uses the trained/loaded nn

            // Save after interactive game? Optional, could drift the model.
            // save_nn(&nn, save_filename);

            printf("Play again? (y/n): ");
            int c;
            // Clear input buffer before scanf for char
            while ((c = getchar()) != '\n' && c != EOF);
            scanf(" %c", &play_again);
             // Clear input buffer after scanf for char
            while ((c = getchar()) != '\n' && c != EOF);

            if (play_again != 'y' && play_again != 'Y') break;
        }
    }

    // Save the final neural network if not already saved during training
    if (save_filename && train_mode) {
        save_nn(&nn, save_filename);
    }

    return 0;
}


/*
- Aggiunte funzioni save_nn e load_nn
- Modificato il main per evitare di allenare perennemente il modello se sono già presenti i pesi
- Modificato train_against_random per scrivere periodicamente dati rilevanti (win/loss/tie stats) per la webui
- Modificato ulteriormente il main per avere un'esecuzione selettiva basata sui parametri
*/