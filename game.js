// ── State ────────────────────────────────────────────────────────────────────
let mode = null;          // "pvp" | "ai"
let board = [];           // 9-element array: null | "X" | "O"
let currentPlayer = "X";
let gameOver = false;

const WIN_LINES = [
  [0, 1, 2], [3, 4, 5], [6, 7, 8], // rows
  [0, 3, 6], [1, 4, 7], [2, 5, 8], // cols
  [0, 4, 8], [2, 4, 6],             // diagonals
];

// ── Screens ──────────────────────────────────────────────────────────────────
function showScreen(id) {
  document.querySelectorAll('.screen').forEach(s => s.classList.add('hidden'));
  document.getElementById(id).classList.remove('hidden');
}

// ── Game lifecycle ────────────────────────────────────────────────────────────
function startGame(selectedMode) {
  mode = selectedMode;
  board = Array(9).fill(null);
  currentPlayer = "X";
  gameOver = false;

  // Clear board cells
  document.querySelectorAll('.cell').forEach(cell => {
    cell.textContent = '';
    cell.className = 'cell';
  });

  setStatus(`Player X's turn`);
  showScreen('game-screen');
}

function resetGame() {
  showScreen('mode-screen');
}

// ── Cell click ────────────────────────────────────────────────────────────────
function handleCellClick(index) {
  if (gameOver || board[index]) return;
  if (mode === 'ai' && currentPlayer === 'O') return; // block clicks during AI turn

  placeMarker(index, currentPlayer);

  const winner = checkWinner(board);
  if (winner) {
    endGame(winner);
    return;
  }
  if (checkDraw(board)) {
    endGame(null);
    return;
  }

  currentPlayer = currentPlayer === 'X' ? 'O' : 'X';
  setStatus(`Player ${currentPlayer}'s turn`);

  if (mode === 'ai' && currentPlayer === 'O') {
    setTimeout(aiMove, 300);
  }
}

function placeMarker(index, player) {
  board[index] = player;
  const cell = document.querySelector(`.cell[data-index="${index}"]`);
  cell.textContent = player;
  cell.classList.add(player.toLowerCase(), 'taken');
}

// ── Win / Draw detection ──────────────────────────────────────────────────────
function checkWinner(b) {
  for (const [a, c, d] of WIN_LINES) {
    if (b[a] && b[a] === b[c] && b[a] === b[d]) return b[a];
  }
  return null;
}

function checkDraw(b) {
  return b.every(cell => cell !== null);
}

function getWinLine(b) {
  for (const line of WIN_LINES) {
    const [a, c, d] = line;
    if (b[a] && b[a] === b[c] && b[a] === b[d]) return line;
  }
  return null;
}

// ── End game ──────────────────────────────────────────────────────────────────
function endGame(winner) {
  gameOver = true;
  if (winner) {
    const line = getWinLine(board);
    highlightWinner(line);
    const label = mode === 'ai' && winner === 'O' ? 'AI wins!' : `Player ${winner} wins!`;
    setStatus(label, 'winner');
  } else {
    setStatus("It's a draw!", 'draw');
  }
}

function highlightWinner(line) {
  line.forEach(i => {
    document.querySelector(`.cell[data-index="${i}"]`).classList.add('win');
  });
}

function setStatus(message, type = '') {
  const el = document.getElementById('status');
  el.textContent = message;
  el.className = 'status' + (type ? ' ' + type : '');
}

// ── AI (minimax) ──────────────────────────────────────────────────────────────
function aiMove() {
  const index = getBestMove(board);
  placeMarker(index, 'O');

  const winner = checkWinner(board);
  if (winner) {
    endGame(winner);
    return;
  }
  if (checkDraw(board)) {
    endGame(null);
    return;
  }

  currentPlayer = 'X';
  setStatus(`Player X's turn`);
}

function getBestMove(b) {
  let bestScore = -Infinity;
  let bestIndex = -1;

  for (let i = 0; i < 9; i++) {
    if (b[i]) continue;
    b[i] = 'O';
    const score = minimax(b, false);
    b[i] = null;
    if (score > bestScore) {
      bestScore = score;
      bestIndex = i;
    }
  }
  return bestIndex;
}

function minimax(b, isMaximizing) {
  const winner = checkWinner(b);
  if (winner === 'O') return 10;
  if (winner === 'X') return -10;
  if (checkDraw(b)) return 0;

  if (isMaximizing) {
    let best = -Infinity;
    for (let i = 0; i < 9; i++) {
      if (b[i]) continue;
      b[i] = 'O';
      best = Math.max(best, minimax(b, false));
      b[i] = null;
    }
    return best;
  } else {
    let best = Infinity;
    for (let i = 0; i < 9; i++) {
      if (b[i]) continue;
      b[i] = 'X';
      best = Math.min(best, minimax(b, true));
      b[i] = null;
    }
    return best;
  }
}
